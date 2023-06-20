#include "edge/edge_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/basic_edge_wrapper.h"
#include "edge/covered_edge_wrapper.h"
#include "message/data_message.h"
#include "message/message_base.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string EdgeWrapperBase::kClassName("EdgeWrapperBase");

    EdgeWrapperBase* EdgeWrapperBase::getEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr, const uint32_t& capacity_bytes)
    {
        EdgeWrapperBase* edge_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            edge_wrapper_ptr = new CoveredEdgeWrapper(cache_name, hash_name, edge_param_ptr, capacity_bytes);
        }
        else
        {
            edge_wrapper_ptr = new BasicEdgeWrapper(cache_name, hash_name, edge_param_ptr, capacity_bytes);
        }

        assert(edge_wrapper_ptr != NULL);
        return edge_wrapper_ptr;
    }

    EdgeWrapperBase::EdgeWrapperBase(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr, const uint32_t& capacity_bytes) : cache_name_(cache_name), edge_param_ptr_(edge_param_ptr), capacity_bytes_(capacity_bytes)
    {
        if (edge_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "edge_param_ptr is NULL!");
            exit(1);
        }

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        base_instance_name_ = oss.str();

        // Allocate per-key rwlock for serializability
        perkey_rwlock_for_serializability_ptr_ = new PerkeyRwlock(edge_param_ptr->getEdgeIdx());
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = CacheWrapperBase::getEdgeCache(cache_name_, edge_param_ptr);
        assert(edge_cache_ptr_ != NULL);

        // Allocate cooperation wrapper for cooperative edge caching
        cooperation_wrapper_ptr_ = CooperationWrapperBase::getCooperationWrapper(cache_name, hash_name, edge_param_ptr);
        assert(cooperation_wrapper_ptr_ != NULL);

        // Prepare a socket server on recvreq port
        uint32_t edge_idx = edge_param_ptr_->getEdgeIdx();
        uint16_t edge_recvreq_port = Util::getEdgeRecvreqPort(edge_idx);
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_recvreq_port);
        edge_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(edge_recvreq_socket_server_ptr_ != NULL);

        // Prepare a socket client to cloud recvreq port
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        NetworkAddr remote_addr(cloud_ipstr, cloud_recvreq_port);
        edge_sendreq_tocloud_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, remote_addr);
        assert(edge_sendreq_tocloud_socket_client_ptr_ != NULL);
    }
        
    EdgeWrapperBase::~EdgeWrapperBase()
    {
        // NOTE: no need to delete edge_param_ptr, as it is maintained outside EdgeWrapperBase

        // Release per-key rwlock
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        delete perkey_rwlock_for_serializability_ptr_;
        perkey_rwlock_for_serializability_ptr_ = NULL;

        // Release local edge cache
        assert(edge_cache_ptr_ != NULL);
        delete edge_cache_ptr_;
        edge_cache_ptr_ = NULL;

        // Release cooperative cache wrapper
        assert(cooperation_wrapper_ptr_ != NULL);
        delete cooperation_wrapper_ptr_;
        cooperation_wrapper_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(edge_recvreq_socket_server_ptr_ != NULL);
        delete edge_recvreq_socket_server_ptr_;
        edge_recvreq_socket_server_ptr_ = NULL;

        // Release the socket client to cloud
        assert(edge_sendreq_tocloud_socket_client_ptr_ != NULL);
        delete edge_sendreq_tocloud_socket_client_ptr_;
        edge_sendreq_tocloud_socket_client_ptr_ = NULL;
    }

    void EdgeWrapperBase::start()
    {
        assert(edge_param_ptr_ != NULL);
        assert(edge_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished
        while (edge_param_ptr_->isEdgeRunning()) // edge_running_ is set as true by default
        {
            // Receive the message payload of data (local/redirected/global) or control requests
            DynamicArray request_msg_payload;
            NetworkAddr request_network_addr;
            bool is_timeout = edge_recvreq_socket_server_ptr_->recv(request_msg_payload, request_network_addr);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {
                assert(request_network_addr.isValid());
                
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(request_msg_payload);
                assert(request_ptr != NULL);

                if (request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    is_finish = processDataRequest_(request_ptr);
                }
                else if (request_ptr->isControlRequest()) // Control requests (e.g., invalidation and cache admission/eviction requests)
                {
                    is_finish = processControlRequest_(request_ptr, request_network_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Release messages
                assert(request_ptr != NULL);
                delete request_ptr;
                request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_timeout == false)
        } // End of while loop
    }

    // (1) Data requests

    bool EdgeWrapperBase::processDataRequest_(MessageBase* data_request_ptr)
    {
        // No need to acquire per-key rwlock for serializability, which will be done in processLocalGetRequest_() or processLocalWriteRequest_()

        assert(data_request_ptr != NULL && data_request_ptr->isDataRequest());

        bool is_finish = false; // Mark if edge node is finished

        if (data_request_ptr->isLocalRequest()) // Local request
        {
            // Get key from local request
            Key tmp_key = MessageBase::getKeyFromMessage(data_request_ptr);
            Value tmp_value();

            // TMPDEBUG
            std::ostringstream oss;
            oss << "receive a local request; type: " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << "; keystr: " << tmp_key.getKeystr();
            Util::dumpDebugMsg(base_instance_name_, oss.str());

            if (data_request_ptr->getMessageType() == MessageType::kLocalGetRequest)
            {
                is_finish = processLocalGetRequest_(data_request_ptr);
            }
            else // Local put/del request
            {
                is_finish = processLocalWriteRequest_(data_request_ptr);
            }
        }
        else if (data_request_ptr->isRedirectedRequest()) // Redirected request
        {
            is_finish = processRedirectedRequest_(data_request_ptr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for processDataRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool EdgeWrapperBase::processLocalGetRequest_(MessageBase* local_request_ptr) const
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL && local_request_ptr->getMessageType() == MessageType::kLocalGetRequest);
        const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(local_request_ptr);
        Key tmp_key = local_get_request_ptr->getKey();
        Value tmp_value;

        // Acquire a read lock for serializability before accessing any shared variable in the closest edge node
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock_shared(tmp_key, "processLocalGetRequest_()"))
            {
                break;
            }
        }

        assert(edge_cache_ptr_ != NULL);
        assert(cooperation_wrapper_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished
        Hitflag hitflag = Hitflag::kGlobalMiss;

        // Access local edge cache (current edge node is the closest edge node)
        bool is_local_cached_and_valid = edge_cache_ptr_->get(tmp_key, tmp_value);
        if (is_local_cached_and_valid) // local cached and valid
        {
            hitflag = Hitflag::kLocalHit;
        }

        // Access cooperative edge cache for local cache miss or invalid object
        bool is_cooperative_cached_and_valid = false;
        if (!is_local_cached_and_valid) // not local cached or invalid
        {
            // Get data from some target edge node for local cache miss
            is_finish = cooperation_wrapper_ptr_->get(tmp_key, tmp_value, is_cooperative_cached_and_valid);
            if (is_finish) // Edge node is NOT running
            {
                perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
                return is_finish;
            }
            if (is_cooperative_cached_and_valid) // cooperative cached and valid
            {
                hitflag = Hitflag::kCooperativeHit;
            }
        }

        // Get data from cloud for global cache miss
        if (!is_local_cached_and_valid && !is_cooperative_cached_and_valid) // (not cached or invalid) in both local and cooperative cache
        {
            is_finish = fetchDataFromCloud_(tmp_key, tmp_value);
            if (is_finish) // Edge node is NOT running
            {
                perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
                return is_finish;
            }
        }

        // Update invalid object of local edge cache if necessary
        tryToUpdateLocalEdgeCache_(tmp_key, tmp_value);

        // Trigger independent cache admission for local/global cache miss if necessary
        tryToTriggerIndependentAdmission_(tmp_key, tmp_value);

        // Prepare LocalGetResponse for client
        LocalGetResponse local_get_response(tmp_key, tmp_value, hitflag);

        // Reply local response message to a client (the remote address set by the most recent recv)
        DynamicArray local_response_msg_payload(local_get_response.getMsgPayloadSize());
        local_get_response.serialize(local_response_msg_payload);
        PropagationSimulator::propagateFromEdgeToClient();
        edge_recvreq_socket_server_ptr_->send(local_response_msg_payload);

        // TMPDEBUG
        std::ostringstream oss;
        oss << "issue a local response; type: " << MessageBase::messageTypeToString(local_get_response.getMessageType()) << "; keystr:" << local_get_response.getKey().getKeystr();
        Util::dumpDebugMsg(base_instance_name_, oss.str());

        perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
        return is_finish;
    }

    bool EdgeWrapperBase::processLocalWriteRequest_(MessageBase* local_request_ptr)
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL);
        assert(local_request_ptr->getMessageType() == MessageType::kLocalPutRequest || local_request_ptr->getMessageType() == MessageType::kLocalDelRequest);
        Key tmp_key;
        Value tmp_value;
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            const LocalPutRequest* const local_put_request_ptr = static_cast<const LocalPutRequest*>(local_request_ptr);
            tmp_key = local_put_request_ptr->getKey();
            tmp_value = local_put_request_ptr->getValue();
            assert(tmp_value.isDeleted() == false);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            const LocalDelRequest* const local_del_request_ptr = static_cast<const LocalDelRequest*>(local_request_ptr);
            tmp_key = local_del_request_ptr->getKey();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(local_request_ptr->getMessageType()) << " for processLocalWriteRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Acquire a write lock for serializability before accessing any shared variable in the closest edge node
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock(tmp_key, "processLocalWriteRequest_()"))
            {
                break;
            }
        }
        
        assert(edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished
        const Hitflag hitflag = Hitflag::kGlobalMiss; // Must be global miss due to write-through policy

        // TODO: Acquire write lock from beacon node for MSI protocol
        // TODO: Update is_finish

        if (is_finish) // Edge node is NOT running
        {
            perkey_rwlock_for_serializability_ptr_->unlock(tmp_key);
            return is_finish;
        }

        // TODO: Wait for beacon node to invalidate all other cache copies for MSI protocol
        // TODO: Update is_finish

        if (is_finish) // Edge node is NOT running
        {
            perkey_rwlock_for_serializability_ptr_->unlock(tmp_key);
            return is_finish;
        }

        // Send request to cloud for write-through policy
        is_finish = writeDataToCloud_(tmp_key, tmp_value, local_request_ptr->getMessageType());

        if (is_finish) // Edge node is NOT running
        {
            perkey_rwlock_for_serializability_ptr_->unlock(tmp_key);
            return is_finish;
        }

        // Update/remove local edge cache
        bool is_local_cached = false;
        MessageBase* local_response_ptr = NULL;
        // NOTE: message type has been checked, which must be one of the following two types
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            is_local_cached = edge_cache_ptr_->update(tmp_key, tmp_value);

            // Prepare LocalPutResponse for client
            local_response_ptr = new LocalPutResponse(tmp_key, hitflag);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            is_local_cached = edge_cache_ptr_->remove(tmp_key);

            // Prepare LocalDelResponse for client
            local_response_ptr = new LocalDelResponse(tmp_key, hitflag);
        }
        UNUSED(is_local_cached);

        // Trigger independent cache admission for local/global cache miss if necessary
        tryToTriggerIndependentAdmission_(tmp_key, tmp_value);

        // TODO: Notify beacon node to finish write and clear blocked edge nodes
        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision
        // TODO: Update is_finish

        if (!is_finish) // // Edge node is STILL running
        {
            // Reply local response message to a client (the remote address set by the most recent recv)
            assert(local_response_ptr != NULL);
            assert(local_response_ptr->getMessageType() == MessageType::kLocalPutResponse || local_response_ptr->getMessageType() == MessageType::kLocalDelResponse);
            DynamicArray local_response_msg_payload(local_response_ptr->getMsgPayloadSize());
            local_response_ptr->serialize(local_response_msg_payload);
            PropagationSimulator::propagateFromEdgeToClient();
            edge_recvreq_socket_server_ptr_->send(local_response_msg_payload);
        }

        // Release response message
        assert(local_response_ptr != NULL);
        delete local_response_ptr;
        local_response_ptr = NULL;

        perkey_rwlock_for_serializability_ptr_->unlock(tmp_key);
        return is_finish;
    }

    bool EdgeWrapperBase::processRedirectedRequest_(MessageBase* redirected_request_ptr)
    {
        // No need to acquire per-key rwlock for serializability, which will be done in processRedirectedGetRequest_()

        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedRequest());

        bool is_finish = false; // Mark if local edge node is finished

        MessageType message_type = redirected_request_ptr->getMessageType();
        if (message_type == MessageType::kRedirectedGetRequest)
        {
            is_finish = processRedirectedGetRequest_(redirected_request_ptr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for processRedirectedRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool EdgeWrapperBase::fetchDataFromCloud_(const Key& key, Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_()

        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_tocloud_socket_client_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished

        GlobalGetRequest global_get_request(key);
        DynamicArray global_request_msg_payload(global_get_request.getMsgPayloadSize());
        global_get_request.serialize(global_request_msg_payload);

        // TMPDEBUG
        std::ostringstream oss;
        oss << "issue a global request; type: " << MessageBase::messageTypeToString(global_get_request.getMessageType()) << "; keystr:" << key.getKeystr();
        Util::dumpDebugMsg(base_instance_name_, oss.str());

        while (true) // Timeout-and-retry
        {
            // Send the message payload of global request to cloud
            PropagationSimulator::propagateFromEdgeToCloud();
            edge_sendreq_tocloud_socket_client_ptr_->send(global_request_msg_payload);

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_sendreq_tocloud_socket_client_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for GlobalGetResponse");
                    continue; // Resend the global request message
                }
            }
            else
            {
                // Receive the global response message successfully
                MessageBase* global_response_ptr = MessageBase::getResponseFromMsgPayload(global_response_msg_payload);
                assert(global_response_ptr != NULL && global_response_ptr->getMessageType() == MessageType::kGlobalGetResponse);
                
                // Get value from global response message
                const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(global_response_ptr);
                value = global_get_response_ptr->getValue();

                // TMPDEBUG
                oss.clear();
                oss.str("");
                oss << "receive a global response; type: " << MessageBase::messageTypeToString(global_response_ptr->getMessageType()) << "; keystr:" << global_get_response_ptr->getKey().getKeystr();
                Util::dumpDebugMsg(base_instance_name_, oss.str());

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop
        
        return is_finish;
    }

    bool EdgeWrapperBase::writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type)
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalWriteRequest_()
        
        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_tocloud_socket_client_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished

        // Prepare global write request message
        MessageBase* global_request_ptr = NULL;
        if (message_type == MessageType::kGlobalPutRequest)
        {
            global_request_ptr = new GlobalPutRequest(key, value);
        }
        else if (message_type == MessageType::kGlobalDelRequest)
        {
            global_request_ptr = new GlobalDelRequest(key);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for writeDataToCloud_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        assert(global_request_ptr != NULL);

        DynamicArray global_request_msg_payload(global_request_ptr->getMsgPayloadSize());
        global_request_ptr->serialize(global_request_msg_payload);
        while (true) // Timeout-and-retry
        {
            // Send the message payload of global request to cloud
            PropagationSimulator::propagateFromEdgeToCloud();
            edge_sendreq_tocloud_socket_client_ptr_->send(global_request_msg_payload);

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_sendreq_tocloud_socket_client_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    continue; // Resend the global request message
                }
            }
            else
            {
                // Receive the global response message successfully
                MessageBase* global_response_ptr = MessageBase::getResponseFromMsgPayload(global_response_msg_payload);
                assert(global_response_ptr != NULL);
                assert(global_response_ptr->getMessageType() == MessageType::kGlobalPutResponse || global_response_ptr->getMessageType() == MessageType::kGlobalDelResponse);

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop

        // Release global request message
        assert(global_request_ptr != NULL);
        delete global_request_ptr;
        global_request_ptr = NULL;
        
        return is_finish;
    }

    void EdgeWrapperBase::tryToUpdateLocalEdgeCache_(const Key& key, const Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_()
        
        bool is_local_cached = edge_cache_ptr_->isLocalCached(key);
        bool is_cached_and_invalid = false;
        if (is_local_cached)
        {
            bool is_valid = edge_cache_ptr_->isValidIfCached(key);
            is_cached_and_invalid = is_local_cached && !is_valid;
        }

        if (is_cached_and_invalid)
        {
            bool is_local_cached_after_udpate = edge_cache_ptr_->update(key, value);
            assert(is_local_cached_after_udpate);

            // Evict until used bytes <= capacity bytes
            while (true)
            {
                // Data and metadata for local edge cache, and cooperation metadata
                uint32_t used_bytes = edge_cache_ptr_->getSizeForCapacity() + cooperation_wrapper_ptr_->getSizeForCapacity();
                if (used_bytes <= capacity_bytes_) // Not exceed capacity limitation
                {
                    break;
                }
                else // Exceed capacity limitation
                {
                    Key victim_key;
                    Value victim_value;
                    edge_cache_ptr_->evict(victim_key, victim_value);
                    bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
                    cooperation_wrapper_ptr_->updateDirectory(victim_key, false, _unused_is_being_written);

                    continue;
                }
            }
        }

        return;
    }

    void EdgeWrapperBase::tryToTriggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_(), processLocalWriteRequest_(), and processRedirectedGetRequest_()

        bool is_local_cached = edge_cache_ptr_->isLocalCached(key);
        if (!is_local_cached && edge_cache_ptr_->needIndependentAdmit(key))
        {
            triggerIndependentAdmission_(key, value);
        }
    }

    // (2) Control requests

    bool EdgeWrapperBase::processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr)
    {
        // No need to acquire per-key rwlock for serializability, which will be done in processDirectoryLookupRequest_() or processDirectoryUpdateRequest_() or processOtherControlRequest_()

        assert(control_request_ptr != NULL && control_request_ptr->isControlRequest());
        assert(edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if edge node is finished

        MessageType message_type = control_request_ptr->getMessageType();
        if (message_type == MessageType::kDirectoryLookupRequest) // TODO: control_request_ptr->isDirectoryLookupRequest() for kCoveredDirectoryLookupRequest
        {
            is_finish = processDirectoryLookupRequest_(control_request_ptr, closest_edge_addr);
        }
        else if (message_type == MessageType::kDirectoryUpdateRequest) // TODO: control_request_ptr->isDirectoryUpdateRequest() for kCoveredDirectoryUpdateRequest
        {
            is_finish = processDirectoryUpdateRequest_(control_request_ptr);
        }
        else
        {
            // NOTE: only COVERED has other control requests to process
            is_finish = processOtherControlRequest_(control_request_ptr);
        }
        // TODO: invalidation and cache admission/eviction requests for control message
        // TODO: reply control response message to a beacon node
        return is_finish;
    }
}
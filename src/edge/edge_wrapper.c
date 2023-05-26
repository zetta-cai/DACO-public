#include "edge/edge_wrapper.h"

#include <assert.h>

#include "common/util.h"
#include "message/global_message.h"
#include "message/local_message.h"
#include "message/message_base.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string EdgeWrapper::kClassName("EdgeWrapper");

    void* EdgeWrapper::launchEdge(void* local_edge_param_ptr)
    {
        EdgeWrapper local_edge((EdgeParam*)local_edge_param_ptr);
        local_edge.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EdgeWrapper::EdgeWrapper(EdgeParam* local_edge_param_ptr)
    {
        if (local_edge_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_edge_param_ptr is NULL!");
            exit(1);
        }
        local_edge_param_ptr_ = local_edge_param_ptr;
        assert(local_edge_param_ptr_ != NULL);
        
        // Allocate local edge cache
        local_edge_cache_ptr_ = CacheWrapperBase::getEdgeCache(Param::getCacheName(), Param::getCapacityBytes());
        assert(local_edge_cache_ptr_ != NULL);

        // Prepare a socket server on recvreq port
        uint32_t global_edge_idx = local_edge_param_ptr_->getGlobalEdgeIdx();
        uint16_t local_edge_recvreq_port = Util::getLocalEdgeRecvreqPort(global_edge_idx);
        NetworkAddr host_addr(Util::ANY_IPSTR, local_edge_recvreq_port);
        local_edge_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(local_edge_recvreq_socket_server_ptr_ != NULL);

        // Prepare a socket client to cloud recvreq port
        std::string global_cloud_ipstr = Util::getGlobalCloudIpstr();
        uint16_t global_cloud_recvreq_port = Config::getGlobalCloudRecvreqPort();
        NetworkAddr remote_addr(global_cloud_ipstr, global_cloud_recvreq_port);
        local_edge_sendreq_tocloud_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, remote_addr);
        assert(local_edge_sendreq_tocloud_socket_client_ptr_ != NULL);
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // NOTE: no need to delete local_edge_param_ptr, as it is maintained outside EdgeWrapper

        // Release local edge cache
        assert(local_edge_param_ptr_ != NULL);
        delete local_edge_param_ptr_;
        local_edge_param_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(local_edge_recvreq_socket_server_ptr_ != NULL);
        delete local_edge_recvreq_socket_server_ptr_;
        local_edge_recvreq_socket_server_ptr_ = NULL;

        // Release the socket client to cloud
        assert(local_edge_sendreq_tocloud_socket_client_ptr_ != NULL);
        delete local_edge_sendreq_tocloud_socket_client_ptr_;
        local_edge_sendreq_tocloud_socket_client_ptr_ = NULL;
    }

    void EdgeWrapper::start()
    {
        assert(local_edge_param_ptr_ != NULL);
        assert(local_edge_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished
        while (local_edge_param_ptr_->isEdgeRunning()) // local_edge_running_ is set as true by default
        {
            // Receive the message payload of data (local/redirected/global) or control requests
            DynamicArray request_msg_payload;
            bool is_timeout = local_edge_recvreq_socket_server_ptr_->recv(request_msg_payload);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(request_msg_payload);
                assert(request_ptr != NULL);

                if (request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    is_finish = processDataRequest_(request_ptr);
                }
                else if (request_ptr->isControlRequest()) // Control requests (e.g., invalidation and cache admission/eviction requests)
                {
                    is_finish = processControlRequest_(request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(kClassName, oss.str());
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

    bool EdgeWrapper::processDataRequest_(MessageBase* data_request_ptr)
    {
        assert(data_request_ptr != NULL && data_request_ptr->isDataRequest());
        assert(local_edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        if (data_request_ptr->isLocalRequest()) // Local request
        {
            // Get key from local request
            Key tmp_key = MessageBase::getKeyFromMessage(data_request_ptr);
            Value tmp_value();

            // Block until not invalidated
            is_finish = blockForInvalidation_(tmp_key);

            if (is_finish) // Check is_finish
            {
                return is_finish;
            }

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
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool EdgeWrapper::processLocalGetRequest_(MessageBase* local_request_ptr)
    {
        assert(local_request_ptr != NULL && local_request_ptr->getMessageType() == MessageType::kLocalGetRequest);
        assert(local_edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished
        Hitflag hitflag = Hitflag::kGlobalMiss;

        const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(local_request_ptr);
        Key tmp_key = local_get_request_ptr->getKey();
        Value tmp_value();
        bool is_local_cached = local_edge_cache_ptr_->get(tmp_key, tmp_value);
        if (is_local_cached)
        {
            hitflag = Hitflag::kLocalHit;
        }

        bool is_cooperative_cached = false;
        if (!is_local_cached)
        {
            // TODO: Get data from neighbor for local cache miss
            // TODO: Update is_finish and is_cooperative_cached
            if (is_cooperative_cached)
            {
                hitflag = Hitflag::kCooperativeHit;
            }
        }

        // Get data from cloud for global cache miss
        if (!is_local_cached && !is_cooperative_cached)
        {
            is_finish = fetchDataFromCloud_(tmp_key, tmp_value);
        }

        if (is_finish) // Check is_finish
        {
            return is_finish;
        }

        // TODO: For COVERED, beacon node will tell the edge node whether to admit, w/o independent decision

        // Trigger independent cache admission for local/global cache miss if necessary
        if (!is_local_cached && local_edge_cache_ptr_->needIndependentAdmit(tmp_key))
        {
            triggerIndependentAdmission_(tmp_key, tmp_value);
        }

        // Prepare LocalGetResponse for client
        LocalGetResponse local_get_response(tmp_key, tmp_value, hitflag);

        // Reply local response message to a client (the remote address set by the most recent recv)
        DynamicArray local_response_msg_payload(local_get_response.getMsgPayloadSize());
        local_get_response.serialize(local_response_msg_payload);
        local_edge_recvreq_socket_server_ptr_->send(local_response_msg_payload);

        return is_finish;
    }

    bool EdgeWrapper::processLocalWriteRequest_(MessageBase* local_request_ptr)
    {
        assert(local_request_ptr != NULL);
        assert(local_request_ptr->getMessageType() == MessageType::kLocalPutRequest || local_request_ptr->getMessageType() == MessageType::kLocalDelRequest);
        assert(local_edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished
        const Hitflag hitflag = Hitflag::kGlobalMiss; // Must be global miss due to write-through policy

        // TODO: Acquire write lock from beacon node
        // TODO: Update is_finish

        if (is_finish) // Check is_finish
        {
            return is_finish;
        }

        // TODO: Wait for beacon node to invalidate all other cache copies
        // TODO: Update is_finish

        if (is_finish) // Check is_finish
        {
            return is_finish;
        }

        // TODO: Send request to cloud for write-through policy
        // TODO: Update is_finish

        if (is_finish) // Check is_finish
        {
            return is_finish;
        }

        // Update/remove local edge cache
        bool is_local_cached = false;
        Key tmp_key();
        Value tmp_value();
        MessageBase* local_response_ptr = NULL;
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            const LocalPutRequest* const local_put_request_ptr = static_cast<const LocalPutRequest*>(local_request_ptr);
            tmp_key = local_put_request_ptr->getKey();
            tmp_value = local_put_request_ptr->getValue();
            assert(tmp_value.isDeleted() == false);

            is_local_cached = local_edge_cache_ptr_->update(tmp_key, tmp_value);

            // Prepare LocalPutResponse for client
            local_response_ptr = new LocalPutResponse(tmp_key, hitflag);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            const LocalDelRequest* const local_del_request_ptr = static_cast<const LocalDelRequest*>(local_request_ptr);
            tmp_key = local_del_request_ptr->getKey();
            is_local_cached = local_edge_cache_ptr_->remove(tmp_key);

            // Prepare LocalDelResponse for client
            local_response_ptr = new LocalDelResponse(tmp_key, hitflag);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(local_request_ptr->getMessageType()) << " for processLocalWriteRequest_()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Trigger independent cache admission for local/global cache miss if necessary
        if (!is_local_cached && local_edge_cache_ptr_->needIndependentAdmit(tmp_key))
        {
            triggerIndependentAdmission_(tmp_key, tmp_value);
        }

        // TODO: Notify beacon node to validate all other cache copies
        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision
        // TODO: Update is_finish

        if (!is_finish) // Check is_finish
        {
            // Reply local response message to a client (the remote address set by the most recent recv)
            assert(local_response_ptr != NULL);
            assert(local_response_ptr->getMessageType() == MessageType::kLocalPutResponse || local_response_ptr->getMessageType() == MessageType::kLocalDelResponse);
            DynamicArray local_response_msg_payload(local_response_ptr->getMsgPayloadSize());
            local_response_ptr->serialize(local_response_msg_payload);
            local_edge_recvreq_socket_server_ptr_->send(local_response_msg_payload);
        }

        // Release response message
        assert(local_response_ptr != NULL);
        delete local_response_ptr;
        local_response_ptr = NULL;

        return is_finish;
    }

    bool EdgeWrapper::processRedirectedRequest_(MessageBase* redirected_request_ptr)
    {
        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedRequest());
        assert(local_edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        // TODO: redirected request for data message (the same as local requests???)
        // TODO: reply redirected response message to an edge node
        // assert(redirected_response_ptr != NULL && redirected_response_ptr->isRedirectedResponse());
        return is_finish;
    }

    bool EdgeWrapper::processControlRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL && control_request_ptr->isControlRequest());
        assert(local_edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        // TODO: invalidation and cache admission/eviction requests for control message
        // TODO: reply control response message to a beacon node
        // assert(control_response_ptr != NULL && control_response_ptr->isControlResponse());
        return is_finish;
    }

    bool EdgeWrapper::blockForInvalidation_(const Key& key)
    {
        assert(local_edge_param_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        bool is_invalidated = false;
        while (true)
        {
            is_invalidated = local_edge_cache_ptr_->isInvalidated(key);
            if (is_invalidated)
            {
                if (!local_edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break;
                }
                else
                {
                    // TODO: sleep a short time to avoid frequent polling
                    continue;
                }
            }
            else
            {
                break;
            }
        }
        assert(!is_invalidated);

        return is_finish;
    }

    bool EdgeWrapper::fetchDataFromCloud_(const Key& key, Value& value)
    {
        assert(local_edge_param_ptr_ != NULL);
        assert(local_edge_sendreq_tocloud_socket_client_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        GlobalGetRequest global_get_request(key);
        DynamicArray global_request_msg_payload(global_get_request.getMsgPayloadSize());
        global_get_request.serialize(global_request_msg_payload);
        Value tmp_value();
        while (true) // Timeout-and-retry
        {
            // Send the message payload of global request to cloud
            local_edge_sendreq_tocloud_socket_client_ptr_->send(global_request_msg_payload);

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload();
            bool is_timeout = local_edge_sendreq_tocloud_socket_client_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!local_edge_param_ptr_->isEdgeRunning())
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
                assert(global_response_ptr != NULL && global_response_ptr->getMessageType() == MessageType::kGlobalGetResponse);
                
                // Get value from global response message
                const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(global_response_ptr);
                value = global_get_response_ptr->getValue();

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop
        
        return is_finish;
    }

    void EdgeWrapper::triggerIndependentAdmission_(const Key& key, const Value& value)
    {
        // NOTE: COVERED must NOT trigger any independent admission
        assert(Param::getCacheName != CacheWrapperBase::COVERED_CACHE_NAME);

        // Independently admit the new key-value pair into local edge cache
        local_edge_cache_ptr->admit(key, value);

        // Evict until cache size <= cache capacity
        uint32_t cache_size = 0;
        uint32_t cache_capacity = local_edge_cache_ptr->getCapacityBytes();
        while (true)
        {
            current_cache_size = local_edge_cache_ptr->getSize();
            if (current_cache_size <= cache_capacity) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                local_edge_cache_ptr->evict();
                continue;
            }
        }

        return;
    }
}
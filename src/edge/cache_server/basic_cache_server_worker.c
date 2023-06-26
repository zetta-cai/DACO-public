#include "edge/cache_server/basic_cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCacheServerWorker::kClassName("BasicCacheServerWorker");

    BasicCacheServerWorker::BasicCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        assert(cache_server_worker_param_ptr->getEdgeWrapperPtr()->cache_name_ != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = cache_server_worker_param_ptr->getEdgeWrapperPtr()->edge_param_ptr_->getEdgeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate BasicCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    BasicCacheServerWorker::~BasicCacheServerWorker() {}

    // (1) Process data requests

    bool BasicCacheServerWorker::processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const
    {
        // Get key and value from redirected request if any
        assert(redirected_request_ptr != NULL && redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        Value tmp_value;

        // Acquire a read lock for serializability before accessing any shared variable in the target edge node
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock_shared(tmp_key, "processRedirectedGetRequest_()"))
            {
                break;
            }
        }

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;
        Hitflag hitflag = Hitflag::kGlobalMiss;

        // Access local edge cache for cooperative edge caching (current edge node is the target edge node)
        bool is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->edge_cache_ptr_->get(tmp_key, tmp_value);
        bool is_cooperaitve_cached = tmp_edge_wrapper_ptr->edge_cache_ptr_->isLocalCached(tmp_key);
        if (is_cooperative_cached_and_valid) // cached and valid
        {
            hitflag = Hitflag::kCooperativeHit;
        }
        else // not cached or invalid
        {
            if (is_cooperaitve_cached) // cached and invalid
            {
                hitflag = Hitflag::kCooperativeInvalid;
            }
        }

        // NOTE: no need to perform recursive cooperative edge caching (current edge node is already the target edge node for cooperative edge caching)
        // NOTE: no need to access cloud to get data, which will be performed by the closest edge node

        // Prepare RedirectedGetResponse for the closest edge node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        RedirectedGetResponse redirected_get_response(tmp_key, tmp_value, hitflag, edge_idx);

        // Reply redirected response message to the closest edge node (the remote address set by the most recent recv)
        DynamicArray redirected_response_msg_payload(redirected_get_response.getMsgPayloadSize());
        redirected_get_response.serialize(redirected_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_cache_server_recvreq_socket_server_ptr_->send(redirected_response_msg_payload);

        perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
        return is_finish;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool BasicCacheServerWorker::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare directory lookup request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        DirectoryLookupRequest directory_lookup_request(key, edge_idx);
        DynamicArray control_request_msg_payload(directory_lookup_request.getMsgPayloadSize());
        directory_lookup_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryLookupResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kDirectoryLookupResponse);

                // Get directory information from the control response message
                const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
                is_being_written = directory_lookup_response_ptr->isBeingWritten();
                is_valid_directory_exist = directory_lookup_response_ptr->isValidDirectoryExist();
                directory_info = directory_lookup_response_ptr->getDirectoryInfo();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    bool BasicCacheServerWorker::redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Prepare redirected get request to get data from target edge node if any
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        RedirectedGetRequest redirected_get_request(key, edge_idx);
        DynamicArray redirected_request_msg_payload(redirected_get_request.getMsgPayloadSize());
        redirected_get_request.serialize(redirected_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the redirected request to the target edge node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_sendreq_totarget_socket_client_ptr_->send(redirected_request_msg_payload);

            // Receive the control repsonse from the target node
            DynamicArray redirected_response_msg_payload;
            bool is_timeout = edge_cache_server_sendreq_totarget_socket_client_ptr_->recv(redirected_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for RedirectedGetResponse");
                    continue; // Resend the redirected request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the redirected response message successfully
                MessageBase* redirected_response_ptr = MessageBase::getResponseFromMsgPayload(redirected_response_msg_payload);
                assert(redirected_response_ptr != NULL && redirected_response_ptr->getMessageType() == MessageType::kRedirectedGetResponse);

                // Get value from redirected response message
                const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(redirected_response_ptr);
                value = redirected_get_response_ptr->getValue();
                Hitflag hitflag = redirected_get_response_ptr->getHitflag();
                if (hitflag == Hitflag::kCooperativeHit)
                {
                    is_cooperative_cached = true;
                    is_valid = true;
                }
                else if (hitflag == Hitflag::kCooperativeInvalid)
                {
                    is_cooperative_cached = true;
                    is_valid = false;
                }
                else if (hitflag == Hitflag::kGlobalMiss)
                {
                    std::ostringstream oss;
                    oss << "target edge node does not cache the key " << key.getKeystr() << " in redirectGetToTarget_()!";
                    Util::dumpWarnMsg(instance_name_, oss.str());

                    is_cooperative_cached = false;
                    is_valid = false;
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid hitflag " << MessageBase::hitflagToString(hitflag) << " for redirectGetToTarget_()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release the redirected response message
                delete redirected_response_ptr;
                redirected_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    // (2.2) Update content directory information

    bool BasicCacheServerWorker::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare directory update request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        DirectoryUpdateRequest directory_update_request(key, is_admit, directory_info, edge_idx);
        DynamicArray control_request_msg_payload(directory_update_request.getMsgPayloadSize());
        directory_update_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryUpdateResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

                // Get is_being_written from control response message
                const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = directory_update_response_ptr->isBeingWritten();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    // (2.3) Process writes and block for MSI protocol

    bool BasicCacheServerWorker::acquireBeaconWritelock_(const Key& key, LockResult& lock_result)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare acquire writelock request to acquire permission for a write
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        AcquireWritelockRequest acquire_writelock_request(key, edge_idx);
        DynamicArray control_request_msg_payload(acquire_writelock_request.getMsgPayloadSize());
        acquire_writelock_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for AcquireWritelockResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kAcquireWritelockResponse);

                // Get lock result from control response
                const AcquireWritelockResponse* const acquire_writelock_response_ptr = static_cast<const AcquireWritelockResponse*>(control_response_ptr);
                lock_result = acquire_writelock_response_ptr->getLockResult();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    bool BasicCacheServerWorker::releaseBeaconWritelock_(const Key& key)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare release writelock request to finish write
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        ReleaseWritelockRequest release_writelock_request(key, edge_idx);
        DynamicArray control_request_msg_payload(release_writelock_request.getMsgPayloadSize());
        release_writelock_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for ReleaseWritelockResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kReleaseWritelockResponse);

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    // (5) Admit uncached objects in local edge cache

    bool BasicCacheServerWorker::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_() and processLocalWriteRequest_()

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "admit key: " << key.getKeystr() << "; value: " << (value.isDeleted()?"true":"false") << "; //valuesize: " << value.getValuesize();
        //Util::dumpDebugMsg(instance_name_, oss.str());

        // Independently admit the new key-value pair into local edge cache
        bool is_being_written = false;
        is_finish = updateDirectory_(key, true, is_being_written);
        if (is_finish)
        {
            return is_finish;
        }
        tmp_edge_wrapper_ptr->edge_cache_ptr_->admit(key, value, !is_being_written); // valid if not being written

        // Evict until used bytes <= capacity bytes
        while (true)
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint32_t used_bytes = tmp_edge_wrapper_ptr->getSizeForCapacity_();
            if (used_bytes <= tmp_edge_wrapper_ptr->capacity_bytes_) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                Key victim_key;
                Value victim_value;
                tmp_edge_wrapper_ptr->edge_cache_ptr_->evict(victim_key, victim_value);
                bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
                is_finish = updateDirectory_(victim_key, false, _unused_is_being_written);
                if (is_finish)
                {
                    return is_finish;
                }

                // TMPDEBUG
                //oss.clear();
                //oss.str("");
                //oss << "evict key: " << victim_key.getKeystr() << "; value: " << (victim_value.isDeleted()?"true":"false") << "; valuesize: " << victim_value.getValuesize();
                //Util::dumpDebugMsg(instance_name_, oss.str());

                continue;
            }
        }

        return is_finish;
    }
}
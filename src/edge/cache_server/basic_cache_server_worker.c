#include "edge/cache_server/basic_cache_server_worker.h"

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
        EdgeWrapper* tmp_edgewrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_;
        assert(tmp_edgewrapper_ptr->cache_name_ != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edgewrapper_ptr->edge_param_ptr_->getNodeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate BasicCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    BasicCacheServerWorker::~BasicCacheServerWorker() {}

    // (1) Process data requests

    bool BasicCacheServerWorker::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        // Get key and value from redirected request if any
        assert(redirected_request_ptr != NULL && redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        assert(recvrsp_dst_addr.isValidAddr());
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        Value tmp_value;

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

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
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_cache_server_recvreq_source_addr_;
        MessageBase* redirected_get_response_ptr = new RedirectedGetResponse(tmp_key, tmp_value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr);

        // Push the redirected response message into edge-to-client propagation simulator to cache server worker in the closest edge node
        tmp_edge_wrapper_ptr->edge_toclient_propagation_simulator_param_ptr_->push(redirected_get_response_ptr, recvrsp_dst_addr);

        // NOTE: redirected_get_response_ptr will be released by edge-to-client propagation simulator

        return is_finish;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool BasicCacheServerWorker::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare directory lookup request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        MessageBase* directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
        assert(directory_lookup_request_ptr != NULL);

        // Get destination address of beacon node
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = getBeaconDstaddr_(key);

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(instance_name_, 4, "beacon edge index:", std::to_string(tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->getBeaconEdgeIdx(key)).c_str(), "keystr:", key.getKeystr().c_str());
        #endif

        while (true) // Timeout-and-retry mechanism
        {
            // Push the control request into edge-to-edge propagation simulator to send to beacon node
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(directory_lookup_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
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

        // NOTE: directory_lookup_request_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }

    bool BasicCacheServerWorker::redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;

        // Prepare redirected get request to get data from target edge node if any
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        MessageBase* redirected_get_request_ptr = new RedirectedGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
        assert(redirected_get_request_ptr != NULL);

        // Prepare destination address of target edge cache server
        NetworkAddr target_edge_cache_server_recvreq_dst_addr = getTargetDstaddr_(directory_info);

        while (true) // Timeout-and-retry mechanism
        {
            // Push the redirected data request into edge-to-edge propagation simulator to target node
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(redirected_get_request_ptr, target_edge_cache_server_recvreq_dst_addr);
            assert(is_successful);

            // Receive the redirected data repsonse from the target node
            DynamicArray redirected_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(redirected_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
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

        // NOTE: redirected_get_request_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }

    // (2.2) Update content directory information

    bool BasicCacheServerWorker::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare directory update request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        MessageBase* directory_update_request_ptr = new DirectoryUpdateRequest(key, is_admit, directory_info, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
        assert(directory_update_request_ptr != NULL);

        // Get destination address of beacon node
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(directory_update_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
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

        // NOTE: directory_update_request_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }

    // (2.3) Process writes and block for MSI protocol

    bool BasicCacheServerWorker::acquireBeaconWritelock_(const Key& key, LockResult& lock_result)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare acquire writelock request to acquire permission for a write
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        MessageBase* acquire_writelock_request_ptr = new AcquireWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
        assert(acquire_writelock_request_ptr != NULL);

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(acquire_writelock_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
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

        // NOTE: acquire_writelock_request_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }

    bool BasicCacheServerWorker::releaseBeaconWritelock_(const Key& key)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Prepare release writelock request to finish write
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        MessageBase* release_writelock_request_ptr = new ReleaseWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
        assert(release_writelock_request_ptr != NULL);

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(release_writelock_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
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

        // NOTE: release_writelock_request_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }

    // (5) Admit uncached objects in local edge cache

    bool BasicCacheServerWorker::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(instance_name_, 7, "independent admission;", "keystr:", key.getKeystr().c_str(), "is value deleted:", Util::toString(value.isDeleted()).c_str(), "value size:", Util::toString(value.getValuesize()).c_str());
        #endif

        // Independently admit the new key-value pair into local edge cache
        bool is_being_written = false;
        is_finish = updateDirectory_(key, true, is_being_written);
        if (is_finish)
        {
            return is_finish;
        }
        tmp_edge_wrapper_ptr->edge_cache_ptr_->admit(key, value, !is_being_written); // valid if not being written

        while (true) // Evict until used bytes <= capacity bytes
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

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(instance_name_, 7, "eviction;", "keystr:", victim_key.getKeystr().c_str(), "is value deleted:", Util::toString(victim_value.isDeleted()).c_str(), "value size:", Util::toString(victim_value.getValuesize()).c_str());
                #endif

                continue;
            }
        }

        return is_finish;
    }
}
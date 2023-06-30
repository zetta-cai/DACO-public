#include "edge/cache_server/cache_server_worker_base.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/cache_server/basic_cache_server_worker.h"
#include "edge/cache_server/covered_cache_server_worker.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CacheServerWorkerBase::kClassName("CacheServerWorkerBase");

    CacheServerWorkerBase* CacheServerWorkerBase::getCacheServerWorkerByCacheName(CacheServerWorkerParam* cache_server_worker_param_ptr)
    {
        CacheServerWorkerBase* cache_server_worker_ptr = NULL;

        assert(cache_server_worker_param_ptr != NULL);
        std::string cache_name = cache_server_worker_param_ptr->getEdgeWrapperPtr()->cache_name_;
        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            cache_server_worker_ptr = new CoveredCacheServerWorker(cache_server_worker_param_ptr);
        }
        else
        {
            cache_server_worker_ptr = new BasicCacheServerWorker(cache_server_worker_param_ptr);
        }

        assert(cache_server_worker_ptr != NULL);
        return cache_server_worker_ptr;
    }

    CacheServerWorkerBase::CacheServerWorkerBase(CacheServerWorkerParam* cache_server_worker_param_ptr) : cache_server_worker_param_ptr_(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr_ != NULL);
        uint32_t edge_idx = cache_server_worker_param_ptr_->getEdgeWrapperPtr()->edge_param_ptr_->getEdgeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();
        base_instance_name_ = oss.str();

        // Prepare a socket client to cloud recvreq port for cache server worker
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        NetworkAddr remote_addr(cloud_ipstr, cloud_recvreq_port);
        edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, remote_addr);
        assert(edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ != NULL);

        // NOTE: we use beacon server of edge0 as default remote address, but will reset remote address of the socket clients based on the key and directory information later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_beacon_server_port = Util::getEdgeBeaconServerRecvreqPort(0);
        NetworkAddr edge0_beacon_server_addr(edge0_ipstr, edge0_beacon_server_port);
        uint16_t edge0_cache_server_port = Util::getEdgeCacheServerRecvreqPort(0);
        NetworkAddr edge0_cache_server_addr(edge0_ipstr, edge0_cache_server_port);
        uint16_t edge0_invalidation_server_port = Util::getEdgeInvalidationServerRecvreqPort(0);
        NetworkAddr edge0_invalidation_server_addr(edge0_ipstr, edge0_invalidation_server_port);

        // Prepare a socket client to beacon edge node for cache server worker
        edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_  = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_beacon_server_addr);
        assert(edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_  != NULL);

        // Prepare a socket client to target edge node for cache server worker
        edge_cache_server_worker_sendreq_totarget_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_cache_server_addr);
        assert(edge_cache_server_worker_sendreq_totarget_socket_client_ptr_ != NULL);

        // Prepare a socket client to client/neighbor for cache server worker
        // NOTE: we use an invalid source address as default remote address, but will reset remote address of the socket client based on network address popped from ring buffer later
        std::string invalid_source_ipstr = Util::LOCALHOST_IPSTR;
        uint16_t invalid_source_port = Util::UDP_MIN_PORT + 1;
        NetworkAddr invalid_source_addr(invalid_source_ipstr, invalid_source_port);
        edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_ = new UdpSocketWrapper(SocketRole
        ::kSocketClient, invalid_source_addr);
        assert(edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_);
    }

    CacheServerWorkerBase::~CacheServerWorkerBase()
    {
        // NOTE: no need to release cache_server_worker_param_ptr_, which will be released outside CacheServerWorkerBase (by CacheServer)

        // Release the socket client to cloud
        assert(edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ != NULL);
        delete edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_;
        edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ = NULL;

        // Release the socket client to beacon node
        assert(edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ != NULL);
        delete edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ ;
        edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ = NULL;

        // Release the socket client to target ndoe
        assert(edge_cache_server_worker_sendreq_totarget_socket_client_ptr_ != NULL);
        delete edge_cache_server_worker_sendreq_totarget_socket_client_ptr_;
        edge_cache_server_worker_sendreq_totarget_socket_client_ptr_ = NULL;

        // Release the socket client to client/neighbor
        assert(edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_ != NULL);
        delete edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_;
        edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_ = NULL;
    }

    void CacheServerWorkerBase::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data request from ring buffer partitioned by cache server
            CacheServerWorkerItem tmp_cache_server_worker_item;
            bool is_successful = cache_server_worker_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_worker_item);
            if (is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_worker_item.getDataRequestPtr();
                assert(data_request_ptr != NULL);
                NetworkAddr network_addr = tmp_cache_server_worker_item.getNetworkAddr();
                assert(network_addr.isValidAddr());

                if (data_request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    is_finish = processDataRequest_(data_request_ptr, network_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the corresponding cache server worker thread
                assert(data_request_ptr != NULL);
                delete data_request_ptr;
                data_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    // (1) Process data requests

    bool CacheServerWorkerBase::processDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& network_addr)
    {
        assert(data_request_ptr != NULL && data_request_ptr->isDataRequest());
        assert(network_addr.isValidAddr());

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
                is_finish = processLocalGetRequest_(data_request_ptr, network_addr);
            }
            else // Local put/del request
            {
                is_finish = processLocalWriteRequest_(data_request_ptr, network_addr);
            }
        }
        else if (data_request_ptr->isRedirectedRequest()) // Redirected request
        {
            is_finish = processRedirectedRequest_(data_request_ptr, network_addr);
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

    bool CacheServerWorkerBase::processLocalGetRequest_(MessageBase* local_request_ptr, const NetworkAddr& network_addr) const
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL && local_request_ptr->getMessageType() == MessageType::kLocalGetRequest);
        assert(network_addr.isValidAddr());
        const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(local_request_ptr);
        Key tmp_key = local_get_request_ptr->getKey();
        Value tmp_value;

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        Hitflag hitflag = Hitflag::kGlobalMiss;

        // Access local edge cache (current edge node is the closest edge node)
        bool is_local_cached_and_valid = tmp_edge_wrapper_ptr->edge_cache_ptr_->get(tmp_key, tmp_value);
        if (is_local_cached_and_valid) // local cached and valid
        {
            hitflag = Hitflag::kLocalHit;
        }

        // Access cooperative edge cache for local cache miss or invalid object
        bool is_cooperative_cached_and_valid = false;
        if (!is_local_cached_and_valid) // not local cached or invalid
        {
            // Get data from some target edge node for local cache miss
            is_finish = fetchDataFromNeighbor_(tmp_key, tmp_value, is_cooperative_cached_and_valid);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
            if (is_cooperative_cached_and_valid) // cooperative cached and valid
            {
                hitflag = Hitflag::kCooperativeHit;
            }
        }

        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision

        // Get data from cloud for global cache miss
        if (!is_local_cached_and_valid && !is_cooperative_cached_and_valid) // (not cached or invalid) in both local and cooperative cache
        {
            is_finish = fetchDataFromCloud_(tmp_key, tmp_value);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
        }

        // Update invalid object of local edge cache if necessary
        is_finish = tryToUpdateInvalidLocalEdgeCache_(tmp_key, tmp_value);
        if (is_finish)
        {
            return is_finish;
        }

        // Trigger independent cache admission for local/global cache miss if necessary
        is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value);
        if (is_finish)
        {
            return is_finish;
        }

        // Prepare LocalGetResponse and set remote address for client
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        LocalGetResponse local_get_response(tmp_key, tmp_value, hitflag, edge_idx);
        edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_->setRemoteAddrForClient(network_addr);

        // Reply local response message to a client (the remote address set by the most recent recv)
        DynamicArray local_response_msg_payload(local_get_response.getMsgPayloadSize());
        local_get_response.serialize(local_response_msg_payload);
        PropagationSimulator::propagateFromEdgeToClient();
        edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_->send(local_response_msg_payload);

        // TMPDEBUG
        std::ostringstream oss;
        oss << "issue a local response; type: " << MessageBase::messageTypeToString(local_get_response.getMessageType()) << "; keystr:" << local_get_response.getKey().getKeystr();
        Util::dumpDebugMsg(base_instance_name_, oss.str());

        return is_finish;
    }

    bool CacheServerWorkerBase::processLocalWriteRequest_(MessageBase* local_request_ptr, const NetworkAddr& network_addr)
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL);
        assert(local_request_ptr->getMessageType() == MessageType::kLocalPutRequest || local_request_ptr->getMessageType() == MessageType::kLocalDelRequest);
        assert(network_addr.isValidAddr());
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
        
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        const Hitflag hitflag = Hitflag::kGlobalMiss; // Must be global miss due to write-through policy

        // Acquire write lock from beacon node no matter the locally cached object is valid or not, where the beacon will invalidate all other cache copies for cache coherence
        LockResult lock_result = LockResult::kFailure;
        is_finish = acquireWritelock_(tmp_key, lock_result);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        else
        {
            assert(lock_result == LockResult::kSuccess || lock_result == LockResult::kNoneed);
        }

        // Send request to cloud for write-through policy
        is_finish = writeDataToCloud_(tmp_key, tmp_value, local_request_ptr->getMessageType());
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }

        // Try to update/remove local edge cache
        bool is_local_cached = false;
        MessageBase* local_response_ptr = NULL;
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        // NOTE: message type has been checked, which must be one of the following two types
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            is_finish = updateLocalEdgeCache_(tmp_key, tmp_value, is_local_cached);

            // Prepare LocalPutResponse for client
            local_response_ptr = new LocalPutResponse(tmp_key, hitflag, edge_idx);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            removeLocalEdgeCache_(tmp_key, is_local_cached);

            // Prepare LocalDelResponse for client
            local_response_ptr = new LocalDelResponse(tmp_key, hitflag, edge_idx);
        }
        UNUSED(is_local_cached);

        // Trigger independent cache admission for local/global cache miss if necessary
        if (!is_finish)
        {
            is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value);
        }

        if (!is_finish && lock_result == LockResult::kSuccess)
        {
            // Notify beacon node to finish writes if acquiring write lock successfully
            is_finish = releaseWritelock_(tmp_key);
        }

        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision

        if (!is_finish) // // Edge node is STILL running
        {
            // Set remote address for client
            edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_->setRemoteAddrForClient(network_addr);

            // Reply local response message to a client (the remote address set by the most recent recv)
            assert(local_response_ptr != NULL);
            assert(local_response_ptr->getMessageType() == MessageType::kLocalPutResponse || local_response_ptr->getMessageType() == MessageType::kLocalDelResponse);
            DynamicArray local_response_msg_payload(local_response_ptr->getMsgPayloadSize());
            local_response_ptr->serialize(local_response_msg_payload);
            PropagationSimulator::propagateFromEdgeToClient();
            edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_->send(local_response_msg_payload);
        }

        // Release response message
        assert(local_response_ptr != NULL);
        delete local_response_ptr;
        local_response_ptr = NULL;

        return is_finish;
    }

    bool CacheServerWorkerBase::processRedirectedRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& network_addr)
    {
        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedRequest());

        bool is_finish = false; // Mark if local edge node is finished

        MessageType message_type = redirected_request_ptr->getMessageType();
        if (message_type == MessageType::kRedirectedGetRequest)
        {
            is_finish = processRedirectedGetRequest_(redirected_request_ptr, network_addr);
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

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool CacheServerWorkerBase::fetchDataFromNeighbor_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        if (!current_is_beacon)
        {
            locateBeaconNode_(key);
        }

        // Check if beacon node is the current edge node and lookup directory information
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        while (true)
        {
            if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            is_being_written = false;
            is_valid_directory_exist = false;
            if (current_is_beacon) // Get target edge index from local directory information
            {
                tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->lookupLocalDirectoryByCacheServer(key, is_being_written, is_valid_directory_exist, directory_info);
                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by polling
                    // TODO: sleep a short time to avoid frequent polling
                    continue; // Continue to lookup local directory info
                }
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                is_finish = lookupBeaconDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate DirectoryLookupRequest
                    is_finish = blockForWritesByInterruption_(key);
                    if (is_finish) // Edge is NOT running
                    {
                        return is_finish;
                    }
                    else
                    {
                        continue; // Continue to lookup remote directory info
                    }
                }
            } // End of current_is_beacon

            // key must NOT being written here
            assert(!is_being_written);

            if (is_valid_directory_exist) // The object is cached by some target edge node
            {
                // NOTE: the target node should not be the current edge node, as CooperationWrapperBase::get() can only be invoked if is_local_cached = false (i.e., the current edge node does not cache the object and hence is_valid_directory_exist should be false)
                bool current_is_target = tmp_edge_wrapper_ptr->currentIsTarget_(directory_info);
                if (current_is_target)
                {
                    std::ostringstream oss;
                    oss << "current edge node " << directory_info.getTargetEdgeIdx() << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                    return is_finish; // NOTE: is_finish is still false, as edge is STILL running
                }

                // Update remote address of sendreq_totarget_socket_client_ptr as the target edge node
                locateTargetNode_(directory_info);

                // Get data from the target edge node if any and update is_cooperative_cached_and_valid
                bool is_cooperative_cached = false;
                bool is_valid = false;
                is_finish = redirectGetToTarget_(key, value, is_cooperative_cached, is_valid);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                // Check is_cooperative_cached and is_valid
                if (!is_cooperative_cached) // Target edge node does not cache the object
                {
                    assert(!is_valid);
                    is_cooperative_cached_and_valid = false; // Closest edge node will fetch data from cloud
                    break;
                }
                else if (is_valid) // Target edge node caches a valid object
                {
                    is_cooperative_cached_and_valid = true; // Closest edge node will directly reply clients
                    break;
                }
                else // Target edge node caches an invalid object
                {
                    continue; // Go back to look up local/remote directory info again
                }
            }
            else // The object is not cached by any target edge node
            {
                is_cooperative_cached_and_valid = false;
                break;
            }
        } // End of while (true)

        return is_finish;
    }

    // (2.2) Update content directory information

    bool CacheServerWorkerBase::updateDirectory_(const Key& key, const bool& is_admit, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        if (!current_is_beacon)
        {
            locateBeaconNode_(key);
        }

        // Check if beacon node is the current edge node and update directory information
        DirectoryInfo directory_info(tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx());
        if (current_is_beacon) // Update target edge index of local directory information
        {
            tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->updateLocalDirectory(key, is_admit, directory_info, is_being_written);
        }
        else // Update remote directory information at the beacon node
        {
            is_finish = updateBeaconDirectory_(key, is_admit, directory_info, is_being_written);
        }

        return is_finish;
    }

    // (2.3) Process writes and block for MSI protocol

    bool CacheServerWorkerBase::acquireWritelock_(const Key& key, LockResult& lock_result)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        if (!current_is_beacon)
        {
            locateBeaconNode_(key);
        }

        // Check if beacon node is the current edge node and acquire write permission
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> all_dirinfo;
        while (true)
        {
            if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            if (current_is_beacon) // Get target edge index from local directory information
            {
                lock_result = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->acquireLocalWritelockByCacheServer(key, all_dirinfo);
                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    // Wait for writes by polling
                    // TODO: sleep a short time to avoid frequent polling
                    continue; // Continue to try to acquire the write lock
                }
                else if (lock_result == LockResult::kSuccess) // If acquire write permission successfully
                {
                    // Invalidate all cache copies
                    tmp_edge_wrapper_ptr->invalidateCacheCopies_(key, all_dirinfo);
                }
                // NOTE: will directly break if lock result is kNoneed
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                is_finish = acquireBeaconWritelock_(key, lock_result);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate AcquireWritelockRequest
                    is_finish = blockForWritesByInterruption_(key);
                    if (is_finish) // Edge is NOT running
                    {
                        return is_finish;
                    }
                    else
                    {
                        continue; // Continue to try to acquire the write lock
                    }
                }
                // NOTE: If lock_result == kSuccess, beacon server of beacon node has already invalidated all cache copies -> NO need to invalidate them again in cache server
                // NOTE: will directly break if lock result is kNoneed
            } // End of current_is_beacon

            assert(lock_result == LockResult::kSuccess || lock_result == LockResult::kNoneed);
            break;
        } // End of while (true)

        return is_finish;
    }

    bool CacheServerWorkerBase::blockForWritesByInterruption_(const Key& key) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // Closest edge node must NOT be the beacon node if with interruption
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;

        // Wait for FinishBlockRequest from beacon node
        while (true)
        {
            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning()) // Edge is not running
                {
                    is_finish = true;
                    break;
                }
                else // Retry to receive a message if edge is still running
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for FinishBlockRequest");
                    continue;
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kFinishBlockRequest);

                // Get key from FinishBlockRequest
                const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_response_ptr);
                Key tmp_key = finish_block_request_ptr->getKey();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;

                // Double-check if key matches
                if (key == tmp_key) // key matches
                {
                    break; // Break the while loop to send back FinishBlockResponse
                    
                }
                else // key NOT match
                {
                    std::ostringstream oss;
                    oss << "wait for key " << key.getKeystr() << " != received key " << tmp_key.getKeystr();
                    Util::dumpWarnMsg(base_instance_name_, oss.str());

                    continue; // Receive the next FinishBlockRequest
                }
            } // End of is_timeout
        } // End of while(true)

        if (!is_finish)
        {
            // Prepare FinishBlockResponse
            uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
            FinishBlockResponse finish_block_response(key, edge_idx);
            DynamicArray control_request_msg_payload(finish_block_response.getMsgPayloadSize());
            finish_block_response.serialize(control_request_msg_payload);

            // Send back FinishBlockResponse
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);
        }

        return is_finish;
    }

    bool CacheServerWorkerBase::releaseWritelock_(const Key& key)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        if (!current_is_beacon)
        {
            locateBeaconNode_(key);
        }

        // Check if beacon node is the current edge node and lookup directory information
    
        if (current_is_beacon) // Get target edge index from local directory information
        {
            DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx());
            std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->releaseLocalWritelock(key, current_directory_info);

            is_finish = tmp_edge_wrapper_ptr->notifyEdgesToFinishBlock_(key, blocked_edges);
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        }
        else // Get target edge index from remote directory information at the beacon node
        {
            // NOTE: beacon server of beacon node has already notified all blocked edges -> NO need to notify them again in cache server
            is_finish = releaseBeaconWritelock_(key);
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        } // End of current_is_beacon

        return is_finish;
    }

    // (2.4) Utility functions for cooperative caching

    void CacheServerWorkerBase::locateBeaconNode_(const Key& key) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        // Set remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_addr = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerAddr(key);
        edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_->setRemoteAddrForClient(beacon_edge_beacon_server_addr);

        return;
    }

    void CacheServerWorkerBase::locateTargetNode_(const DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        // The current edge node must NOT be the target node
        bool current_is_target = tmp_edge_wrapper_ptr->currentIsTarget_(directory_info);
        assert(!current_is_target);

        // Set remote address such that the current edge node can communicate with the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx);
        uint16_t target_edge_cache_server_port = Util::getEdgeCacheServerRecvreqPort(target_edge_idx);
        NetworkAddr target_edge_cache_server_addr(target_edge_ipstr, target_edge_cache_server_port);
        edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_->setRemoteAddrForClient(target_edge_cache_server_addr);
        
        return;
    }

    // (3) Access cloud

    bool CacheServerWorkerBase::fetchDataFromCloud_(const Key& key, Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished

        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        GlobalGetRequest global_get_request(key, edge_idx);
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
            edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_->send(global_request_msg_payload);

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
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

    bool CacheServerWorkerBase::writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type)
    {        
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished

        // Prepare global write request message
        MessageBase* global_request_ptr = NULL;
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        if (message_type == MessageType::kGlobalPutRequest)
        {
            global_request_ptr = new GlobalPutRequest(key, value, edge_idx);
        }
        else if (message_type == MessageType::kGlobalDelRequest)
        {
            global_request_ptr = new GlobalDelRequest(key, edge_idx);
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
            edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_->send(global_request_msg_payload);

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for GlobalPutResponse/GlobalDelResponse");
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

    // (4) Update cached objects in local edge cache

    bool CacheServerWorkerBase::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;
        
        bool is_local_cached = tmp_edge_wrapper_ptr->edge_cache_ptr_->isLocalCached(key);
        bool is_cached_and_invalid = false;
        if (is_local_cached)
        {
            bool is_valid = tmp_edge_wrapper_ptr->edge_cache_ptr_->isValidKeyForLocalCachedObject(key);
            is_cached_and_invalid = is_local_cached && !is_valid;
        }

        if (is_cached_and_invalid)
        {
            bool is_local_cached_after_udpate = false;
            if (value.isDeleted()) // value is deleted
            {
                removeLocalEdgeCache_(key, is_local_cached_after_udpate);
            }
            else // non-deleted value
            {
                is_finish = updateLocalEdgeCache_(key, value, is_local_cached_after_udpate);
            }
            assert(is_local_cached_after_udpate);
        }

        return is_finish;
    }

    bool CacheServerWorkerBase::updateLocalEdgeCache_(const Key& key, const Value& value, bool& is_local_cached_after_udpate) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_() or processLocalWriteRequest_()

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        is_local_cached_after_udpate = tmp_edge_wrapper_ptr->edge_cache_ptr_->update(key, value);

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

                continue;
            }
        }

        return is_finish;
    }

    void CacheServerWorkerBase::removeLocalEdgeCache_(const Key& key, bool& is_local_cached_after_udpate) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        is_local_cached_after_udpate = tmp_edge_wrapper_ptr->edge_cache_ptr_->remove(key);

        // NOTE: no need to check capacity, as remove() only replaces the original value (value size + is_deleted) with a deleted value (zero value size + is_deleted of true), where deleted value uses minimum bytes and remove() cannot increase used bytes to trigger any eviction

        return;
    }

    // (5) Admit uncached objects in local edge cache

    bool CacheServerWorkerBase::tryToTriggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        bool is_local_cached = tmp_edge_wrapper_ptr->edge_cache_ptr_->isLocalCached(key);
        if (!is_local_cached && tmp_edge_wrapper_ptr->edge_cache_ptr_->needIndependentAdmit(key))
        {
            is_finish = triggerIndependentAdmission_(key, value);
        }

        return is_finish;
    }

    // (6) Utility functions

    void CacheServerWorkerBase::checkPointers_() const
    {
        assert(cache_server_worker_param_ptr_ != NULL);
        assert(cache_server_worker_param_ptr_->getEdgeWrapperPtr() != NULL);
        assert(cache_server_worker_param_ptr_->getEdgeWrapperPtr()->edge_param_ptr_ != NULL);
        assert(cache_server_worker_param_ptr_->getEdgeWrapperPtr()->edge_cache_ptr_ != NULL);
        assert(cache_server_worker_param_ptr_->getEdgeWrapperPtr()->cooperation_wrapper_ptr_ != NULL);
        assert(cache_server_worker_param_ptr_->getDataRequestBufferPtr() != NULL);
        assert(edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ != NULL);
        assert(edge_cache_server_worker_sendreq_totarget_socket_client_ptr_ != NULL);
        assert(edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_ != NULL);
    }
}
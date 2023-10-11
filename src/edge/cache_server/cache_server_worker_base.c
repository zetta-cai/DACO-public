#include "edge/cache_server/cache_server_worker_base.h"

#include <assert.h>

#include "common/bandwidth_usage.h"
#include "common/config.h"
#include "common/util.h"
#include "edge/cache_server/basic_cache_server_worker.h"
#include "edge/cache_server/covered_cache_server_worker.h"
#include "event/event.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CacheServerWorkerBase::kClassName("CacheServerWorkerBase");

    void* CacheServerWorkerBase::launchCacheServerWorker(void* cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);

        CacheServerWorkerBase* cache_server_worker_ptr = getCacheServerWorkerByCacheName_((CacheServerWorkerParam*)cache_server_worker_param_ptr);
        assert(cache_server_worker_ptr != NULL);
        cache_server_worker_ptr->start();

        assert(cache_server_worker_ptr != NULL);
        delete cache_server_worker_ptr;
        cache_server_worker_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerWorkerBase* CacheServerWorkerBase::getCacheServerWorkerByCacheName_(CacheServerWorkerParam* cache_server_worker_param_ptr)
    {
        CacheServerWorkerBase* cache_server_worker_ptr = NULL;

        assert(cache_server_worker_param_ptr != NULL);
        std::string cache_name = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getCacheName();
        if (cache_name == Util::COVERED_CACHE_NAME)
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
        assert(cache_server_worker_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();
        const uint32_t edgecnt = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeCnt();
        const uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();
        const uint32_t percacheserver_workercnt = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getPercacheserverWorkercnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();
        base_instance_name_ = oss.str();

        // Prepare destination address to the corresponding cloud
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        corresponding_cloud_recvreq_dst_addr_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // For receiving control responses and redirected data responses

        // Get source address of cache server worker to receive control responses and redirected data responses
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_worker_recvrsp_port = Util::getEdgeCacheServerWorkerRecvrspPort(edge_idx, edgecnt, local_cache_server_worker_idx, percacheserver_workercnt);
        edge_cache_server_worker_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_worker_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_worker_recvrsp_port);
        edge_cache_server_worker_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);

        // For receiving finish block requests

        // Get source address of cache server worker to receive finish block requests
        uint16_t edge_cache_server_worker_recvreq_port = Util::getEdgeCacheServerWorkerRecvreqPort(edge_idx, edgecnt, local_cache_server_worker_idx, percacheserver_workercnt);
        edge_cache_server_worker_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_worker_recvreq_port);

        // Prepare a socket server to receive finish block requests
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, edge_cache_server_worker_recvreq_port);
        edge_cache_server_worker_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
    }

    CacheServerWorkerBase::~CacheServerWorkerBase()
    {
        // NOTE: no need to release cache_server_worker_param_ptr_, which will be released outside CacheServerWorkerBase (by CacheServer)

        // Release the socket server to receive control responses, redirected data responses, and global responses
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_worker_recvrsp_socket_server_ptr_;
        edge_cache_server_worker_recvrsp_socket_server_ptr_ = NULL;

        // Release the socket server to receive finish block requests
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_worker_recvreq_socket_server_ptr_;
        edge_cache_server_worker_recvreq_socket_server_ptr_ = NULL;
    }

    void CacheServerWorkerBase::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_worker_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // client worker or cache server worker
                    is_finish = processDataRequest_(data_request_ptr, recvrsp_dst_addr);
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

    bool CacheServerWorkerBase::processDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(data_request_ptr != NULL && data_request_ptr->isDataRequest());
        assert(recvrsp_dst_addr.isValidAddr());

        bool is_finish = false; // Mark if edge node is finished

        if (data_request_ptr->isLocalDataRequest()) // Local request
        {
            if (data_request_ptr->getMessageType() == MessageType::kLocalGetRequest)
            {
                is_finish = processLocalGetRequest_(data_request_ptr, recvrsp_dst_addr);
            }
            else // Local put/del request
            {
                is_finish = processLocalWriteRequest_(data_request_ptr, recvrsp_dst_addr);
            }
        }
        else if (data_request_ptr->isRedirectedDataRequest()) // Redirected request
        {
            is_finish = processRedirectedRequest_(data_request_ptr, recvrsp_dst_addr);
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

    // (1) Process read requests

    bool CacheServerWorkerBase::processLocalGetRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL && local_request_ptr->getMessageType() == MessageType::kLocalGetRequest);
        assert(recvrsp_dst_addr.isValidAddr());
        const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(local_request_ptr);
        Key tmp_key = local_get_request_ptr->getKey();
        Value tmp_value;
        const bool skip_propagation_latency = local_request_ptr->isSkipPropagationLatency();

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "receive a local get request;", "type:", MessageBase::messageTypeToString(local_request_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        Hitflag hitflag = Hitflag::kGlobalMiss;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received local get request
        uint32_t client_edge_local_req_bandwidth_bytes = local_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(client_edge_local_req_bandwidth_bytes, 0, 0));

        // Access local edge cache (current edge node is the closest edge node)
        struct timespec get_local_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_local_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, tmp_value);
        if (is_local_cached_and_valid) // local cached and valid
        {
            hitflag = Hitflag::kLocalHit;
        }
        struct timespec get_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t get_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_local_cache_end_timestamp, get_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_LOCAL_CACHE_EVENT_NAME, get_local_cache_latency_us); // Add intermediate event if with event tracking

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "acesss local edge cache;", "is_local_cached_and_valid:", Util::toString(is_local_cached_and_valid).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        // Access cooperative edge cache for local cache miss or invalid object
        bool is_cooperative_cached_and_valid = false;
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        if (!is_local_cached_and_valid) // not local cached or invalid
        {
            struct timespec get_cooperative_cache_start_timestamp = Util::getCurrentTimespec();

            // Get data from some target edge node for local cache miss (add events of intermediate responses if with event tracking)
            is_finish = fetchDataFromNeighbor_(tmp_key, tmp_value, is_cooperative_cached_and_valid, best_placement_edgeset, need_hybrid_fetching, total_bandwidth_usage, event_list, skip_propagation_latency);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
            if (is_cooperative_cached_and_valid) // cooperative cached and valid
            {
                hitflag = Hitflag::kCooperativeHit;
            }

            struct timespec get_cooperative_cache_end_timestamp = Util::getCurrentTimespec();
            uint32_t get_cooperative_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_cooperative_cache_end_timestamp, get_cooperative_cache_start_timestamp));
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_COOPERATIVE_CACHE_EVENT_NAME, get_cooperative_cache_latency_us); // Add intermediate event if with event tracking

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 5, "acesss cooperative edge cache;", "is_cooperative_cached_and_valid:", Util::toString(is_cooperative_cached_and_valid).c_str(), "keystr:", tmp_key.getKeystr().c_str());
            #endif
        }

        // Get data from cloud for global cache miss
        struct timespec get_cloud_start_timestamp = Util::getCurrentTimespec();
        if (!is_local_cached_and_valid && !is_cooperative_cached_and_valid) // (not cached or invalid) in both local and cooperative cache
        {
            is_finish = fetchDataFromCloud_(tmp_key, tmp_value, total_bandwidth_usage, event_list, skip_propagation_latency);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
        }
        struct timespec get_cloud_end_timestamp = Util::getCurrentTimespec();
        uint32_t get_cloud_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_cloud_end_timestamp, get_cloud_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_CLOUD_EVENT_NAME, get_cloud_latency_us); // Add intermediate event if with event tracking

        // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching (ONLY for COVERED)
        struct timespec trigger_placement_start_timestamp = Util::getCurrentTimespec();
        if (need_hybrid_fetching)
        {
            is_finish = tryToTriggerPlacementNotificationAfterHybridFetch_(tmp_key, tmp_value, best_placement_edgeset, total_bandwidth_usage, event_list, skip_propagation_latency);
            if (is_finish) // Edge is NOT running now
            {
                return is_finish;
            }
        }
        struct timespec trigger_placement_end_timestamp = Util::getCurrentTimespec();
        uint32_t trigger_placement_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(trigger_placement_end_timestamp, trigger_placement_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_TRIGGER_PLACEMENT_EVENT_NAME, trigger_placement_latency_us); // Add intermediate event if with event tracking

        // Update invalid object of local edge cache if necessary
        struct timespec update_invalid_local_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_local_cached_and_invalid = tryToUpdateInvalidLocalEdgeCache_(tmp_key, tmp_value);
        if (!tmp_value.isDeleted() && is_local_cached_and_invalid) // Update may trigger eviction
        {
            is_finish = tmp_cache_server_ptr->evictForCapacity_(edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate response if with event tracking
        }
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec update_invalid_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_invalid_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_invalid_local_cache_end_timestamp, update_invalid_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_UPDATE_INVALID_LOCAL_CACHE_EVENT_NAME, update_invalid_local_cache_latency_us); // Add intermediate event if with event tracking

        // Trigger independent cache admission for local/global cache miss if necessary
        // NOTE: For COVERED, beacon node will tell the edge node whether to admit or not, w/o independent decision
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate responses if with event tracking
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Prepare LocalGetResponse for client
        uint64_t used_bytes = tmp_edge_wrapper_ptr->getSizeForCapacity();
        uint64_t capacity_bytes = tmp_edge_wrapper_ptr->getCapacityBytes();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeCacheServerRecvreqSourceAddr();
        MessageBase* local_get_response_ptr = new LocalGetResponse(tmp_key, tmp_value, hitflag, used_bytes, capacity_bytes, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(local_get_response_ptr != NULL);

        // Push local response message into edge-to-client propagation simulator to a client
        bool is_successful = tmp_edge_wrapper_ptr->getEdgeToclientPropagationSimulatorParamPtr()->push(local_get_response_ptr, recvrsp_dst_addr);
        assert(is_successful);

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "issue a local response;", "type:", MessageBase::messageTypeToString(local_get_response_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        // NOTE: local_get_response_ptr will be released by edge-to-client propagation simulator
        local_get_response_ptr = NULL;

        return is_finish;
    }

    // (1.1) Access local edge cache

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    bool CacheServerWorkerBase::fetchDataFromNeighbor_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 4, "current_is_beacon:", Util::toString(current_is_beacon).c_str(), "keystr:", key.getKeystr().c_str());
        #endif

        // Check if beacon node is the current edge node and lookup directory information
        struct timespec lookup_directory_start_timestamp = Util::getCurrentTimespec();
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        need_hybrid_fetching = false;
        while (true) // Wait for valid directory after writes by polling or interruption
        {
            if (!tmp_edge_wrapper_ptr->isNodeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            is_being_written = false;
            is_valid_directory_exist = false;
            if (current_is_beacon) // Get target edge index from local directory information
            {
                // Frequent polling
                is_finish = lookupLocalDirectory_(key, is_being_written, is_valid_directory_exist, directory_info, best_placement_edgeset, need_hybrid_fetching, total_bandwidth_usage, event_list, skip_propagation_latency);
                if (is_finish)
                {
                    return is_finish; // Edge is NOT running
                }
                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    continue; // Continue to lookup local directory info
                }
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                bool need_lookup_beacon_directory = needLookupBeaconDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);
                if (need_lookup_beacon_directory)
                {
                    is_finish = lookupBeaconDirectory_(key, is_being_written, is_valid_directory_exist, directory_info, best_placement_edgeset, need_hybrid_fetching, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate responses if with event tracking
                }
                
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate DirectoryLookupRequest
                    is_finish = blockForWritesByInterruption_(key, event_list, skip_propagation_latency); // Add intermediate event if with event tracking
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

            // Add intermediate event if with event tracking
            // NOTE: too large latency to lookup local directory may indicate polling for writes, while too large latency to lookup remote directory may indicate blocking for writes
            struct timespec lookup_directory_end_timestamp = Util::getCurrentTimespec();
            uint32_t lookup_directory_latency_us = Util::getDeltaTimeUs(lookup_directory_end_timestamp, lookup_directory_start_timestamp);
            event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_REMOTE_DIRECTORY_EVENT_NAME, lookup_directory_latency_us);

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 4, "is_valid_directory_exist:", Util::toString(is_valid_directory_exist).c_str(), "keystr:", key.getKeystr().c_str());
            #endif

            if (is_valid_directory_exist) // The object is cached by some target edge node
            {
                struct timespec redirect_get_start_timestamp = Util::getCurrentTimespec();

                // NOTE: the target node should not be the current edge node, as CooperationWrapperBase::get() can only be invoked if is_local_cached = false (i.e., the current edge node does not cache the object and hence is_valid_directory_exist should be false)
                bool current_is_target = tmp_edge_wrapper_ptr->currentIsTarget(directory_info);
                if (current_is_target)
                {
                    std::ostringstream oss;
                    oss << "current edge node " << directory_info.getTargetEdgeIdx() << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                    return is_finish; // NOTE: is_finish is still false, as edge is STILL running
                }

                // Get data from the target edge node if any and update is_cooperative_cached_and_valid
                bool is_cooperative_cached = false;
                bool is_valid = false;
                is_finish = redirectGetToTarget_(directory_info, key, value, is_cooperative_cached, is_valid, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate responses if with event tracking
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                // Add intermediate event if with event tracking
                struct timespec redirect_get_end_timestamp = Util::getCurrentTimespec();
                uint32_t redirect_get_latency_us = Util::getDeltaTimeUs(redirect_get_end_timestamp, redirect_get_start_timestamp);
                event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_REDIRECT_GET_EVENT_NAME, redirect_get_latency_us);

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(base_instance_name_, 9, "issue redirected get request:", "target:", Util::toString(directory_info.getTargetEdgeIdx()).c_str(), "is_cooperative_cached:", Util::toString(is_cooperative_cached).c_str(), "is_valid", Util::toString(is_valid).c_str(), "keystr:", key.getKeystr().c_str());
                #endif

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
                    lookup_directory_start_timestamp = Util::getCurrentTimespec(); // Reset start timestamp for the next round
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

    bool CacheServerWorkerBase::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec issue_directory_lookup_req_start_timestamp = Util::getCurrentTimespec();

        // Get destination address of beacon node
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            MessageBase* directory_lookup_request_ptr = getReqToLookupBeaconDirectory_(key, skip_propagation_latency);
            assert(directory_lookup_request_ptr != NULL);

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 4, "beacon edge index:", std::to_string(tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key)).c_str(), "keystr:", key.getKeystr().c_str());
            #endif

            // Push the control request into edge-to-edge propagation simulator to send to beacon node
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_lookup_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: directory_lookup_request_ptr will be released by edge-to-edge propagation simulator
            directory_lookup_request_ptr = NULL;

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for DirectoryLookupResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                processRspToLookupBeaconDirectory_(control_response_ptr, is_being_written, is_valid_directory_exist, directory_info, best_placement_edgeset, need_hybrid_fetching);

                // Update total bandwidth usage for received directory lookup response
                BandwidthUsage directory_lookup_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_directory_lookup_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                directory_lookup_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_lookup_rsp_bandwidth_bytes, 0));
                total_bandwidth_usage.update(directory_lookup_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;

                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        // Add intermediate event if with event tracking
        struct timespec issue_directory_lookup_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_directory_lookup_req_latency_us = Util::getDeltaTimeUs(issue_directory_lookup_req_end_timestamp, issue_directory_lookup_req_start_timestamp);
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_LOOKUP_REQ_EVENT_NAME, issue_directory_lookup_req_latency_us);

        return is_finish;
    }

    bool CacheServerWorkerBase::redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        struct timespec issue_redirect_get_req_start_timestamp = Util::getCurrentTimespec();

        // Prepare destination address of target edge cache server
        NetworkAddr target_edge_cache_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getTargetDstaddr(directory_info); // Send to cache server of the target edge node for cache server worker

        while (true) // Timeout-and-retry mechanism
        {
            // Prepare redirected get request to get data from target edge node if any
            MessageBase* redirected_get_request_ptr = getReqToRedirectGet_(key, skip_propagation_latency);
            assert(redirected_get_request_ptr != NULL);

            // Push the redirected data request into edge-to-edge propagation simulator to target node
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(redirected_get_request_ptr, target_edge_cache_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: redirected_get_request_ptr will be released by edge-to-edge propagation simulator
            redirected_get_request_ptr = NULL;

            // Receive the redirected data repsonse from the target node
            DynamicArray redirected_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(redirected_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for RedirectedGetResponse");
                    continue; // Resend the redirected request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the redirected response message successfully
                MessageBase* redirected_response_ptr = MessageBase::getResponseFromMsgPayload(redirected_response_msg_payload);
                assert(redirected_response_ptr != NULL);

                // Get value and hitflag from redirected response message
                Hitflag hitflag = Hitflag::kGlobalMiss;
                processRspToRedirectGet_(redirected_response_ptr, value, hitflag);

                // Judge if key is cooperative cached and valid in the neighbor edge node
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
                    Util::dumpWarnMsg(base_instance_name_, oss.str());

                    is_cooperative_cached = false;
                    is_valid = false;
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid hitflag " << MessageBase::hitflagToString(hitflag) << " for redirectGetToTarget_()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Update total bandwidth usage for received redirected get response
                BandwidthUsage redirected_get_response_bandwidth_usage = redirected_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_redirected_get_rsp_bandwidth_bytes = redirected_response_ptr->getMsgPayloadSize();
                redirected_get_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_rsp_bandwidth_bytes, 0));
                total_bandwidth_usage.update(redirected_get_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(redirected_response_ptr->getEventListRef());

                // Release the redirected response message
                delete redirected_response_ptr;
                redirected_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        // Add intermediate event if with event tracking
        struct timespec issue_redirect_get_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_redirect_get_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_redirect_get_req_end_timestamp, issue_redirect_get_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_REDIRECT_GET_REQ_EVENT_NAME, issue_redirect_get_latency_us);

        return is_finish;
    }

    // (1.3) Access cloud

    bool CacheServerWorkerBase::fetchDataFromCloud_(const Key& key, Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        struct timespec issue_global_get_req_start_timestamp = Util::getCurrentTimespec();

        while (true) // Timeout-and-retry
        {
            // Prepare global get request to cloud
            uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            MessageBase* global_get_request_ptr = new GlobalGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
            assert(global_get_request_ptr != NULL);

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 5, "issue a global request;", "type:", MessageBase::messageTypeToString(global_get_request_ptr->getMessageType()).c_str(), "keystr:", key.getKeystr().c_str());
            #endif

            // Push the global request into edge-to-cloud propagation simulator to cloud
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeTocloudPropagationSimulatorParamPtr()->push(global_get_request_ptr, corresponding_cloud_recvreq_dst_addr_);
            assert(is_successful);

            // NOTE: global_get_request_ptr will be released by edge-to-cloud propagation simulator
            global_get_request_ptr = NULL;

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
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

                // Update total bandwidth usage for received global get response
                BandwidthUsage global_response_bandwidth_usage = global_response_ptr->getBandwidthUsageRef();
                uint32_t edge_cloud_global_rsp_bandwidth_bytes = global_response_ptr->getMsgPayloadSize();
                global_response_bandwidth_usage.update(BandwidthUsage(0, 0, edge_cloud_global_rsp_bandwidth_bytes));
                total_bandwidth_usage.update(global_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(global_get_response_ptr->getEventListRef());

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(base_instance_name_, 5, "receive a global response", "type:", MessageBase::messageTypeToString(global_response_ptr->getMessageType()).c_str(), "keystr:", global_get_response_ptr->getKey().getKeystr().c_str());
                #endif

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop

        // Add intermediate event if with event tracking
        struct timespec issue_global_get_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_global_get_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_global_get_req_end_timestamp, issue_global_get_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_GET_REQ_EVENT_NAME, issue_global_get_req_latency_us);
        
        return is_finish;
    }

    // (1.4) Update invalid cached objects in local edge cache

    // (2) Process write requests

    bool CacheServerWorkerBase::processLocalWriteRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL);
        assert(local_request_ptr->getMessageType() == MessageType::kLocalPutRequest || local_request_ptr->getMessageType() == MessageType::kLocalDelRequest);
        assert(recvrsp_dst_addr.isValidAddr());
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
        const bool skip_propagation_latency = local_request_ptr->isSkipPropagationLatency();

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 9, "receive a local write request;", "type:", MessageBase::messageTypeToString(local_request_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str(), "valuesize:", std::to_string(tmp_value.getValuesize()).c_str(), "is deleted:", Util::toString(tmp_value.isDeleted()).c_str());
        #endif
        
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        const Hitflag hitflag = Hitflag::kGlobalMiss; // Must be global miss due to write-through policy
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        
        // Update total bandwidth usage for received local put/del request
        uint32_t client_edge_local_req_bandwidth_bytes = local_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(client_edge_local_req_bandwidth_bytes, 0, 0));

        // Acquire write lock from beacon node no matter the locally cached object is valid or not, where the beacon will invalidate all other cache copies for cache coherence
        struct timespec acquire_writelock_start_timestamp = Util::getCurrentTimespec();
        LockResult lock_result = LockResult::kFailure;
        is_finish = acquireWritelock_(tmp_key, lock_result, total_bandwidth_usage, event_list, skip_propagation_latency);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        else
        {
            assert(lock_result == LockResult::kSuccess || lock_result == LockResult::kNoneed);
        }
        struct timespec acquire_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_writelock_end_timestamp, acquire_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_WRITELOCK_EVENT_NAME, acquire_writelock_latency_us); // Add intermediate event if with event tracking

        // Send request to cloud for write-through policy
        struct timespec write_cloud_start_timestamp = Util::getCurrentTimespec();
        is_finish = writeDataToCloud_(tmp_key, tmp_value, local_request_ptr->getMessageType(), total_bandwidth_usage, event_list, skip_propagation_latency);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        struct timespec write_cloud_end_timestamp = Util::getCurrentTimespec();
        uint32_t write_cloud_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(write_cloud_end_timestamp, write_cloud_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_WRITE_CLOUD_EVENT_NAME, write_cloud_latency_us); // Add intermediate event if with event tracking

        // Try to update/remove local edge cache
        struct timespec write_local_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_local_cached = false;
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeCacheServerRecvreqSourceAddr();
        // NOTE: message type has been checked, which must be one of the following two types
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            is_local_cached = updateLocalEdgeCache_(tmp_key, tmp_value);

            // NOTE: we will check capacity and trigger eviction for value updates (add events of intermediate response if with event tracking)
            is_finish = tmp_cache_server_ptr->evictForCapacity_(edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            is_local_cached = removeLocalEdgeCache_(tmp_key);

            // NOTE: no need to check capacity, as remove() only replaces the original value (value size + is_deleted) with a deleted value (zero value size + is_deleted of true), where deleted value uses minimum bytes and remove() cannot increase used bytes to trigger any eviction
        }
        UNUSED(is_local_cached);
        struct timespec write_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t write_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(write_local_cache_end_timestamp, write_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_WRITE_LOCAL_CACHE_EVENT_NAME, write_local_cache_latency_us); // Add intermediate event if with event tracking

        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }

        // Trigger independent cache admission for local/global cache miss if necessary
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value, total_bandwidth_usage, event_list, skip_propagation_latency);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Notify beacon node to finish writes if acquiring write lock successfully
        if (lock_result == LockResult::kSuccess)
        {
            struct timespec release_writelock_start_timestamp = Util::getCurrentTimespec();

            is_finish = releaseWritelock_(tmp_key, tmp_value, total_bandwidth_usage, event_list, skip_propagation_latency);

            // Add intermediate event if with event tracking
            struct timespec release_writelock_end_timestamp = Util::getCurrentTimespec();
            uint32_t release_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_writelock_end_timestamp, release_writelock_start_timestamp));
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_RELEASE_WRITELOCK_EVENT_NAME, release_writelock_latency_us);
        }
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }

        // Prepare local response
        MessageBase* local_response_ptr = NULL;
        uint64_t used_bytes = tmp_edge_wrapper_ptr->getSizeForCapacity();
        uint64_t capacity_bytes = tmp_edge_wrapper_ptr->getCapacityBytes();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            // Prepare LocalPutResponse for client
            local_response_ptr = new LocalPutResponse(tmp_key, hitflag, used_bytes, capacity_bytes, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            // Prepare LocalDelResponse for client
            local_response_ptr = new LocalDelResponse(tmp_key, hitflag, used_bytes, capacity_bytes, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }

        if (!is_finish) // // Edge node is STILL running
        {
            // Push local response message into edge-to-client propagation simulator to a client
            assert(local_response_ptr != NULL);
            assert(local_response_ptr->getMessageType() == MessageType::kLocalPutResponse || local_response_ptr->getMessageType() == MessageType::kLocalDelResponse);
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToclientPropagationSimulatorParamPtr()->push(local_response_ptr, recvrsp_dst_addr);
            assert(is_successful);
        }

        // NOTE: local_response_ptr will be released by edge-to-client propagation simulator
        local_response_ptr = NULL;

        return is_finish;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool CacheServerWorkerBase::acquireWritelock_(const Key& key, LockResult& lock_result, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        struct timespec acquire_local_or_remote_writelock_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);

        // Check if beacon node is the current edge node and acquire write permission
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> all_dirinfo;
        while (true) // Wait for write permission by polling or interruption
        {
            if (!tmp_edge_wrapper_ptr->isNodeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            if (current_is_beacon) // Get target edge index from local directory information
            {
                // Frequent polling
                is_finish = acquireLocalWritelock_(key, lock_result, all_dirinfo, total_bandwidth_usage, event_list, skip_propagation_latency);
                if (is_finish)
                {
                    return is_finish; // Edge is NOT running
                }
                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    continue; // Continue to try to acquire the write lock
                }
                else if (lock_result == LockResult::kSuccess) // If acquire write permission successfully
                {
                    // Invalidate all cache copies
                    tmp_edge_wrapper_ptr->parallelInvalidateCacheCopies(edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, key, all_dirinfo, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate response if with event tracking
                }
                // NOTE: will directly break if lock result is kNoneed
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                // Add events of intermediate response if with event tracking
                is_finish = acquireBeaconWritelock_(key, lock_result, total_bandwidth_usage, event_list, skip_propagation_latency);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate AcquireWritelockRequest
                    is_finish = blockForWritesByInterruption_(key, event_list, skip_propagation_latency);
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

        // Add intermediate event if with event tracking
        struct timespec acquire_local_or_remote_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_local_or_remote_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_local_or_remote_writelock_end_timestamp, acquire_local_or_remote_writelock_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_REMOTE_WRITELOCK_EVENT_NAME, acquire_local_or_remote_writelock_latency_us);

        return is_finish;
    }
    
    bool CacheServerWorkerBase::acquireBeaconWritelock_(const Key& key, LockResult& lock_result, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec issue_acquire_writelock_req_start_timestamp = Util::getCurrentTimespec();

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Prepare acquire writelock request to acquire permission for a write
            MessageBase* acquire_writelock_request_ptr = getReqToAcquireBeaconWritelock_(key, skip_propagation_latency);
            assert(acquire_writelock_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(acquire_writelock_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: acquire_writelock_request_ptr will be released by edge-to-edge propagation simulator
            acquire_writelock_request_ptr = NULL;

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for AcquireWritelockResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                // Get lock result from control response
                processRspToAcquireBeaconWritelock_(control_response_ptr, lock_result);

                // Update total bandwidth usage for received acquire writelock response
                BandwidthUsage acquire_writelock_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_acquire_writelock_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                acquire_writelock_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_acquire_writelock_rsp_bandwidth_bytes, 0));
                total_bandwidth_usage.update(acquire_writelock_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        // Add intermediate event if with event tracking
        struct timespec issue_acquire_writelock_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_acquire_writelock_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_acquire_writelock_req_end_timestamp, issue_acquire_writelock_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_ACQUIRE_WRITELOCK_REQ_EVENT_NAME, issue_acquire_writelock_req_latency_us);

        return is_finish;
    }

    bool CacheServerWorkerBase::blockForWritesByInterruption_(const Key& key, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Closest edge node must NOT be the beacon node if with interruption
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        BandwidthUsage tmp_bandwidth_usage; // NOTE: NO need to update total_bandwidth_usage, as the bandwidth usage is counted by the write request triggering FinishBlockRequest instead of the local request being blocked

        struct timespec block_for_writes_start_timestamp = Util::getCurrentTimespec();

        NetworkAddr recvrsp_dstaddr; // cache server worker or beacon server
        while (true) // Wait for FinishBlockRequest from beacon node by interruption
        {
            // Receive the control repsonse from the beacon node
            DynamicArray control_request_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvreq_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning()) // Edge is not running
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
                // Receive the control request message successfully
                MessageBase* control_request_ptr = MessageBase::getResponseFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL && control_request_ptr->getMessageType() == MessageType::kFinishBlockRequest);
                recvrsp_dstaddr = control_request_ptr->getSourceAddr();

                // Update tmp bandwidth usage for received finish block request
                uint32_t cross_edge_finish_block_req_bandwidth_bytes = control_request_ptr->getMsgPayloadSize();
                tmp_bandwidth_usage.update(BandwidthUsage(0, cross_edge_finish_block_req_bandwidth_bytes, 0));

                // Get key from FinishBlockRequest
                const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_request_ptr);
                Key tmp_key = finish_block_request_ptr->getKey();
                const bool finish_block_request_skip_propagation_latency = finish_block_request_ptr->isSkipPropagationLatency();

                // NOTE: skip_propagation_latency comes from local request A (currently blocked), while finish_block_request_skip_propagation_latency comes from local write request B, where B is processed before A to block A.
                if (skip_propagation_latency != finish_block_request_skip_propagation_latency)
                {
                    // Still hold skip_propagation_latency to simulate propagation latency for local request A (currently blocked), yet pose a warning
                    std::ostringstream oss;
                    oss << "skip_propagation_latency is " << Util::toString(skip_propagation_latency) << " for currently-blocked request, while finish_block_request_skip_propagation_latency is " << Util::toString(finish_block_request_skip_propagation_latency) << " for previous write request!";
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                }

                // Release the control request message
                delete control_request_ptr;
                control_request_ptr = NULL;

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
            // NOTE: we just execute break to finish block, so no need to add an event for FinishBlockResponse
            uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            MessageBase* finish_block_response_ptr = new FinishBlockResponse(key, edge_idx, edge_cache_server_worker_recvreq_source_addr_, tmp_bandwidth_usage, EventList(), skip_propagation_latency); // NOTE: still use skip_propagation_latency of currently-blocked request rather than that of previous write request
            assert(finish_block_response_ptr != NULL);

            // Push FinishBlockResponse into edge-to-edge propagation simulator to cache server worker or beacon server
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(finish_block_response_ptr, recvrsp_dstaddr);
            assert(is_successful);

            // NOTE: finish_block_response_ptr will be released by edge-to-edge propagation simulator
            finish_block_response_ptr = NULL;
        }

        // Add intermediate event if with event tracking
        struct timespec block_for_writes_end_timestamp = Util::getCurrentTimespec();
        uint32_t block_for_writes_latency_us = Util::getDeltaTimeUs(block_for_writes_end_timestamp, block_for_writes_start_timestamp);
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_BLOCK_FOR_WRITES_EVENT_NAME, block_for_writes_latency_us);

        return is_finish;
    }

    // (2.2) Update cloud

    bool CacheServerWorkerBase::writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {        
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        struct timespec issue_global_write_req_start_timestamp = Util::getCurrentTimespec();

        while (true) // Timeout-and-retry
        {
            // Prepare global write request message
            MessageBase* global_request_ptr = NULL;
            uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            if (message_type == MessageType::kLocalPutRequest)
            {
                global_request_ptr = new GlobalPutRequest(key, value, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
            }
            else if (message_type == MessageType::kLocalDelRequest)
            {
                global_request_ptr = new GlobalDelRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
            }
            else
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for writeDataToCloud_()!";
                Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }
            assert(global_request_ptr != NULL);

            // Push the global request into edge-to-cloud propagation simulator to cloud
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeTocloudPropagationSimulatorParamPtr()->push(global_request_ptr, corresponding_cloud_recvreq_dst_addr_);
            assert(is_successful);

            // NOTE: global_request_ptr will be released by edge-to-cloud propagation simulator
            global_request_ptr = NULL;

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
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

                // Update total bandwidth usage for received global put/del response
                BandwidthUsage global_response_bandwidth_usage = global_response_ptr->getBandwidthUsageRef();
                uint32_t edge_cloud_global_rsp_bandwidth_bytes = global_response_ptr->getMsgPayloadSize();
                global_response_bandwidth_usage.update(BandwidthUsage(0, 0, edge_cloud_global_rsp_bandwidth_bytes));
                total_bandwidth_usage.update(global_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(global_response_ptr->getEventListRef());

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop

        // Add intermediate event if with event tracking
        struct timespec issue_global_write_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_global_write_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_global_write_req_end_timestamp, issue_global_write_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_WRITE_REQ_EVENT_NAME, issue_global_write_req_latency_us);
        
        return is_finish;
    }

    // (2.3) Update cached objects in local edge cache

    // (2.4) Release write lock for MSI protocol

    bool CacheServerWorkerBase::releaseWritelock_(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        struct timespec release_local_or_remote_writelock_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);

        // Check if beacon node is the current edge node and lookup directory information
    
        if (current_is_beacon) // Get target edge index from local directory information
        {
            // Release write lock and get blocked edges
            std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges;
            is_finish = releaseLocalWritelock_(key, value, blocked_edges, total_bandwidth_usage, event_list, skip_propagation_latency);
            if (is_finish)
            {
                return is_finish; // Edge is NOT running
            }

            // Notify blocked edge nodes to finish blocking
            is_finish = tmp_edge_wrapper_ptr->parallelNotifyEdgesToFinishBlock(edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, key, blocked_edges, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate response if with event tracking
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        }
        else // Get target edge index from remote directory information at the beacon node
        {
            // NOTE: beacon server of beacon node has already notified all blocked edges -> NO need to notify them again in cache server
            is_finish = releaseBeaconWritelock_(key, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate response if with event tracking
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        } // End of current_is_beacon

        // Add intermediate event if with event tracking
        struct timespec release_local_or_remote_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t release_local_or_remote_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_local_or_remote_writelock_end_timestamp, release_local_or_remote_writelock_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_RELEASE_REMOTE_WRITELOCK_EVENT_NAME, release_local_or_remote_writelock_latency_us);

        return is_finish;
    }

    bool CacheServerWorkerBase::releaseBeaconWritelock_(const Key& key, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec issue_release_writelock_req_start_timestamp = Util::getCurrentTimespec();

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Prepare release writelock request to finish write
            MessageBase* release_writelock_request_ptr = getReqToReleaseBeaconWritelock_(key, skip_propagation_latency);
            assert(release_writelock_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(release_writelock_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: release_writelock_request_ptr will be released by edge-to-edge propagation simulator
            release_writelock_request_ptr = NULL;

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for ReleaseWritelockResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                // Process release writelock response
                processRspToReleaseBeaconWritelock_(control_response_ptr);

                // Update total bandwidth usage for received release writelock response
                BandwidthUsage release_writelock_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_release_writelock_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                release_writelock_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_release_writelock_rsp_bandwidth_bytes, 0));
                total_bandwidth_usage.update(release_writelock_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        // Add intermediate event if with event tracking
        struct timespec issue_release_writelock_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_release_writelock_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_release_writelock_req_end_timestamp, issue_release_writelock_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_RELEASE_WRITELOCK_REQ_EVENT_NAME, issue_release_writelock_req_latency_us);

        return is_finish;
    }

    // (3) Process redirected requests

    bool CacheServerWorkerBase::processRedirectedRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedDataRequest());

        bool is_finish = false; // Mark if local edge node is finished

        MessageType message_type = redirected_request_ptr->getMessageType();
        if (message_type == MessageType::kRedirectedGetRequest || message_type == MessageType::kCoveredRedirectedGetRequest || message_type == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            is_finish = processRedirectedGetRequest_(redirected_request_ptr, recvrsp_dst_addr);
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

    bool CacheServerWorkerBase::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        assert(redirected_request_ptr != NULL);
        assert(recvrsp_dst_addr.isValidAddr());

        // Get key and value from redirected request if any
        /*assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received redirected get request
        uint32_t cross_edge_redirected_get_req_bandwidth_bytes = redirected_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_req_bandwidth_bytes, 0));

        struct timespec target_get_local_cache_start_timestamp = Util::getCurrentTimespec();

        // Access local edge cache for cooperative edge caching (current edge node is the target edge node)
        Value tmp_value;
        bool is_cooperaitve_cached = false;
        bool is_cooperative_cached_and_valid = false;
        processReqForRedirectedGet_(redirected_request_ptr, tmp_value, is_cooperaitve_cached, is_cooperative_cached_and_valid);

        // Set hitflag accordingly
        Hitflag hitflag = Hitflag::kGlobalMiss;
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

        struct timespec target_get_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t target_get_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(target_get_local_cache_end_timestamp, target_get_local_cache_start_timestamp));
        bool is_background_request = redirected_request_ptr->isBackgroundRequest();
        if (is_background_request)
        {
            event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME, target_get_local_cache_latency_us);
        }
        else
        {
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME, target_get_local_cache_latency_us);
        }

        // NOTE: no need to perform recursive cooperative edge caching (current edge node is already the target edge node for cooperative edge caching)
        // NOTE: no need to access cloud to get data, which will be performed by the closest edge node

        // Prepare RedirectedGetResponse for the closest edge node
        MessageBase* redirected_get_response_ptr = getRspForRedirectedGet_(redirected_request_ptr, tmp_value, hitflag, total_bandwidth_usage, event_list);
        assert(redirected_get_response_ptr != NULL);

        // Push the redirected response message into edge-to-client propagation simulator to cache server worker in the closest edge node
        tmp_edge_wrapper_ptr->getEdgeToclientPropagationSimulatorParamPtr()->push(redirected_get_response_ptr, recvrsp_dst_addr);

        // NOTE: redirected_get_response_ptr will be released by edge-to-client propagation simulator
        redirected_get_response_ptr = NULL;

        return is_finish;
    }

    // (4) Cache management

    // (4.1) Admit uncached objects in local edge cache

    bool CacheServerWorkerBase::tryToTriggerIndependentAdmission_(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* local_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool is_finish = false;

        if (local_edge_cache_ptr->needIndependentAdmit(key)) // Trigger independent admission
        {
            // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
            assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);

            #ifdef DEBUG_CACHE_SERVER
            uint64_t used_bytes_before_admit = tmp_edge_wrapper_ptr->getSizeForCapacity();
            Util::dumpVariablesForDebug(base_instance_name_, 11, "independent admission;", "keystr:", key.getKeystr().c_str(), "keysize:", std::to_string(key.getKeyLength()).c_str(), "is value deleted:", Util::toString(value.isDeleted()).c_str(), "value size:", Util::toString(value.getValuesize()).c_str(), "used_bytes_before_admit:", std::to_string(used_bytes_before_admit).c_str());
            #endif

            is_finish = admitObject_(key, value, total_bandwidth_usage, event_list, skip_propagation_latency);
        }

        return is_finish;
    }

    bool CacheServerWorkerBase::admitObject_(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;

        struct timespec update_directory_to_admit_start_timestamp = Util::getCurrentTimespec();

        // Independently admit the new key-value pair into local edge cache
        bool is_being_written = false;
        is_finish = admitDirectory_(key, is_being_written, total_bandwidth_usage, event_list, skip_propagation_latency);
        if (is_finish)
        {
            return is_finish;
        }
        tmp_cache_server_ptr->admitLocalEdgeCache_(key, value, !is_being_written); // valid if not being written

        struct timespec update_directory_to_admit_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_directory_to_admit_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_directory_to_admit_end_timestamp, update_directory_to_admit_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_UPDATE_DIRECTORY_TO_ADMIT_EVENT_NAME, update_directory_to_admit_latency_us); // Add intermediate event if with event tracking

        // Trigger eviction if necessary
        is_finish = tmp_cache_server_ptr->evictForCapacity_(edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);

        return is_finish;
    }

    // (4.2) Update content directory information

    bool CacheServerWorkerBase::admitDirectory_(const Key& key, bool& is_being_written, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        struct timespec update_directory_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);

        // Check if beacon node is the current edge node and update directory information
        DirectoryInfo directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        if (current_is_beacon) // Update target edge index of local directory information
        {
            tmp_edge_wrapper_ptr->admitLocalDirectory_(key, directory_info, is_being_written);
        }
        else // Update remote directory information at the beacon node
        {
            // Add events of intermediate responses if with event tracking
            is_finish = tmp_cache_server_ptr->admitBeaconDirectory_(key, directory_info, is_being_written, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }

        // Add intermediate event if with event tracking
        struct timespec update_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_directory_end_timestamp, update_directory_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_UPDATE_REMOTE_DIRECTORY_EVENT_NAME, update_directory_latency_us);

        return is_finish;
    }

    // (4.3) Trigger non-blocking placement notification (ONLY for COVERED)

    bool CacheServerWorkerBase::tryToTriggerPlacementNotificationAfterHybridFetch_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME); // ONLY for COVERED

        bool is_finish = false;
        
        const bool sender_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        if (sender_is_beacon) // best_placement_edgeset and need_hybrid_fetching come from lookupLocalDirectory_()
        {
            // Trigger placement notification locally
            tmp_edge_wrapper_ptr->nonblockNotifyForPlacement(key, value, best_placement_edgeset, skip_propagation_latency);
        }
        else // best_placement_edgeset and need_hybrid_fetching come from lookupBeaconDirectory_()
        {
            // Trigger placement notification remotely at the beacon edge node
            is_finish = tmp_cache_server_ptr->notifyBeaconForPlacementAfterHybridFetch_(key, value, best_placement_edgeset, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }

        return is_finish;
    }

    // (5) Utility functions

    void CacheServerWorkerBase::checkPointers_() const
    {
        assert(cache_server_worker_param_ptr_ != NULL);
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
    }
}
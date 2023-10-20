#include "edge/cache_server/cache_server_placement_processor.h"

#include <assert.h>

#include "common/config.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerPlacementProcessor::kClassName = "CacheServerPlacementProcessor";

    void* CacheServerPlacementProcessor::launchCacheServerPlacementProcessor(void* cache_serer_placement_processor_param_ptr)
    {
        assert(cache_serer_placement_processor_param_ptr != NULL);

        CacheServerPlacementProcessor cache_server_placement_processor((CacheServerPlacementProcessorParam*)cache_serer_placement_processor_param_ptr);
        cache_server_placement_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerPlacementProcessor::CacheServerPlacementProcessor(CacheServerPlacementProcessorParam* cache_serer_placement_processor_param_ptr) : cache_server_placement_processor_param_ptr_(cache_serer_placement_processor_param_ptr)
    {
        assert(cache_serer_placement_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_serer_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();
        const uint32_t edgecnt = cache_serer_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-placement-processor";
        instance_name_ = oss.str();

        // For receiving control responses

        // Get source address of cache server placement processor to receive control responses and redirected data responses
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_placement_processor_recvrsp_port = Util::getEdgeCacheServerPlacementProcessorRecvrspPort(edge_idx, edgecnt);
        edge_cache_server_placement_processor_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_placement_processor_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_placement_processor_recvrsp_port);
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
    }

    CacheServerPlacementProcessor::~CacheServerPlacementProcessor()
    {
        // NOTE: no need to release cache_server_placement_processor_param_ptr_, which will be released outside CacheServerPlacementProcessor (by CacheServer)

        // Release the socket server to receive control responses,
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_placement_processor_recvrsp_socket_server_ptr_;
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = NULL;
    }

    void CacheServerPlacementProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the placement notification request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful_for_notify_placement = cache_server_placement_processor_param_ptr_->getNotifyPlacementRequestBufferPtr()->pop(tmp_cache_server_item);
            if (is_successful_for_notify_placement) // Receive an item for placement notification successfully
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->getMessageType() == MessageType::kCoveredPlacementNotifyRequest) // Placement notification
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processPlacementNotifyRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server placement processor thread
                assert(data_request_ptr != NULL);
                delete data_request_ptr;
                data_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful_for_notify_placement == true)

            // Try to get local cache admission from ring buffer pushed by edge cache server workers or edge beacon server
            LocalCacheAdmissionItem tmp_local_cache_admission_item;
            //bool is_successful_for_local_cache_admission = cache_server_placement_processor_param_ptr_->getLocalCacheAdmissionBufferPtr()->pop(tmp_local_cache_admission_item);
            bool is_successful_for_local_cache_admission = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->pop(tmp_local_cache_admission_item);
            if (is_successful_for_local_cache_admission) // Receive a local cache admission successfully
            {
                is_finish = processLocalCacheAdmission_(tmp_local_cache_admission_item);

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful_for_local_cache_admission == true)
        } // End of while loop

        return;
    }

    bool CacheServerPlacementProcessor::processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(data_request_ptr != NULL);
        assert(data_request_ptr->getMessageType() == MessageType::kCoveredPlacementNotifyRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        const bool is_background = true;

        const CoveredPlacementNotifyRequest* const covered_placement_notify_request_ptr = static_cast<const CoveredPlacementNotifyRequest*>(data_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, covered_placement_notify_request_ptr->getMsgPayloadSize(), 0));

        struct timespec placement_notify_start_timestamp = Util::getCurrentTimespec();

        // NOTE: CoveredPlacementNotifyRequest does NOT need placement edgeset, as placement notification has already been triggered
        //PlacementEdgeset tmp_placement_edgeset = covered_placement_notify_request_ptr->getEdgesetRef();
        //assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

        // Current edge node MUST NOT be the beacon node for the given key due to remote placement notification
        const Key tmp_key = covered_placement_notify_request_ptr->getKey();
        assert(!tmp_edge_wrapper_ptr->currentIsBeacon(tmp_key));

        // Victim synchronization
        const uint32_t source_edge_idx = covered_placement_notify_request_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_placement_notify_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, DirinfoSet, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(neighbor_victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        // Issue directory update request with is_admit = true
        bool is_being_written = false;
        const bool& skip_propagation_latency = covered_placement_notify_request_ptr->isSkipPropagationLatency();
        is_finish = tmp_cache_server_ptr->admitBeaconDirectory_(tmp_key, DirectoryInfo(source_edge_idx), is_being_written, edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
        if (is_finish)
        {
            return is_finish;
        }

        bool is_valid = covered_placement_notify_request_ptr->isValid();
        if (is_being_written) // Double-check is_being_written to udpate is_valid if necessary
        {
            // NOTE: ONLY update is_valid if is_being_written is true; if is_being_written is false (i.e., key is NOT being written now), we still keep original is_valid, as the value MUST be stale if is_being_written was true before
            is_valid = false;
        }

        // Admit into local edge cache for the received remote placement notification
        const Value tmp_value = covered_placement_notify_request_ptr->getValue();
        tmp_cache_server_ptr->admitLocalEdgeCache_(tmp_key, tmp_value, is_valid); // May update local synced victims

        // Perform background cache eviction in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
        // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
        is_finish = tmp_cache_server_ptr->evictForCapacity_(edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // May update local synced victims

        struct timespec placement_notify_end_timestamp = Util::getCurrentTimespec();
        uint32_t placement_notify_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(placement_notify_end_timestamp, placement_notify_start_timestamp));
        event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_PLACEMENT_NOTIFY_EVENT_NAME, placement_notify_latency_us); // Add placement notify event if with event tracking

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    bool CacheServerPlacementProcessor::processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        const bool is_background = true;

        // NOTE: NO need victim synchronization here (local placement notification from cache server worker (local beacon node) or beacon server (remote beacon node) has NO directory update request&response and hence NO victim synchronization -> see EdgeWrapper::nonblockNotifyForPlacement(); cache server worker (sender node) has finished victim synchronization when notifying results of hybrid data fetching to beacon node -> see CacheServerWorkerBase::notifyBeaconForPlacementAfterHybridFetch_())

        // NOTE: NO need to issue directory udpate requests (local placement notification from cache server worker (local beacon node) or beacon server (remote beacon node) does NOT need directory update request as the current edge node is beacon -> see EdgeWrapper::nonblockNotifyForPlacement(); cache server worker (sender node) has finished remote directory admission when notifying results of hybrid data fetching to beacon node -> see CacheServerWorkerBase::notifyBeaconForPlacementAfterHybridFetch_())

        struct timespec admission_start_timestamp = Util::getCurrentTimespec();

        // Admit into local edge cache for the received local cache admission decision
        const Key tmp_key = local_cache_admission_item.getKey();
        const Value tmp_value = local_cache_admission_item.getValue();
        const bool is_valid = local_cache_admission_item.isValid();
        tmp_cache_server_ptr->admitLocalEdgeCache_(tmp_key, tmp_value, is_valid); // May update local synced victims

        // Perform background cache eviction in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
        // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
        const bool skip_propagation_latency = local_cache_admission_item.skipPropagationLatency();
        is_finish = tmp_cache_server_ptr->evictForCapacity_(edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // May update local synced victims

        struct timespec admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(admission_end_timestamp, admission_start_timestamp));
        event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_LOCAL_CACHE_ADMISSION_EVENT_NAME, admission_latency_us); // Add placement notify event if with event tracking

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    void CacheServerPlacementProcessor::checkPointers_() const
    {
        assert(cache_server_placement_processor_param_ptr_ != NULL);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
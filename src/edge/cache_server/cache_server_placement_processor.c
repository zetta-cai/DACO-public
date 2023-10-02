#include "edge/cache_server/cache_server_placement_processor.h"

#include <assert.h>

#include "common/config.h"
#include "message/data_message.h"

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

    CacheServerPlacementProcessor::CacheServerPlacementProcessor(CacheServerPlacementProcessorParam* cache_serer_placement_processor_param_ptr) : cache_serer_placement_processor_param_ptr_(cache_serer_placement_processor_param_ptr)
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
        // NOTE: no need to release cache_serer_placement_processor_param_ptr_, which will be released outside CacheServerPlacementProcessor (by CacheServer)

        // Release the socket server to receive control responses,
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_placement_processor_recvrsp_socket_server_ptr_;
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = NULL;
    }

    void CacheServerPlacementProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_serer_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_serer_placement_processor_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getDataRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isManagementDataRequest()) // Management data requests
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processManagementDataRequest_(data_request_ptr, recvrsp_dst_addr);
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
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    bool CacheServerPlacementProcessor::processManagementDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        checkPointers_();
        assert(data_request_ptr != NULL);
        assert(data_request_ptr->isManagementDataRequest());

        bool is_finish = false;

        if (data_request_ptr->getMessageType() == MessageType::kCoveredPlacementNotifyRequest)
        {
            is_finish = processPlacementNotifyRequest_(data_request_ptr, recvrsp_dst_addr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for processManagementDataRequest_()!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return is_finish;
    }

    bool CacheServerPlacementProcessor::processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(data_request_ptr != NULL);
        assert(data_request_ptr->getMessageType() == MessageType::kCoveredPlacementNotifyRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_serer_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        const bool is_background = true;

        const CoveredPlacementNotifyRequest* const covered_placement_notify_request_ptr = static_cast<const CoveredPlacementNotifyRequest*>(data_request_ptr);

        // TODO: Embed placement edgeset into CoveredPlacementNotifyRequest for hybrid data fetching
        //PlacementEdgeset tmp_placement_edgeset = covered_placement_notify_request_ptr->getEdgesetRef();
        //assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

        // Current edge node MUST NOT be the beacon node for the given key due to remote placement notification
        const Key tmp_key = covered_placement_notify_request_ptr->getKey();
        assert(!tmp_edge_wrapper_ptr->currentIsBeacon(tmp_key));

        // Victim synchronization
        const uint32_t source_edge_idx = covered_placement_notify_request_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_placement_notify_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

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
        tmp_edge_wrapper_ptr->admitLocalEdgeCache_(tmp_key, tmp_value, is_valid); // May update local synced victims

        // Perform background cache eviction in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
        // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
        is_finish = tmp_edge_wrapper_ptr->evictForCapacity_(edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // May update local synced victims

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    void CacheServerPlacementProcessor::checkPointers_() const
    {
        assert(cache_serer_placement_processor_param_ptr_ != NULL);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
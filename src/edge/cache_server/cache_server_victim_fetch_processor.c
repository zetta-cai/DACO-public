#include "edge/cache_server/cache_server_victim_fetch_processor.h"

#include <assert.h>

#include "common/config.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerVictimFetchProcessor::kClassName = "CacheServerVictimFetchProcessor";

    void* CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor(void* cache_serer_victim_fetch_processor_param_ptr)
    {
        assert(cache_serer_victim_fetch_processor_param_ptr != NULL);

        CacheServerVictimFetchProcessor cache_server_victim_fetch_processor((CacheServerProcessorParam*)cache_serer_victim_fetch_processor_param_ptr);
        cache_server_victim_fetch_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerVictimFetchProcessor::CacheServerVictimFetchProcessor(CacheServerProcessorParam* cache_serer_victim_fetch_processor_param_ptr) : cache_serer_victim_fetch_processor_param_ptr_(cache_serer_victim_fetch_processor_param_ptr)
    {
        assert(cache_serer_victim_fetch_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_serer_victim_fetch_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();
        const uint32_t edgecnt = cache_serer_victim_fetch_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-victim-fetch-processor";
        instance_name_ = oss.str();

        // For receiving control responses

        // Get source address of cache server victim fetch processor to receive control responses and redirected data responses
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_victim_fetch_processor_recvrsp_port = Util::getEdgeCacheServerVictimFetchProcessorRecvrspPort(edge_idx, edgecnt);
        edge_cache_server_victim_fetch_processor_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_victim_fetch_processor_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_victim_fetch_processor_recvrsp_port);
        edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);
    }

    CacheServerVictimFetchProcessor::~CacheServerVictimFetchProcessor()
    {
        // NOTE: no need to release cache_serer_victim_fetch_processor_param_ptr_, which will be released outside CacheServerVictimFetchProcessor (by CacheServer)

        // Release the socket server to receive control responses,
        assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_;
        edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ = NULL;
    }

    void CacheServerVictimFetchProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_serer_victim_fetch_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_serer_victim_fetch_processor_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getDataRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest) // Victim fetch request
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processVictimFetchRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server victim fetch processor thread
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

    bool CacheServerVictimFetchProcessor::processVictimFetchRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(data_request_ptr != NULL);
        assert(data_request_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_serer_victim_fetch_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        const bool is_background = true;

        const CoveredVictimFetchRequest* const covered_victim_fetch_notify_request_ptr = static_cast<const CoveredVictimFetchRequest*>(data_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, covered_victim_fetch_notify_request_ptr->getMsgPayloadSize(), 0));

        struct timespec victim_fetch_start_timestamp = Util::getCurrentTimespec();

        // NOTE: Current edge node MUST NOT be the beacon node for the key (triggering victim fetching) due to remote victim fetch request
        // We do NOT make assertion here as CoveredVictimFetchRequest does NOT need to embed the key triggering victim fetching

        // Victim synchronization
        const uint32_t source_edge_idx = covered_victim_fetch_notify_request_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_victim_fetch_notify_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // TODO: Fetch victim information based on object size

        struct timespec victim_fetch_end_timestamp = Util::getCurrentTimespec();
        uint32_t victim_fetch_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(victim_fetch_end_timestamp, victim_fetch_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_VICTIM_FETCH_PROCESSOR_FETCHING_EVENT_NAME, victim_fetch_latency_us); // Add admission event if with event tracking
        
        // TODO: Send back CoveredVictimFetchResponse with total_bandwidth_usage and event_list

        return is_finish;
    }

    void CacheServerVictimFetchProcessor::checkPointers_() const
    {
        assert(cache_serer_victim_fetch_processor_param_ptr_ != NULL);
        assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
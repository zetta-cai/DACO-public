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

        CacheServerVictimFetchProcessor cache_server_victim_fetch_processor((CacheServerVictimFetchProcessorParam*)cache_serer_victim_fetch_processor_param_ptr);
        cache_server_victim_fetch_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerVictimFetchProcessor::CacheServerVictimFetchProcessor(CacheServerVictimFetchProcessorParam* cache_serer_victim_fetch_processor_param_ptr) : cache_serer_victim_fetch_processor_param_ptr_(cache_serer_victim_fetch_processor_param_ptr)
    {
        assert(cache_serer_victim_fetch_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_serer_victim_fetch_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();
        const uint32_t edgecnt = cache_serer_victim_fetch_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-victim-fetch-processor";
        instance_name_ = oss.str();

        // For receiving control responses

        UNUSED(edgecnt);
        // Get source address of cache server victim fetch processor to receive control responses and redirected data responses
        // const bool is_launch_edge = true; // The edge cache server victim fetch processor belongs to the logical edge node launched in the current physical machine
        // std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_launch_edge);
        // uint16_t edge_cache_server_victim_fetch_processor_recvrsp_port = Util::getEdgeCacheServerVictimFetchProcessorRecvrspPort(edge_idx, edgecnt);
        // edge_cache_server_victim_fetch_processor_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_victim_fetch_processor_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        //NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_victim_fetch_processor_recvrsp_port);
        //edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        //assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);
    }

    CacheServerVictimFetchProcessor::~CacheServerVictimFetchProcessor()
    {
        // NOTE: no need to release cache_serer_victim_fetch_processor_param_ptr_, which will be released outside CacheServerVictimFetchProcessor (by CacheServer)

        // Release the socket server to receive control responses,
        // assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);
        // delete edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_;
        // edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ = NULL;
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
            bool is_successful = cache_serer_victim_fetch_processor_param_ptr_->getControlRequestBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* control_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(control_request_ptr != NULL);

                if (control_request_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest) // Victim fetch request
                {
                    NetworkAddr recvrsp_dst_addr = control_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processVictimFetchRequest_(control_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server victim fetch processor thread
                assert(control_request_ptr != NULL);
                delete control_request_ptr;
                control_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    bool CacheServerVictimFetchProcessor::processVictimFetchRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_serer_victim_fetch_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        //const bool is_background = true;

        const CoveredVictimFetchRequest* const covered_victim_fetch_request_ptr = static_cast<const CoveredVictimFetchRequest*>(control_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, covered_victim_fetch_request_ptr->getMsgPayloadSize(), 0));

        struct timespec victim_fetch_start_timestamp = Util::getCurrentTimespec();

        // NOTE: Current edge node MUST NOT be the beacon node for the key (triggering victim fetching) due to remote victim fetch request
        // We do NOT make assertion here as CoveredVictimFetchRequest does NOT need to embed the key triggering victim fetching

        // Victim synchronization
        const uint32_t source_edge_idx = covered_victim_fetch_request_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_victim_fetch_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, DirinfoSet, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(neighbor_victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        // Get victim cacheinfos from local edge cache for object size
        const ObjectSize object_size = covered_victim_fetch_request_ptr->getObjectSize(); // Size of newly-admitted object (i.e., required size)
        std::list<VictimCacheinfo> tmp_victim_cacheinfos;
        bool has_victim_key = tmp_edge_wrapper_ptr->getEdgeCachePtr()->fetchVictimCacheinfosForRequiredSize(tmp_victim_cacheinfos, object_size); // NOTE: victim cacheinfos from local edge cache MUST be complete
        assert(has_victim_key == true);

        // Get dirinfo sets for local beaconed ones of fetched victims
        // NOTE: victim dirinfo sets from local directory table MUST be complete
        std::unordered_map<Key, DirinfoSet, KeyHasher> tmp_perkey_dirinfoset = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromCacheinfos(tmp_victim_cacheinfos);

        struct timespec victim_fetch_end_timestamp = Util::getCurrentTimespec();
        uint32_t victim_fetch_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(victim_fetch_end_timestamp, victim_fetch_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_VICTIM_FETCH_PROCESSOR_FETCHING_EVENT_NAME, victim_fetch_latency_us); // Add admission event if with event tracking
        
        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = control_request_ptr->getSourceIndex();
        VictimSyncset local_victim_syncset = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr()->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes()); // complete or compressed

        // Prepare victim fetchset for lazy victim fetching
        // NOTE: we do NOT care about seqnum, is_enforce_complete, and cache_margin_bytes in victim_fetchset
        VictimSyncset local_victim_fetchset(0, false, 0, tmp_victim_cacheinfos, tmp_perkey_dirinfoset);
        assert(local_victim_fetchset.isComplete()); // NOTE: extra fetched victim cacheinfos and dirinfo sets in victim fetchset MUST be complete
        
        // Prepare CoveredVictimFetchResponse with total_bandwidth_usage and event_list
        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const NetworkAddr edge_cache_server_recvreq_source_addr = cache_serer_victim_fetch_processor_param_ptr_->getCacheServerPtr()->getEdgeCacheServerRecvreqSourceAddr();
        const bool skip_propagation_latency = covered_victim_fetch_request_ptr->isSkipPropagationLatency();
        MessageBase* covered_victim_fetch_response_ptr = new CoveredVictimFetchResponse(local_victim_fetchset, local_victim_syncset, current_edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_victim_fetch_response_ptr != NULL);

        // Push CoveredVictimFetchResponse into edge-to-edge propagation simulator to send back to the beacon edge node
        bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(covered_victim_fetch_response_ptr, recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: covered_victim_fetch_response_ptr will be released by edge-to-edge propagation simulator
        covered_victim_fetch_response_ptr = NULL;

        return is_finish;
    }

    void CacheServerVictimFetchProcessor::checkPointers_() const
    {
        assert(cache_serer_victim_fetch_processor_param_ptr_ != NULL);
        //assert(edge_cache_server_victim_fetch_processor_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
#include "edge/cache_server/cache_server_metadata_update_processor.h"

#include <assert.h>

#include "cache/covered_cache_custom_func_param.h"
#include "common/config.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerMetadataUpdateProcessor::kClassName = "CacheServerMetadataUpdateProcessor";

    void* CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor(void* cache_server_metadata_update_processor_param_ptr)
    {
        assert(cache_server_metadata_update_processor_param_ptr != NULL);

        CacheServerMetadataUpdateProcessor cache_server_metadata_update_processor((CacheServerProcessorParam*)cache_server_metadata_update_processor_param_ptr);
        cache_server_metadata_update_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerMetadataUpdateProcessor::CacheServerMetadataUpdateProcessor(CacheServerProcessorParam* cache_server_metadata_update_processor_param_ptr) : cache_server_metadata_update_processor_param_ptr_(cache_server_metadata_update_processor_param_ptr)
    {
        assert(cache_server_metadata_update_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_metadata_update_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-metadata-update-processor";
        instance_name_ = oss.str();
    }

    CacheServerMetadataUpdateProcessor::~CacheServerMetadataUpdateProcessor()
    {
        // NOTE: no need to release cache_server_metadata_update_processor_param_ptr_, which will be released outside CacheServerMetadataUpdateProcessor (by CacheServer)
    }

    void CacheServerMetadataUpdateProcessor::start()
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_metadata_update_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_server_metadata_update_processor_param_ptr_->pop(tmp_cache_server_item, (void*)tmp_edge_wrapper_ptr);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* control_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(control_request_ptr != NULL);

                if (control_request_ptr->getMessageType() == MessageType::kCoveredMetadataUpdateRequest) // Metadata update request
                {
                    NetworkAddr recvrsp_dst_addr = control_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processMetadataUpdateRequest_(control_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server metadata update processor thread
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

    bool CacheServerMetadataUpdateProcessor::processMetadataUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredMetadataUpdateRequest);

        checkPointers_();
        CacheServer* tmp_cache_server_ptr = cache_server_metadata_update_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        // CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();
        // CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        const CoveredMetadataUpdateRequest* const covered_metadata_update_request_ptr = static_cast<const CoveredMetadataUpdateRequest*>(control_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, covered_metadata_update_request_ptr->getMsgPayloadSize(), 0, 0, 1, 0));

        struct timespec metadata_update_start_timestamp = Util::getCurrentTimespec();

        // NOTE: Current edge node MUST NOT be the beacon node for the key (triggering metadata update) due to remote cached metadata update request
        const Key tmp_key = covered_metadata_update_request_ptr->getKey();
        MYASSERT(!tmp_edge_wrapper_ptr->currentIsBeacon(tmp_key));

        // // Victim synchronization
        // const uint32_t source_edge_idx = covered_metadata_update_request_ptr->getSourceIndex();
        // const VictimSyncset& neighbor_victim_syncset = covered_metadata_update_request_ptr->getVictimSyncsetRef();
        // UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        // edge_wrapper_ptr_->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        // Update is_neighbor_cached flag in local cached metadata
        const bool is_neighbor_cached = covered_metadata_update_request_ptr->isNeighborCached();
        UpdateIsNeighborCachedFlagFuncParam tmp_param(tmp_key, is_neighbor_cached);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateIsNeighborCachedFlagFuncParam::FUNCNAME, &tmp_param);

        struct timespec metadata_update_end_timestamp = Util::getCurrentTimespec();
        uint32_t metadata_update_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(metadata_update_end_timestamp, metadata_update_start_timestamp));
        event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_METADATA_UPDATE_PROCESSOR_UPDATE_EVENT_NAME, metadata_update_latency_us); // Add metadata update event if with event tracking
        
        // Get background eventlist and bandwidth usage to update background counter for beacon server
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    void CacheServerMetadataUpdateProcessor::checkPointers_() const
    {
        assert(cache_server_metadata_update_processor_param_ptr_ != NULL);

        return;
    }
}
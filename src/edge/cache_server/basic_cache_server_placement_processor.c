#include "edge/cache_server/basic_cache_server_placement_processor.h"

#include "cache/basic_cache_custom_func_param.h"
#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicCacheServerPlacementProcessor::kClassName("BasicCacheServerPlacementProcessor)");

    BasicCacheServerPlacementProcessor::BasicCacheServerPlacementProcessor(CacheServerProcessorParam* cache_server_placement_processor_param_ptr) : CacheServerPlacementProcessorBase(cache_server_placement_processor_param_ptr)
    {
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-placement-processor";
        instance_name_ = oss.str();
    }

    BasicCacheServerPlacementProcessor::~BasicCacheServerPlacementProcessor()
    {}

    bool BasicCacheServerPlacementProcessor::processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME);

        assert(data_request_ptr != NULL);
        assert(data_request_ptr->getMessageType() == MessageType::kBestGuessBgplacePlacementNotifyRequest);

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update bandwidth usage for received request
        const BestGuessBgplacePlacementNotifyRequest* const bestguess_placement_notify_request_ptr = static_cast<const BestGuessBgplacePlacementNotifyRequest*>(data_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, bestguess_placement_notify_request_ptr->getMsgBandwidthSize(), 0, 0, 1, 0));

        // Vtime synchronization
        const uint32_t source_edge_idx = bestguess_placement_notify_request_ptr->getSourceIndex();
        const BestGuessSyncinfo syncinfo = bestguess_placement_notify_request_ptr->getSyncinfo();
        UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);

        const Key key = bestguess_placement_notify_request_ptr->getKey();
        const Value value = bestguess_placement_notify_request_ptr->getValue();
        const bool is_valid = bestguess_placement_notify_request_ptr->isValid();
        const ExtraCommonMsghdr extra_common_msghdr = bestguess_placement_notify_request_ptr->getExtraCommonMsghdr();
        is_finish = processPlacementNotifyRequestInternal_(key, value, is_valid, total_bandwidth_usage, event_list, extra_common_msghdr, recvrsp_dst_addr); // NOTE: will update background counter

        return is_finish;
    }

    bool BasicCacheServerPlacementProcessor::processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME);

        // NOTE: NO need to issue directory update requests and also no need for vtime synchronization here
        // -> (i) local placement notification from local/remote beacon server (beacon is placement) does NOT need directory update request&response and hence NO vtime synchronization (see BasicCacheServerWorker::triggerBestGuessPlacementInternal_() and BasicBeaconServer::processPlacementTriggerRequestForBestGuess_())
        // -> (ii) cache server worker (sender is placement) has finished remote directory admission with vtime synchronization after triggering placement by preserving an invalid dirinfo (see BasicCacheServerWorker::triggerBestGuessPlacementInternal_())

        bool is_finish = processLocalCacheAdmissionInternal_(local_cache_admission_item); // NOTE: will update background counter
        return is_finish;
    }
}
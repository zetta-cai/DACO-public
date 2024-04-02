#include "edge/cache_server/covered_cache_server_placement_processor.h"

#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CoveredCacheServerPlacementProcessor::kClassName("CoveredCacheServerPlacementProcessor)");

    CoveredCacheServerPlacementProcessor::CoveredCacheServerPlacementProcessor(CacheServerProcessorParam* cache_server_placement_processor_param_ptr) : CacheServerPlacementProcessorBase(cache_server_placement_processor_param_ptr)
    {
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-placement-processor";
        instance_name_ = oss.str();
    }

    CoveredCacheServerPlacementProcessor::~CoveredCacheServerPlacementProcessor()
    {}

    bool CoveredCacheServerPlacementProcessor::processPlacementNotifyRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        assert(data_request_ptr != NULL);
        assert(data_request_ptr->getMessageType() == MessageType::kCoveredBgplacePlacementNotifyRequest);

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // NOTE: CoveredBgplacePlacementNotifyRequest does NOT need placement edgeset, as placement notification has already been triggered
        //PlacementEdgeset tmp_placement_edgeset = covered_placement_notify_request_ptr->getEdgesetRef();
        //assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

        // Update bandwidth usage for received request
        const CoveredBgplacePlacementNotifyRequest* const covered_placement_notify_request_ptr = static_cast<const CoveredBgplacePlacementNotifyRequest*>(data_request_ptr);
        total_bandwidth_usage.update(BandwidthUsage(0, covered_placement_notify_request_ptr->getMsgBandwidthSize(), 0, 0, 1, 0, false));

        // Victim synchronization
        const uint32_t source_edge_idx = covered_placement_notify_request_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_placement_notify_request_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        const Key key = covered_placement_notify_request_ptr->getKey();
        const Value value = covered_placement_notify_request_ptr->getValue();
        const bool is_valid = covered_placement_notify_request_ptr->isValid();
        const ExtraCommonMsghdr extra_common_msghdr = covered_placement_notify_request_ptr->getExtraCommonMsghdr();
        is_finish = processPlacementNotifyRequestInternal_(key, value, is_valid, total_bandwidth_usage, event_list, extra_common_msghdr, recvrsp_dst_addr); // NOTE: will update background counter

        return is_finish;
    }

    bool CoveredCacheServerPlacementProcessor::processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        checkPointers_();

        // NOTE: NO need to issue directory update requests and also no need for victim synchronization here
        // -> (i) local placement notification from cache server worker (local beacon node) or beacon server (remote beacon node) does NOT need directory update request&response and hence NO victim synchronization (see CoveredEdgeWrapper::nonblockNotifyForPlacementInternal_())
        // -> (ii) cache server worker (sender node) has finished remote directory admission with victim synchronization when notifying results of hybrid data fetching to beacon node (see CoveredCacheServer::notifyBeaconForPlacementAfterHybridFetchInternal_())
        // -> (iii) cache server worker (sender node, yet not beacon node) has admitted directory information with victim synchronization after fast-path placement calculation (see CoveredCacheServerWorker::tryToTriggerCachePlacementForGetrspInternal_())

        bool is_finish = processLocalCacheAdmissionInternal_(local_cache_admission_item); // NOTE: will update background counter
        return is_finish;
    }
}
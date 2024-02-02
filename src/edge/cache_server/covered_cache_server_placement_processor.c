#include "edge/cache_server/covered_cache_server_placement_processor.h"

#include "common/util.h"

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

    bool CoveredCacheServerPlacementProcessor::processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        // NOTE: NO need to issue directory update requests and also no need for victim synchronization here
        // -> (i) local placement notification from cache server worker (local beacon node) or beacon server (remote beacon node) does NOT need directory update request&response and hence NO victim synchronization (see CoveredEdgeWrapper::nonblockNotifyForPlacementInternal_())
        // -> (ii) cache server worker (sender node) has finished remote directory admission with victim synchronization when notifying results of hybrid data fetching to beacon node (see CoveredCacheServer::notifyBeaconForPlacementAfterHybridFetchInternal_())
        // -> (iii) cache server worker (sender node, yet not beacon node) has admitted directory information with victim synchronization after fast-path placement calculation (see CoveredCacheServerWorker::tryToTriggerCachePlacementForGetrspInternal_())

        bool is_finish = processLocalCacheAdmissionInternal_(local_cache_admission_item);
        return is_finish;
    }
}
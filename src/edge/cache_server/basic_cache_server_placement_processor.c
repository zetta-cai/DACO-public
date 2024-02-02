#include "edge/cache_server/basic_cache_server_placement_processor.h"

#include "common/util.h"

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

    bool BasicCacheServerPlacementProcessor::processLocalCacheAdmission_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME);

        // NOTE: NO need to issue directory update requests and also no need for vtime synchronization here
        // -> (i) local placement notification from local/remote beacon server (beacon is placement) does NOT need directory update request&response and hence NO vtime synchronization (see BasicCacheServerWorker::triggerBestGuessPlacementInternal_() and BasicBeaconServer::processPlacementTriggerRequestForBestGuess_())
        // -> (ii) cache server worker (sender is placement) has finished remote directory admission with vtime synchronization after triggering placement by preserving an invalid dirinfo (see BasicCacheServerWorker::triggerBestGuessPlacementInternal_())

        bool is_finish = processLocalCacheAdmissionInternal_(local_cache_admission_item);
        return is_finish;
    }
}
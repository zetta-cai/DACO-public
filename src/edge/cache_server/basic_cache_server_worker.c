#include "edge/cache_server/basic_cache_server_worker.h"

#include <assert.h>
#include <sstream>

#include "cache/basic_cache_custom_func_param.h"
#include "edge/basic_edge_custom_func_param.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCacheServerWorker::kClassName("BasicCacheServerWorker");

    BasicCacheServerWorker::BasicCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate BasicCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    BasicCacheServerWorker::~BasicCacheServerWorker() {}

    // (1.1) Access local edge cache

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    bool BasicCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    bool BasicCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // NOTE: basic cooperative edge caching does NOT have directory metadata cache, so it always needs to send request to lookup remote directory information at beacon node
        bool need_lookup_beacon_directory = true;
        return need_lookup_beacon_directory;
    }

    MessageBase* BasicCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        // Prepare directory lookup request to check directory information in beacon node
        MessageBase* directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(directory_lookup_request_ptr != NULL);

        return directory_lookup_request_ptr;
    }

    void BasicCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, const uint32_t& content_discovery_cross_edge_latency_us) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kDirectoryLookupResponse);

        // Get directory information from the control response message
        const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
        is_being_written = directory_lookup_response_ptr->isBeingWritten();
        is_valid_directory_exist = directory_lookup_response_ptr->isValidDirectoryExist();
        directory_info = directory_lookup_response_ptr->getDirectoryInfo();

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(fast_path_hint);
        UNUSED(content_discovery_cross_edge_latency_us);

        return;
    }

    MessageBase* BasicCacheServerWorker::getReqToRedirectGet_(const uint32_t& dst_edge_idx_for_compression, const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        UNUSED(dst_edge_idx_for_compression);

        // Prepare redirected get request to fetch data from other edge nodes
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* redirected_get_request_ptr = new RedirectedGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(redirected_get_request_ptr != NULL);

        return redirected_get_request_ptr;
    }

    void BasicCacheServerWorker::processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag, const uint32_t& request_redirection_cross_edge_latency_us) const
    {
        assert(redirected_response_ptr != NULL);
        assert(redirected_response_ptr->getMessageType() == MessageType::kRedirectedGetResponse);

        // Get value and hitflag from redirected response message
        const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(redirected_response_ptr);
        value = redirected_get_response_ptr->getValue();
        hitflag = redirected_get_response_ptr->getHitflag();

        UNUSED(request_redirection_cross_edge_latency_us);

        return;
    }
    
    // (1.3) Access cloud

    void BasicCacheServerWorker::processRspToAccessCloud_(MessageBase* global_response_ptr, Value& value, const uint32_t& cloud_access_edge_cloud_latency) const
    {
        assert(global_response_ptr->getMessageType() == MessageType::kGlobalGetResponse);

        const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(global_response_ptr);
        value = global_get_response_ptr->getValue();

        UNUSED(cloud_access_edge_cloud_latency);

        return;
    }

    // (1.4) Update invalid cached objects in local edge cache

    bool BasicCacheServerWorker::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->removeIfInvalidForGetrsp(key, is_global_cached, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->updateIfInvalidForGetrsp(key, value, is_global_cached, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        
        return is_local_cached_and_invalid;
    }

    // (1.5) After getting value from local/neighbor/cloud

    bool BasicCacheServerWorker::afterFetchingValue_(const Key& key, const Value& value, const bool& is_tracked_before_fetch_value, const bool& is_cooperative_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        UNUSED(is_tracked_before_fetch_value);
        UNUSED(is_cooperative_cached);
        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(fast_path_hint);

        bool is_finish = false;

        // Trigger independent cache admission for local/global cache miss if necessary
        // NOTE: For COVERED, beacon node will tell the edge node whether to admit or not, w/o independent decision
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(key, value, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermediate responses if with event tracking
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Trigger best-guess placement/replacement
        if (tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME && !tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(key) && !is_cooperative_cached) // Local uncached and cooperative uncached (i.e., global uncached)
        {
            TriggerBestGuessPlacementFuncParam tmp_param(key, value, total_bandwidth_usage, event_list, skip_propagation_latency);
            constCustomFunc(TriggerBestGuessPlacementFuncParam::FUNCNAME, &tmp_param);
            is_finish = tmp_param.isFinish();
            if (is_finish)
            {
                return is_finish;
            }
        }

        return is_finish;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool BasicCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        lock_result = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->acquireLocalWritelockByCacheServer(key, current_edge_idx, all_dirinfo, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);
        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* acquire_writelock_request_ptr = new AcquireWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(acquire_writelock_request_ptr != NULL);

        return acquire_writelock_request_ptr;
    }

    void BasicCacheServerWorker::processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kAcquireWritelockResponse);

        // Get result of acquiring write lock
        const AcquireWritelockResponse* const acquire_writelock_response_ptr = static_cast<const AcquireWritelockResponse*>(control_response_ptr);
        lock_result = acquire_writelock_response_ptr->getLockResult();

        return;
    }

    void BasicCacheServerWorker::processReqToFinishBlock_(MessageBase* control_request_ptr) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kFinishBlockRequest);

        return;
    }

    MessageBase* BasicCacheServerWorker::getRspToFinishBlock_(MessageBase* control_request_ptr, const BandwidthUsage& tmp_bandwidth_usage) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kFinishBlockRequest);
        
        const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_request_ptr);
        const Key tmp_key = finish_block_request_ptr->getKey();
        const bool skip_propagation_latency = finish_block_request_ptr->isSkipPropagationLatency();

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* finish_block_response_ptr = new FinishBlockResponse(tmp_key, edge_idx, edge_cache_server_worker_recvreq_source_addr_, tmp_bandwidth_usage, EventList(), skip_propagation_latency); // NOTE: still use skip_propagation_latency of currently-blocked request rather than that of previous write request

        return finish_block_response_ptr;
    }

    // (2.3) Update cached objects in local edge cache

    bool BasicCacheServerWorker::updateLocalEdgeCache_(const Key& key, const Value& value, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_udpate = tmp_edge_wrapper_ptr->getEdgeCachePtr()->update(key, value, is_global_cached, affect_victim_tracker);
        UNUSED(affect_victim_tracker); // ONLY used by COVERED        

        return is_local_cached_after_udpate;
    }

    bool BasicCacheServerWorker::removeLocalEdgeCache_(const Key& key, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_remove = tmp_edge_wrapper_ptr->getEdgeCachePtr()->remove(key, is_global_cached, affect_victim_tracker);
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    bool BasicCacheServerWorker::releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_edge_idx, current_directory_info, is_source_cached);

        UNUSED(value);
        UNUSED(total_bandwidth_usgae);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* release_writelock_request_ptr = new ReleaseWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(release_writelock_request_ptr != NULL);

        return release_writelock_request_ptr;
    }

    bool BasicCacheServerWorker::processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kReleaseWritelockResponse);

        bool is_finish = false;

        // Do nothing for ReleaseWritelockResponse

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    // (3) Process redirected requests (see src/cache_server/cache_server_redirection_processor.*)

    // (4.1) Admit uncached objects in local edge cache

    // (4.2) Admit content directory information

    // (5) Cache-method-specific custom functions

    void BasicCacheServerWorker::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        checkPointers_();
        assert(func_param_ptr != NULL);

        if (funcname == TriggerBestGuessPlacementFuncParam::FUNCNAME)
        {
            TriggerBestGuessPlacementFuncParam* tmp_param_ptr = static_cast<TriggerBestGuessPlacementFuncParam*>(func_param_ptr);
            bool is_finish = triggerBestGuessPlacementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->isSkipPropagationLatency());
            tmp_param_ptr->setIsFinish(is_finish);
        }
        else
        {
            std::ostringstream oss;
            oss << "Unknown function name: " << funcname;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // Trigger best-guess placement/replacement for getrsp & putrsp
    bool BasicCacheServerWorker::triggerBestGuessPlacementInternal_(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        // Get placement edge idx with the approximate global LRU victim
        GetPlacementEdgeIdxParam tmp_param;
        tmp_edge_cache_ptr->constCustomFunc(GetPlacementEdgeIdxParam::FUNCNAME, &tmp_param);
        const uint32_t& placement_edge_idx = tmp_param.getPlacementEdgeIdxConstRef();
        assert(placement_edge_idx < tmp_edge_wrapper_ptr->getNodeCnt());

        // TODO: END HERE

        return false;
    }
}
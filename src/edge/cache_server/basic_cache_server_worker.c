#include "edge/cache_server/basic_cache_server_worker.h"

#include <assert.h>
#include <sstream>

#include "cache/basic_cache_custom_func_param.h"
#include "cooperation/basic_cooperation_custom_func_param.h"
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

    bool BasicCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
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
        UNUSED(extra_common_msghdr);

        return is_finish;
    }

    bool BasicCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // NOTE: basic cooperative edge caching does NOT have directory metadata cache, so it always needs to send request to lookup remote directory information at beacon node
        bool need_lookup_beacon_directory = true;
        return need_lookup_beacon_directory;
    }

    MessageBase* BasicCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        // Prepare directory lookup request to check directory information in beacon node
        const std::string cache_name = tmp_edge_wrapper_ptr->getCacheName();
        MessageBase* directory_lookup_request_ptr = NULL;
        if (cache_name != Util::BESTGUESS_CACHE_NAME) // other baselines
        {
            directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        else // BestGuess
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            directory_lookup_request_ptr = new BestGuessDirectoryLookupRequest(key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        assert(directory_lookup_request_ptr != NULL);

        return directory_lookup_request_ptr;
    }

    void BasicCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, const uint32_t& content_discovery_cross_edge_latency_us) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        assert(control_response_ptr != NULL);
        uint32_t source_edge_idx = control_response_ptr->getSourceIndex();

        const MessageType message_type = control_response_ptr->getMessageType();
        if (message_type == MessageType::kDirectoryLookupResponse)
        {
            // Get directory information from the control response message
            const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
            is_being_written = directory_lookup_response_ptr->isBeingWritten();
            is_valid_directory_exist = directory_lookup_response_ptr->isValidDirectoryExist();
            directory_info = directory_lookup_response_ptr->getDirectoryInfo();
        }
        else if (message_type == MessageType::kBestGuessDirectoryLookupResponse)
        {
            // Get directory information from the control response message
            const BestGuessDirectoryLookupResponse* const bestguess_directory_lookup_response_ptr = static_cast<const BestGuessDirectoryLookupResponse*>(control_response_ptr);
            is_being_written = bestguess_directory_lookup_response_ptr->isBeingWritten();
            is_valid_directory_exist = bestguess_directory_lookup_response_ptr->isValidDirectoryExist();
            directory_info = bestguess_directory_lookup_response_ptr->getDirectoryInfo();
            BestGuessSyncinfo syncinfo = bestguess_directory_lookup_response_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(fast_path_hint);
        UNUSED(content_discovery_cross_edge_latency_us);

        return;
    }

    MessageBase* BasicCacheServerWorker::getReqToRedirectGet_(const uint32_t& dst_edge_idx_for_compression, const Key& key, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        UNUSED(dst_edge_idx_for_compression);

        // Prepare redirected get request to fetch data from other edge nodes
        const std::string cache_name = tmp_edge_wrapper_ptr->getCacheName();
        MessageBase* redirected_get_request_ptr = NULL;
        if (cache_name != Util::BESTGUESS_CACHE_NAME) // other baselines
        {
            redirected_get_request_ptr = new RedirectedGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        else // BestGuess
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            redirected_get_request_ptr = new BestGuessRedirectedGetRequest(key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        assert(redirected_get_request_ptr != NULL);

        return redirected_get_request_ptr;
    }

    void BasicCacheServerWorker::processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag, const uint32_t& request_redirection_cross_edge_latency_us) const
    {
        assert(redirected_response_ptr != NULL);
        const MessageType message_type = redirected_response_ptr->getMessageType();

        // Get value and hitflag from redirected response message
        if (message_type == MessageType::kRedirectedGetResponse)
        {
            const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(redirected_response_ptr);
            value = redirected_get_response_ptr->getValue();
            hitflag = redirected_get_response_ptr->getHitflag();
        }
        else if (message_type == MessageType::kBestGuessRedirectedGetResponse)
        {
            const BestGuessRedirectedGetResponse* const bestguess_redirected_get_response_ptr = static_cast<const BestGuessRedirectedGetResponse*>(redirected_response_ptr);
            value = bestguess_redirected_get_response_ptr->getValue();
            hitflag = bestguess_redirected_get_response_ptr->getHitflag();
            BestGuessSyncinfo syncinfo = bestguess_redirected_get_response_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(bestguess_redirected_get_response_ptr->getSourceIndex(), syncinfo.getVtime());
            cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr()->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerWorker::processRspToRedirectGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        UNUSED(request_redirection_cross_edge_latency_us);

        return;
    }
    
    // (1.3) Access cloud

    void BasicCacheServerWorker::processRspToAccessCloud_(MessageBase* global_response_ptr, Value& value, const uint32_t& cloud_access_edge_cloud_latency) const
    {
        if (global_response_ptr->getMessageType() != MessageType::kGlobalGetResponse)
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << MessageBase::messageTypeToString(global_response_ptr->getMessageType()) << " for BasicCacheServerWorker::processRspToAccessCloud_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

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

    bool BasicCacheServerWorker::afterFetchingValue_(const Key& key, const Value& value, const bool& is_tracked_before_fetch_value, const bool& is_cooperative_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
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
        // NOTE: for COVERED, beacon node will tell the edge node whether to admit or not, w/o independent decision
        // NOTE: for BestGuess, the closest node will trigger best-guess placement via beacon node, w/o independent decision
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(key, value, total_bandwidth_usage, event_list, extra_common_msghdr); // Add events of intermediate responses if with event tracking
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Trigger best-guess placement/replacement for BestGuess
        if (tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME && !tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(key) && !is_cooperative_cached) // Local uncached and cooperative uncached (i.e., global uncached)
        {
            TriggerBestGuessPlacementFuncParam tmp_param(key, value, total_bandwidth_usage, event_list, extra_common_msghdr);
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

    bool BasicCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr)
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
        UNUSED(extra_common_msghdr);
        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* acquire_writelock_request_ptr = NULL;
        const std::string cache_name = tmp_edge_wrapper_ptr->getCacheName();
        if (cache_name != Util::BESTGUESS_CACHE_NAME) // other baselines
        {
            acquire_writelock_request_ptr = new AcquireWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        else // BestGuess
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            acquire_writelock_request_ptr = new BestGuessAcquireWritelockRequest(key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        assert(acquire_writelock_request_ptr != NULL);

        return acquire_writelock_request_ptr;
    }

    void BasicCacheServerWorker::processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const
    {
        assert(control_response_ptr != NULL);

        // Get result of acquiring write lock
        const MessageType messate_type = control_response_ptr->getMessageType();
        if (messate_type == MessageType::kAcquireWritelockResponse)
        {
            const AcquireWritelockResponse* const acquire_writelock_response_ptr = static_cast<const AcquireWritelockResponse*>(control_response_ptr);
            lock_result = acquire_writelock_response_ptr->getLockResult();
        }
        else if (messate_type == MessageType::kBestGuessAcquireWritelockResponse)
        {
            const BestGuessAcquireWritelockResponse* const bestguess_acquire_writelock_response_ptr = static_cast<const BestGuessAcquireWritelockResponse*>(control_response_ptr);
            lock_result = bestguess_acquire_writelock_response_ptr->getLockResult();
            BestGuessSyncinfo syncinfo = bestguess_acquire_writelock_response_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(bestguess_acquire_writelock_response_ptr->getSourceIndex(), syncinfo.getVtime());
            cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr()->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << messate_type << " for BasicCacheServerWorker::processRspToAcquireBeaconWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void BasicCacheServerWorker::processReqToFinishBlock_(MessageBase* control_request_ptr) const
    {
        assert(control_request_ptr != NULL);

        const MessageType message_type = control_request_ptr->getMessageType();
        if (message_type == MessageType::kFinishBlockRequest)
        {
            // Do nothing
        }
        else if (message_type == MessageType::kBestGuessFinishBlockRequest)
        {
            BestGuessFinishBlockRequest* bestguess_finish_block_request_ptr = static_cast<BestGuessFinishBlockRequest*>(control_request_ptr);
            BestGuessSyncinfo syncinfo = bestguess_finish_block_request_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(bestguess_finish_block_request_ptr->getSourceIndex(), syncinfo.getVtime());
            cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr()->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerWorker::processReqToFinishBlock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    MessageBase* BasicCacheServerWorker::getRspToFinishBlock_(MessageBase* control_request_ptr, const BandwidthUsage& tmp_bandwidth_usage) const
    {
        assert(control_request_ptr != NULL);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        
        const MessageType message_type = control_request_ptr->getMessageType();
        MessageBase* finish_block_response_ptr = NULL;
        if (message_type == MessageType::kFinishBlockRequest)
        {
            const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_request_ptr);
            const Key tmp_key = finish_block_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = finish_block_request_ptr->getExtraCommonMsghdr();

            finish_block_response_ptr = new FinishBlockResponse(tmp_key, edge_idx, edge_cache_server_worker_recvreq_source_addr_, tmp_bandwidth_usage, EventList(), extra_common_msghdr); // NOTE: still use extra_common_msghdr of currently-blocked request rather than that of previous write request
        }
        else if (message_type == MessageType::kBestGuessFinishBlockRequest)
        {
            const BestGuessFinishBlockRequest* const bestguess_finish_block_request_ptr = static_cast<const BestGuessFinishBlockRequest*>(control_request_ptr);
            const Key tmp_key = bestguess_finish_block_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = bestguess_finish_block_request_ptr->getExtraCommonMsghdr();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            finish_block_response_ptr = new BestGuessFinishBlockResponse(tmp_key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_worker_recvreq_source_addr_, tmp_bandwidth_usage, EventList(), extra_common_msghdr); // NOTE: still use extra_common_msghdr of currently-blocked request rather than that of previous write request
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerWorker::getRspToFinishBlock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(finish_block_response_ptr != NULL);

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

    bool BasicCacheServerWorker::releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_edge_idx, current_directory_info, is_source_cached);

        UNUSED(value);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(extra_common_msghdr);

        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        const std::string cache_name = tmp_edge_wrapper_ptr->getCacheName();
        MessageBase* release_writelock_request_ptr = NULL;
        if (cache_name != Util::BESTGUESS_CACHE_NAME) // other baselines
        {
            release_writelock_request_ptr = new ReleaseWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        else // BestGuess
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            release_writelock_request_ptr = new BestGuessReleaseWritelockRequest(key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
        }
        assert(release_writelock_request_ptr != NULL);

        return release_writelock_request_ptr;
    }

    bool BasicCacheServerWorker::processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        assert(control_response_ptr != NULL);
        const MessageType message_type = control_response_ptr->getMessageType();

        bool is_finish = false;

        if (message_type == MessageType::kReleaseWritelockResponse)
        {
            // Do nothing for ReleaseWritelockResponse
        }
        else if (message_type == MessageType::kBestGuessReleaseWritelockResponse)
        {
            const BestGuessReleaseWritelockResponse* const bestguess_release_writelock_response_ptr = static_cast<const BestGuessReleaseWritelockResponse*>(control_response_ptr);
            const BestGuessSyncinfo syncinfo = bestguess_release_writelock_response_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(bestguess_release_writelock_response_ptr->getSourceIndex(), syncinfo.getVtime());
            cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr()->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for BasicCacheServerWorker::processRspToReleaseBeaconWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(extra_common_msghdr);

        return is_finish;
    }

    // (2.5) After writing value into cloud and local edge cache if any

    bool BasicCacheServerWorker::afterWritingValue_(const Key& key, const Value& value, const LockResult& lock_result, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // NOTE: we MUST trigger placement (local independent admission of other baselines, or remote best-guess placement triggered by edge cache server worker of BestGuess via explicit placement trigger request) after releasing writelock if any -> otherwise the newly-admited object MUST be invalid!!!

        bool is_finish = false;

        // Trigger independent cache admission for local/global cache miss if necessary
        // NOTE: for COVERED, beacon node will tell the edge node whether to admit or not, w/o independent decision
        // NOTE: for BestGuess, the closest node will trigger best-guess placement via beacon node, w/o independent decision
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(key, value, total_bandwidth_usage, event_list, extra_common_msghdr);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Trigger best-guess placement/replacement for BestGuess
        if (tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME && lock_result == LockResult::kNoneed) // No need of writelock (i.e., global uncached)
        {
            TriggerBestGuessPlacementFuncParam tmp_param(key, value, total_bandwidth_usage, event_list, extra_common_msghdr);
            constCustomFunc(TriggerBestGuessPlacementFuncParam::FUNCNAME, &tmp_param);
            is_finish = tmp_param.isFinish();
            if (is_finish)
            {
                return is_finish;
            }
        }

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
            bool is_finish = triggerBestGuessPlacementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr());
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
    bool BasicCacheServerWorker::triggerBestGuessPlacementInternal_(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool is_finish = false;

        // Get placement edge idx with the approximate global LRU victim
        GetPlacementEdgeIdxParam tmp_param_for_placementidx;
        tmp_edge_cache_ptr->constCustomFunc(GetPlacementEdgeIdxParam::FUNCNAME, &tmp_param_for_placementidx);
        const uint32_t& placement_edge_idx = tmp_param_for_placementidx.getPlacementEdgeIdxConstRef();
        assert(placement_edge_idx < tmp_edge_wrapper_ptr->getNodeCnt());

        // Get trigger flag from local/remote beacon node if this is the first cache miss of the globally uncached object
        bool is_triggered = false;
        is_finish = getBestGuessTriggerFlag_(key, value, placement_edge_idx, extra_common_msghdr, is_triggered);
        if (is_finish)
        {
            return is_finish;
        }

        // Perform local cache admission if triggered (i.e., the first placement trigger) and sender is placement node
        if (is_triggered && placement_edge_idx == tmp_edge_wrapper_ptr->getNodeIdx())
        {
            // Admit local/remote beacon directory
            bool is_being_written = false;
            bool is_finish = admitDirectory_(key, is_being_written, total_bandwidth_usage, event_list, extra_common_msghdr);
            if (is_finish)
            {
                return is_finish;
            }

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton in the background
            const bool is_neighbor_cached = false; // (i) MUST be false due to is_triggered = true (i.e., the first placement trigger for the global uncached object); (ii) is_neighbor_cached will NOT be used by BestGuess local edge cachce
            const bool is_valid = !is_being_written;
            bool is_successful = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, is_neighbor_cached, is_valid, extra_common_msghdr));
            assert(is_successful);
        }

        return is_finish;
    }

    bool BasicCacheServerWorker::getBestGuessTriggerFlag_(const Key& key, const Value& value, const uint32_t& placement_edge_idx, const ExtraCommonMsghdr& extra_common_msghdr, bool& is_triggered) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool is_finish = false;

        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const uint32_t dst_beacon_edge_idx_to_trigger_placement = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        if (dst_beacon_edge_idx_to_trigger_placement == current_edge_idx) // Sender is beacon (get local trigger flag)
        {
            // Try to preserve invalid dirinfo in directory table for trigger flag
            PreserveDirectoryTableIfGlobalUncachedFuncParam tmp_param_for_preserve(key, DirectoryInfo(placement_edge_idx));
            tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->customFunc(PreserveDirectoryTableIfGlobalUncachedFuncParam::FUNCNAME, &tmp_param_for_preserve);
            is_triggered = tmp_param_for_preserve.isSuccessfulPreservationConstRef();
        }
        else // Sender is not beacon (get beacon trigger flag)
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_cache_ptr->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            // Prepare destination address of beacon server
            NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(dst_beacon_edge_idx_to_trigger_placement);

            while (true) // Timeout-and-retry mechanism
            {
                // Prepare placement trigger request
                MessageBase* tmp_request_ptr = new BestGuessPlacementTriggerRequest(key, value, BestGuessPlaceinfo(placement_edge_idx), BestGuessSyncinfo(local_victim_vtime), current_edge_idx, edge_cache_server_worker_recvrsp_source_addr_, extra_common_msghdr);
                assert(tmp_request_ptr != NULL);

                // Push the control request into edge-to-edge propagation simulator to the beacon node
                bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(tmp_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
                assert(is_successful);

                // NOTE: control_request_ptr will be released by edge-to-edge propagation simulator
                tmp_request_ptr = NULL;

                // Wait for the corresponding control response from the beacon edge node
                DynamicArray control_response_msg_payload;
                bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    if (!tmp_edge_wrapper_ptr->isNodeRunning())
                    {
                        is_finish = true; // Edge is NOT running
                        break; // Break from while (true)
                    }
                    else
                    {
                        std::ostringstream oss;
                        oss << "edge timeout to wait for BestGuessPlacementTriggerResponse for key " << key.getKeyDebugstr() << "from beacon " << dst_beacon_edge_idx_to_trigger_placement;
                        Util::dumpWarnMsg(instance_name_, oss.str());
                        continue; // Resend the control request message
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);

                    // Get trigger flag
                    assert(control_response_ptr->getMessageType() == MessageType::kBestGuessPlacementTriggerResponse);
                    BestGuessPlacementTriggerResponse* best_guess_placement_trigger_response_ptr = static_cast<BestGuessPlacementTriggerResponse*>(control_response_ptr);
                    is_triggered = best_guess_placement_trigger_response_ptr->isTriggered();

                    // Vtime synchronization
                    UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(best_guess_placement_trigger_response_ptr->getSourceIndex(), best_guess_placement_trigger_response_ptr->getSyncinfo().getVtime());
                    tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);

                    // Release the control response message
                    delete control_response_ptr;
                    control_response_ptr = NULL;

                    break;
                } // End of (is_timeout == false)
            } // End of while (true)
        } // End of (sender is not beacon)

        return is_finish;
    }
}
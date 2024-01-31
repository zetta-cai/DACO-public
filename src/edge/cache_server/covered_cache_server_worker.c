#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <list>
#include <sstream>

#include "cache/covered_cache_custom_func_param.h"
#include "common/util.h"
#include "cooperation/covered_cooperation_custom_func_param.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "edge/covered_edge_custom_func_param.h"
#include "message/control_message.h"
#include "message/data_message.h"

namespace covered
{
    const std::string CoveredCacheServerWorker::kClassName("CoveredCacheServerWorker");

    CoveredCacheServerWorker::CoveredCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate CoveredCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    CoveredCacheServerWorker::~CoveredCacheServerWorker() {}

    // (1.1) Access local edge cache

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    bool CoveredCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);

        // Update local/remote beacon access cnt for workload-aware probability tuning (NOTE: lookupLocalDirectory_() is ONLY invoked by CacheServerWorkerBase::fetchDataFromNeighbor_())
        tmp_edge_wrapper_ptr->getWeightTunerRef().incrLocalBeaconAccessCnt(); // Local beacon access (sender is beacon)

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory lookup
        GetCollectedPopularityParam tmp_param_for_popcollect(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param_for_popcollect); // collected_popularity.is_tracked indicates if the local uncached key is tracked in local uncached metadata

        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation (note that victim tracker has been updated by getLocalEdgeCache_() before this function)

        // Selective popularity aggregation after local directory lookup
        AfterDirectoryLookupHelperFuncParam tmp_param_after_dirlookup(key, current_edge_idx, tmp_param_for_popcollect.getCollectedPopularityConstRef(), is_global_cached, is_source_cached, best_placement_edgeset, need_hybrid_fetching, NULL, edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        tmp_edge_wrapper_ptr->constCustomFunc(AfterDirectoryLookupHelperFuncParam::FUNCNAME, &tmp_param_after_dirlookup);
        is_finish = tmp_param_after_dirlookup.isFinishConstRef();

        return is_finish;
    }

    bool CoveredCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool need_lookup_beacon_directory = true;

        // Check if key is tracked by local uncached metadata and get local uncached popularity if any
        GetCollectedPopularityParam tmp_param(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);

        const CollectedPopularity tmp_collected_popularity = tmp_param.getCollectedPopularityConstRef();
        bool is_key_tracked = tmp_collected_popularity.isTracked(); // Indicate if the local uncached key is tracked in local uncached metadata
        if (!is_key_tracked) // If key is NOT tracked by local uncached metadata (key is cached or key is uncached yet not popular)
        {
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key); // Remove cached directory info of untracked key if any
        }
        else
        {
            CachedDirectory cached_directory;
            bool is_large_popularity_change = true;
            bool has_cached_directory = tmp_covered_cache_manager_ptr->accessDirectoryCacherToCheckPopularityChange(key, tmp_collected_popularity.getLocalUncachedPopularity(), cached_directory, is_large_popularity_change);
            if (has_cached_directory) // If key has cached dirinfo
            {
                // NOTE: only local uncached object tracked by local uncached metadata can have cached directory
                assert(is_key_tracked == true);

                if (!is_large_popularity_change) // If popularity change is NOT large
                {
                    is_being_written = false; // NOTE: although we do NOT know whether key is being written from directory metadata cache, we can simply assume key is NOT being written temporarily to issue redirected get request; redirected get response will return a hitflag of kCooperativeInvalid if key is being written
                    is_valid_directory_exist = true; // NOTE: we ONLY cache valid remote directory for popular local uncached objects in directory metadata cache
                    directory_info = cached_directory.getDirinfo();

                    need_lookup_beacon_directory = false; // NO need to send directory lookup request to beacon node if hit directory metadata cache
                }

                // NOTE: If local uncached popularity has a large change compared with the previously-collected one, we still send directory lookup request to beacon node with piggybacking-based popularity collection for selective popularity aggregation
            }
        }

        return need_lookup_beacon_directory;
    }

    MessageBase* CoveredCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        // Update local/remote beacon access cnt for workload-aware probability tuning (NOTE: getReqToLookupBeaconDirectory_() is ONLY invoked by CacheServerWorkerBase::lookupBeaconDirectory_() in CacheServerWorkerBase::fetchDataFromNeighbor_())
        tmp_edge_wrapper_ptr->getWeightTunerRef().incrRemoteBeaconAccessCnt(); // Remote beacon access (sender is not beacon)

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        GetCollectedPopularityParam tmp_param(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata

        // Prepare CoveredDirectoryLookupRequest to check directory information in beacon node with popularity collection and victim synchronization
        MessageBase* covered_directory_lookup_request_ptr = new CoveredDirectoryLookupRequest(key, tmp_param.getCollectedPopularityConstRef(), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_directory_lookup_request_ptr != NULL);

        return covered_directory_lookup_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, const uint32_t& content_discovery_cross_edge_latency_us) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        const MessageType message_type = control_response_ptr->getMessageType();

        // Update EWMA of cross-edge latency for latency-aware weight tuning (NOTE: processRspToLookupBeaconDirectory_() is ONLY invoked by CacheServerWorkerBase::lookupBeaconDirectory_() in CacheServerWorkerBase::fetchDataFromNeighbor_())
        tmp_edge_wrapper_ptr->getWeightTunerRef().updateEwmaCrossedgeLatency(content_discovery_cross_edge_latency_us);

        Key tmp_key;
        uint32_t source_edge_idx = 0;
        VictimSyncset neighbor_victim_syncset;
        if (message_type == MessageType::kCoveredDirectoryLookupResponse) // Normal directory lookup response
        {
            // Get directory information from the control response message
            const CoveredDirectoryLookupResponse* const covered_directory_lookup_response_ptr = static_cast<const CoveredDirectoryLookupResponse*>(control_response_ptr);
            is_being_written = covered_directory_lookup_response_ptr->isBeingWritten();
            is_valid_directory_exist = covered_directory_lookup_response_ptr->isValidDirectoryExist();
            directory_info = covered_directory_lookup_response_ptr->getDirectoryInfo();

            // Get key, source edge idx, and victim syncset
            tmp_key = covered_directory_lookup_response_ptr->getKey();
            source_edge_idx = covered_directory_lookup_response_ptr->getSourceIndex();
            neighbor_victim_syncset = covered_directory_lookup_response_ptr->getVictimSyncsetRef();

            // NO hybrid data fetching
            best_placement_edgeset.clear();
            need_hybrid_fetching = false;
        }
        #ifdef ENABLE_FAST_PATH_PLACEMENT
        else if (message_type == MessageType::kCoveredFastDirectoryLookupResponse) // Directory lookup response with fast-path placement
        {
            // Get directory information from the control response message
            const CoveredFastDirectoryLookupResponse* const covered_fast_directory_lookup_response_ptr = static_cast<const CoveredFastDirectoryLookupResponse*>(control_response_ptr);
            is_being_written = covered_fast_directory_lookup_response_ptr->isBeingWritten();
            is_valid_directory_exist = covered_fast_directory_lookup_response_ptr->isValidDirectoryExist();
            directory_info = covered_fast_directory_lookup_response_ptr->getDirectoryInfo();

            // Get key, source edge idx, and victim syncset
            tmp_key = covered_fast_directory_lookup_response_ptr->getKey();
            source_edge_idx = covered_fast_directory_lookup_response_ptr->getSourceIndex();
            neighbor_victim_syncset = covered_fast_directory_lookup_response_ptr->getVictimSyncsetRef();

            // NO hybrid data fetching
            best_placement_edgeset.clear();
            need_hybrid_fetching = false;

            // Fast-path placement
            fast_path_hint = covered_fast_directory_lookup_response_ptr->getFastPathHintRef();
            assert(fast_path_hint.isValid());
        }
        #endif
        else if (message_type == MessageType::kCoveredPlacementDirectoryLookupResponse) // Directory lookup response with hybrid data fetching
        {
            // Get directory information from the control response message
            const CoveredPlacementDirectoryLookupResponse* const covered_placement_directory_lookup_response_ptr = static_cast<const CoveredPlacementDirectoryLookupResponse*>(control_response_ptr);
            is_being_written = covered_placement_directory_lookup_response_ptr->isBeingWritten();
            is_valid_directory_exist = covered_placement_directory_lookup_response_ptr->isValidDirectoryExist();
            directory_info = covered_placement_directory_lookup_response_ptr->getDirectoryInfo();

            // Get key, source edge idx, and victim syncset
            tmp_key = covered_placement_directory_lookup_response_ptr->getKey();
            source_edge_idx = covered_placement_directory_lookup_response_ptr->getSourceIndex();
            neighbor_victim_syncset = covered_placement_directory_lookup_response_ptr->getVictimSyncsetRef();

            // Get best placement edgeset for hybrid data fetching
            best_placement_edgeset = covered_placement_directory_lookup_response_ptr->getEdgesetRef();
            need_hybrid_fetching = true;
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for processRspToLookupBeaconDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Victim synchronization
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        // Update DirectoryCacher if necessary
        if (is_valid_directory_exist) // If with valid dirinfo
        {
            // Check if key is tracked by local uncached metadata and get local uncached popularity if any
            GetCollectedPopularityParam tmp_param(tmp_key);
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);

            const CollectedPopularity& tmp_collected_popularity = tmp_param.getCollectedPopularityConstRef();
            bool is_key_tracked = tmp_collected_popularity.isTracked(); // Indicate if the local uncached key is tracked in local uncached metadata
            if (is_key_tracked) // If key is tracked by local uncached metadata
            {
                tmp_covered_cache_manager_ptr->updateDirectoryCacherForNewCachedDirectory(tmp_key, CachedDirectory(directory_info, tmp_collected_popularity.getLocalUncachedPopularity())); // Add or insert new cached directory for the given key
            }
            else // Key is NOT tracked by local uncached metadata
            {
                // NOTE: NOT insert into DirectoryCacher if key is local cached (valid/invalid), or local uncached yet not tracked by local uncached metadata
                tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(tmp_key); // Remove existing cached directory if any
            }
        }
        else // If with invalid dirinfo
        {
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(tmp_key); // Remove existing cached directory if any
        }

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToRedirectGet_(const uint32_t& dst_edge_idx_for_compression, const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare redirected get request to fetch data from other edge nodes
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_redirected_get_request_ptr = new CoveredRedirectedGetRequest(key, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_redirected_get_request_ptr != NULL);

        return covered_redirected_get_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag, const uint32_t& request_redirection_cross_edge_latency_us) const
    {
        assert(redirected_response_ptr != NULL);
        assert(redirected_response_ptr->getMessageType() == MessageType::kCoveredRedirectedGetResponse);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Update EWMA of cross-edge latency for latency-aware weight tuning (NOTE: processRspToRedirectGet_() is ONLY invoked by CacheServerWorkerBase::redirectGetToTarget_() in CacheServerWorkerBase::fetchDataFromNeighbor_())
        tmp_edge_wrapper_ptr->getWeightTunerRef().updateEwmaCrossedgeLatency(request_redirection_cross_edge_latency_us);

        // Get value and hitflag from redirected response message
        const CoveredRedirectedGetResponse* const covered_redirected_get_response_ptr = static_cast<const CoveredRedirectedGetResponse*>(redirected_response_ptr);
        value = covered_redirected_get_response_ptr->getValue();
        hitflag = covered_redirected_get_response_ptr->getHitflag();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_redirected_get_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_redirected_get_response_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        // Invalidate DirectoryCacher if necessary
        if (hitflag == Hitflag::kCooperativeInvalid || hitflag == Hitflag::kGlobalMiss) // Dirinfo is invalid
        {
            const Key tmp_key = covered_redirected_get_response_ptr->getKey();
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(tmp_key); // Remove existing cached directory if any
        }
        // NOTE: If hitflag is kCooperativeHit, we do NOT need to insert/update DirectoryCacher, as it has been done during directory lookup if necessary

        return;
    }

    // (1.3) Access cloud

    void CoveredCacheServerWorker::processRspToAccessCloud_(MessageBase* global_response_ptr, Value& value, const uint32_t& cloud_access_edge_cloud_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        assert(global_response_ptr->getMessageType() == MessageType::kGlobalGetResponse);

        // Update EWMA of edge-cloud latency for latency-aware weight tuning (NOTE: processRspToAccessCloud_() is ONLY invoked by CacheServerWorkerBase::fetchDataFromCloud_() in CacheServerWorkerBase::fetchDataFromCloud_())
        tmp_edge_wrapper_ptr->getWeightTunerRef().updateEwmaEdgecloudLatency(cloud_access_edge_cloud_latency);

        const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(global_response_ptr);
        value = global_get_response_ptr->getValue();

        return;
    }

    // (1.4) Update invalid cached objects in local edge cache

    bool CoveredCacheServerWorker::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->removeIfInvalidForGetrsp(key, is_global_cached, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->updateIfInvalidForGetrsp(key, value, is_global_cached, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }

        // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
        UpdateCacheManagerForLocalSyncedVictimsFuncParam tmp_param(affect_victim_tracker);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME, &tmp_param);
        
        return is_local_cached_and_invalid;
    }

    // (1.5) After getting value from local/neighbor/cloud

    bool CoveredCacheServerWorker::afterFetchingValue_(const Key& key, const Value& value, const bool& is_tracked_before_fetch_value, const bool& is_cooperative_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        // NOTE: hybrid fetching MUST NOT trigger fast-path placement due to key is already tracked by local uncached metadata (no matter sender is beacon or not)
        if (need_hybrid_fetching)
        {
            assert(!fast_path_hint.isValid());

            // NOTE: key MUST be tracked before fetching value, such that directory lookup request can trigger placement calculation and hence need hybrid data fetching for non-blocking placement deployment
            assert(is_tracked_before_fetch_value);
        }
        if (fast_path_hint.isValid())
        {
            assert(!need_hybrid_fetching);

            // NOTE: key MUST NOT tracked by local uncached metadata before fetching value from neighbor/cloud
            assert(!is_tracked_before_fetch_value);
        }

        // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching
        struct timespec trigger_placement_start_timestamp = Util::getCurrentTimespec();
        if (need_hybrid_fetching)
        {
            TryToTriggerPlacementNotificationAfterHybridFetchFuncParam tmp_param(key, value, best_placement_edgeset, total_bandwidth_usage, event_list, skip_propagation_latency);
            constCustomFunc(TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::FUNCNAME, &tmp_param);
            is_finish = tmp_param.isFinishConstRef();
            if (is_finish) // Edge is NOT running now
            {
                return is_finish;
            }
        }
        struct timespec trigger_placement_end_timestamp = Util::getCurrentTimespec();
        uint32_t trigger_placement_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(trigger_placement_end_timestamp, trigger_placement_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_TRIGGER_PLACEMENT_EVENT_NAME, trigger_placement_latency_us); // Add intermediate event if with event tracking

        // Try to trigger cache placement calculation for getrsp if w/ sender-is-beacon or fast-path placement, if key is local uncached and newly-tracked after fetching value from neighbor/cloud
        if (!tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(key) && !is_tracked_before_fetch_value) // Local uncached object and NOT tracked before fetching value
        {
            GetCollectedPopularityParam tmp_param(key);
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);
            const CollectedPopularity tmp_collected_popularity_after_fetch_value = tmp_param.getCollectedPopularityRef();
            const bool is_tracked_after_fetch_value = tmp_collected_popularity_after_fetch_value.isTracked();
            if (is_tracked_after_fetch_value) // Newly-tracked after fetching value from neighbor/cloud (local uncached metadata is updated when accessing local edge cache to try to update invalid value)
            {
                struct timespec placement_for_getrsp_start_timestamp = Util::getCurrentTimespec();

                // Try to trigger cache placement if necessary (sender is beacon, or beacon node provides fast-path hint)
                TryToTriggerCachePlacementForGetrspFuncParam tmp_param(key, value, tmp_collected_popularity_after_fetch_value, fast_path_hint, is_cooperative_cached, total_bandwidth_usage, event_list, skip_propagation_latency);
                constCustomFunc(TryToTriggerCachePlacementForGetrspFuncParam::FUNCNAME, &tmp_param);
                is_finish = tmp_param.isFinishConstRef();
                if (is_finish)
                {
                    return is_finish;
                }

                struct timespec placement_for_getrsp_end_timestamp = Util::getCurrentTimespec();
                uint32_t placement_for_getrsp_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(placement_for_getrsp_end_timestamp, placement_for_getrsp_start_timestamp));
                event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_PLACEMENT_FOR_GETRSP_EVENT_NAME, placement_for_getrsp_latency_us); // Add intermediate event if with event tracking
            }
        }

        return is_finish;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool CoveredCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();

        bool is_finish = false;

        // Acquire local write lock for the given key
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        lock_result = tmp_cooperation_wrapper_ptr->acquireLocalWritelockByCacheServer(key, current_edge_idx, all_dirinfo, is_source_cached);
        bool is_global_cached = (lock_result != LockResult::kNoneed);

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local acquire write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local acquire write lock
        GetCollectedPopularityParam tmp_param_for_popcollect(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param_for_popcollect); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // NOTE: NO need to update local synced victims, which will be done by updateLocalEdgeCache_() and removeLocalEdgeCache_() after this function

        // Selective popularity aggregation after local acquire writelock
        AfterWritelockAcquireHelperFuncParam tmp_param_after_acquirelock(key, current_edge_idx, tmp_param_for_popcollect.getCollectedPopularityConstRef(), is_global_cached, is_source_cached, edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        tmp_edge_wrapper_ptr->constCustomFunc(AfterWritelockAcquireHelperFuncParam::FUNCNAME, &tmp_param_after_acquirelock);
        is_finish = tmp_param_after_acquirelock.isFinishConstRef();

        return is_finish;
    }

    MessageBase* CoveredCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        GetCollectedPopularityParam tmp_param(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        MessageBase* covered_acquire_writelock_request_ptr = new CoveredAcquireWritelockRequest(key, tmp_param.getCollectedPopularityConstRef(), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_acquire_writelock_request_ptr != NULL);

        // Remove existing cached directory if any as key will be local cached
        // NOTE: aquire write lock means global cached key and acquire beacon write lock means neighbor beaconed key (i.e., remote directory) -> even if key is local uncached and tracked, NO need to update previously-collected local uncached popularity in DirectoryCacher if any, as all cache copies in other edge nodes of the given key will become invalid
        tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key);

        return covered_acquire_writelock_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredAcquireWritelockResponse);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Get result of acquiring write lock
        const CoveredAcquireWritelockResponse* const covered_acquire_writelock_response_ptr = static_cast<const CoveredAcquireWritelockResponse*>(control_response_ptr);
        lock_result = covered_acquire_writelock_response_ptr->getLockResult();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_acquire_writelock_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_acquire_writelock_response_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    void CoveredCacheServerWorker::processReqToFinishBlock_(MessageBase* control_request_ptr) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredFinishBlockRequest);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Victim synchronization
        const CoveredFinishBlockRequest* const covered_finish_block_request_ptr = static_cast<const CoveredFinishBlockRequest*>(control_request_ptr);
        const uint32_t source_edge_idx = covered_finish_block_request_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_finish_block_request_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    MessageBase* CoveredCacheServerWorker::getRspToFinishBlock_(MessageBase* control_request_ptr, const BandwidthUsage& tmp_bandwidth_usage) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredFinishBlockRequest);
        
        const CoveredFinishBlockRequest* const covered_finish_block_request_ptr = static_cast<const CoveredFinishBlockRequest*>(control_request_ptr);
        const Key tmp_key = covered_finish_block_request_ptr->getKey();
        const bool skip_propagation_latency = covered_finish_block_request_ptr->isSkipPropagationLatency();

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t tmp_dst_edge_idx_for_compression = covered_finish_block_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(tmp_dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_finish_block_response_ptr = new CoveredFinishBlockResponse(tmp_key, victim_syncset, edge_idx, edge_cache_server_worker_recvreq_source_addr_, tmp_bandwidth_usage, EventList(), skip_propagation_latency); // NOTE: still use skip_propagation_latency of currently-blocked request rather than that of previous write request

        return covered_finish_block_response_ptr;
    }

    // (2.3) Update cached objects in local edge cache

    bool CoveredCacheServerWorker::updateLocalEdgeCache_(const Key& key, const Value& value, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_after_udpate = tmp_edge_wrapper_ptr->getEdgeCachePtr()->update(key, value, is_global_cached, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
        UpdateCacheManagerForLocalSyncedVictimsFuncParam tmp_param(affect_victim_tracker);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME, &tmp_param);

        return is_local_cached_after_udpate;
    }

    bool CoveredCacheServerWorker::removeLocalEdgeCache_(const Key& key, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_after_remove = tmp_edge_wrapper_ptr->getEdgeCachePtr()->remove(key, is_global_cached, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
        UpdateCacheManagerForLocalSyncedVictimsFuncParam tmp_param(affect_victim_tracker);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME, &tmp_param);

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    bool CoveredCacheServerWorker::releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        // Release local write lock for the given key
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_edge_idx, current_directory_info, is_source_cached);

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local release write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: although acquireLocalWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local release write lock
        GetCollectedPopularityParam tmp_param_for_popcollect(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param_for_popcollect); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation (note that we have updated victim tracker in updateLocalEdgeCache_() or removeLocalEdgeCache_() before this function)

        // Selective popularity aggregation after local release writelock
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        AfterWritelockReleaseHelperFuncParam tmp_param_after_releaselock(key, current_edge_idx, tmp_param_for_popcollect.getCollectedPopularityConstRef(), is_source_cached, best_placement_edgeset, need_hybrid_fetching, edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, total_bandwidth_usgae, event_list, skip_propagation_latency);
        tmp_edge_wrapper_ptr->constCustomFunc(AfterWritelockReleaseHelperFuncParam::FUNCNAME, &tmp_param_after_releaselock);
        is_finish = tmp_param_after_releaselock.isFinishConstRef();
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching
        if (need_hybrid_fetching)
        {
            NonblockNotifyForPlacementFuncParam tmp_param(key, value, best_placement_edgeset, skip_propagation_latency);
            tmp_edge_wrapper_ptr->constCustomFunc(NonblockNotifyForPlacementFuncParam::FUNCNAME, &tmp_param);
        }

        return is_finish;
    }

    MessageBase* CoveredCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: although getReqToAcquireBeaconWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        GetCollectedPopularityParam tmp_param(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        MessageBase* covered_release_writelock_request_ptr = new CoveredReleaseWritelockRequest(key, tmp_param.getCollectedPopularityConstRef(), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_release_writelock_request_ptr != NULL);

        // Remove existing cached directory if any as key will be local cached
        // NOTE: aquire write lock means global cached key and acquire beacon write lock means neighbor beaconed key (i.e., remote directory) -> even if key is local uncached and tracked, NO need to update previously-collected local uncached popularity in DirectoryCacher if any, as all cache copies in other edge nodes of the given key will become invalid
        // TODO: we may comment the removal as we have removed cached directory in getReqToAcquireBeaconWritelock_() for acquireBeaconWritelock_()
        tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key);

        return covered_release_writelock_request_ptr;
    }

    bool CoveredCacheServerWorker::processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        assert(control_response_ptr != NULL);
        const uint32_t source_edge_idx = control_response_ptr->getSourceIndex();

        bool is_finish = false;

        const MessageType message_type = control_response_ptr->getMessageType();
        Key tmp_key;
        VictimSyncset neighbor_victim_syncset;
        bool need_hybrid_fetching = false;
        Edgeset best_placement_edgeset; // Used only if need_hybrid_fetching = true
        if (message_type == MessageType::kCoveredReleaseWritelockResponse) // Normal release writelock response
        {
            const CoveredReleaseWritelockResponse* covered_release_writelock_response_ptr = static_cast<const CoveredReleaseWritelockResponse*>(control_response_ptr);
            tmp_key = covered_release_writelock_response_ptr->getKey();
            neighbor_victim_syncset = covered_release_writelock_response_ptr->getVictimSyncsetRef();
        }
        else if (message_type == MessageType::kCoveredPlacementReleaseWritelockResponse) // Release writelock response with hybrid data fetching
        {
            const CoveredPlacementReleaseWritelockResponse* covered_placement_release_writelock_response_ptr = static_cast<const CoveredPlacementReleaseWritelockResponse*>(control_response_ptr);
            tmp_key = covered_placement_release_writelock_response_ptr->getKey();
            neighbor_victim_syncset = covered_placement_release_writelock_response_ptr->getVictimSyncsetRef();

            need_hybrid_fetching = true;
            best_placement_edgeset = covered_placement_release_writelock_response_ptr->getEdgesetRef();
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type: " << message_type << " for processRspToReleaseBeaconWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        // Victim synchronization
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        // Do nothing for CoveredReleaseWritelockResponse, yet notify beacon for non-blocking placement notification for CoveredPlacementReleaseWritelockResponse after hybrid data fetching
        if (need_hybrid_fetching)
        {
            // Trigger placement notification remotely at the beacon edge node
            NotifyBeaconForPlacementAfterHybridFetchFuncParam tmp_param(tmp_key, value, best_placement_edgeset, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            tmp_cache_server_ptr->constCustomFunc(NotifyBeaconForPlacementAfterHybridFetchFuncParam::FUNCNAME, &tmp_param);
            is_finish = tmp_param.isFinishConstRef();
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

    void CoveredCacheServerWorker::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        checkPointers_();
        assert(func_param_ptr != NULL);

        if (funcname == TryToTriggerCachePlacementForGetrspFuncParam::FUNCNAME)
        {
            TryToTriggerCachePlacementForGetrspFuncParam* tmp_param_ptr = static_cast<TryToTriggerCachePlacementForGetrspFuncParam*>(func_param_ptr);

            bool& is_finish_ref = tmp_param_ptr->isFinishRef();
            is_finish_ref = tryToTriggerCachePlacementForGetrspInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getCollectedPopularityConstRef(), tmp_param_ptr->getFastPathHintConstRef(), tmp_param_ptr->isGlobalCached(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->isSkipPropagationLatency());
        }
        else if (funcname == TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::FUNCNAME)
        {
            TryToTriggerPlacementNotificationAfterHybridFetchFuncParam* tmp_param_ptr = static_cast<TryToTriggerPlacementNotificationAfterHybridFetchFuncParam*>(func_param_ptr);

            bool& is_finish_ref = tmp_param_ptr->isFinishRef();
            is_finish_ref = tryToTriggerPlacementNotificationAfterHybridFetchInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->isSkipPropagationLatency());
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid function name: " << funcname << " for constCustomFunc()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // Trigger cache placement for getrsp
    bool CoveredCacheServerWorker::tryToTriggerCachePlacementForGetrspInternal_(const Key& key, const Value& value, const CollectedPopularity& collected_popularity_after_fetch_value, const FastPathHint& fast_path_hint, const bool& is_global_cached, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        assert(collected_popularity_after_fetch_value.isTracked()); // MUST be tracked (actually newly-tracked) after fetching value from neighbor/cloud

        CacheServerBase* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;

        const bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        if (current_is_beacon) // Trigger normal cache placement by popularity aggregation if sender is beacon
        {
            assert(!fast_path_hint.isValid()); // ONLY remote beacon can provide FastPathHint to sender edge node

            uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            bool latest_is_global_cached = false; // NOTE: passed param (is_global_cached) is looked up before fetching value, which may be stale, so we use the latest is_global_cached here
            bool is_source_cached = false;
            tmp_cooperation_wrapper_ptr->isGlobalAndSourceCached(key, current_edge_idx, latest_is_global_cached, is_source_cached);

            // Selective popularity aggregation
            const bool need_placement_calculation = true;
            const bool sender_is_beacon = true; // Sender and beacon is the same edge node for placement calculation
            Edgeset best_placement_edgeset;
            best_placement_edgeset.clear();
            bool need_hybrid_fetching = false;
            is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity_after_fetch_value, latest_is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
            if (is_finish)
            {
                return is_finish; // Edge node is NOT running now
            }

            // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching
            if (need_hybrid_fetching)
            {
                NonblockNotifyForPlacementFuncParam tmp_param(key, value, best_placement_edgeset, skip_propagation_latency);
                tmp_edge_wrapper_ptr->constCustomFunc(NonblockNotifyForPlacementFuncParam::FUNCNAME, &tmp_param);
            }
        }
        #ifdef ENABLE_FAST_PATH_PLACEMENT
        else if (fast_path_hint.isValid()) // Trigger fast-path single-placement calculation if with valid FastPathHint
        {
            MYASSERT(!current_is_beacon); // ONLY remote beacon can provide FastPathHint to sender edge node

            // NOTE: although passed param (is_global_cached) is looked up before fetching value, which may be stale, we still use it due to approximate fast-path placement, instead of looking up the latest is_global_cached from remote beacon edge node, which will introduce extra message overhead

            // Calculate local admission benefit
            Popularity local_uncached_popularity = collected_popularity_after_fetch_value.getLocalUncachedPopularity();
            Popularity redirected_uncached_popularity = fast_path_hint.getSumLocalUncachedPopularity(); // NOTE: sum_local_uncached_popularity MUST NOT include local uncached popularity of the current edge node
            CalcLocalUncachedRewardFuncParam tmp_param(local_uncached_popularity, is_global_cached, redirected_uncached_popularity);
            tmp_edge_wrapper_ptr->constCustomFunc(CalcLocalUncachedRewardFuncParam::FUNCNAME, &tmp_param);
            DeltaReward local_admission_benefit = tmp_param.getRewardConstRef();

            // Approximate global admission policy
            bool is_global_popular = false;
            DeltaReward smallest_max_admission_benfit = fast_path_hint.getSmallestMaxAdmissionBenefit();
            if (smallest_max_admission_benfit == MIN_ADMISSION_BENEFIT) // Beacon edge node has sufficient space to add/update the local uncached popularity of the object from the current edge node
            {
                is_global_popular = true;
            }
            else if (local_admission_benefit > smallest_max_admission_benfit)
            {
                is_global_popular = true;
            }

            // Approximate single-placement calculation (a fast path w/o extra message overhead towards remote beacon edge node)
            if (is_global_popular)
            {
                const uint64_t tmp_object_size = key.getKeyLength() + value.getValuesize();
                const uint64_t tmp_cache_margin_bytes = tmp_edge_wrapper_ptr->getCacheMarginBytes();

                DeltaReward local_eviction_cost = 0.0;
                if (tmp_cache_margin_bytes >= tmp_object_size) // With sufficient space to admit the object -> NO need to evict
                {
                    local_eviction_cost = 0.0;
                }
                else
                {
                    // Find victims from local edge cache based on required size (NOT use local synced victims in victim tracker, as it still needs to access local edge cache if need more extra victims)
                    std::list<VictimCacheinfo> tmp_local_cached_victim_cacheinfos;
                    const uint64_t tmp_required_size = Util::uint64Minus(tmp_object_size, tmp_cache_margin_bytes);
                    bool tmp_is_exist_victims = tmp_edge_wrapper_ptr->getEdgeCachePtr()->fetchVictimCacheinfosForRequiredSize(tmp_local_cached_victim_cacheinfos, tmp_required_size);
                    assert(tmp_is_exist_victims == true);

                    // Get dirinfo for local beaconed victims
                    GetLocalBeaconedVictimsFromCacheinfosParam tmp_param(tmp_local_cached_victim_cacheinfos);
                    tmp_cooperation_wrapper_ptr->constCustomFunc(GetLocalBeaconedVictimsFromCacheinfosParam::FUNCNAME, &tmp_param);
                    const std::list<std::pair<Key, DirinfoSet>>& tmp_local_beaconed_local_cached_victim_dirinfoset_const_ref = tmp_param.getLocalBeaconedVictimDirinfosetsConstRef();

                    // Calculate local eviction cost by VictimTracker (to utilize perkey_victim_dirinfo_)
                    local_eviction_cost = tmp_covered_cache_manager_ptr->accessVictimTrackerForFastPathEvictionCost(tmp_edge_wrapper_ptr, tmp_local_cached_victim_cacheinfos, tmp_local_beaconed_local_cached_victim_dirinfoset_const_ref);
                }

                // Trigger local cache placement if local admission benefit is larger than local eviction cost
                if (local_admission_benefit > local_eviction_cost)
                {
                    // Admit dirinfo into remote beacon edge node
                    bool tmp_is_being_written = false;
                    const bool is_background = false; // Similar as only-sender hybrid data fetching
                    bool is_neighbor_cached = false; // NOTE: we do NOT use cooperation wrapper to check is_neighbor_cached, as sender must NOT be the beacon here
                    is_finish = tmp_cache_server_ptr->admitBeaconDirectory_(key, DirectoryInfo(tmp_edge_wrapper_ptr->getNodeIdx()), tmp_is_being_written, is_neighbor_cached, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
                    if (is_finish) // Edge is NOT running now
                    {
                        return is_finish;
                    }

                    // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker which may serve subsequent fast-path single-placement calculation
                    const bool tmp_is_valid = !tmp_is_being_written;
                    bool tmp_is_successful = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, is_neighbor_cached, tmp_is_valid, skip_propagation_latency));
                    assert(tmp_is_successful);
                }
            }
        }
        #endif

        return is_finish;
    }

    // Trigger non-blocking placement notification
    bool CoveredCacheServerWorker::tryToTriggerPlacementNotificationAfterHybridFetchInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        
        const bool sender_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        if (sender_is_beacon) // best_placement_edgeset and need_hybrid_fetching come from lookupLocalDirectory_()
        {
            // Trigger placement notification locally
            NonblockNotifyForPlacementFuncParam tmp_param(key, value, best_placement_edgeset, skip_propagation_latency);
            tmp_edge_wrapper_ptr->constCustomFunc(NonblockNotifyForPlacementFuncParam::FUNCNAME, &tmp_param);
        }
        else // best_placement_edgeset and need_hybrid_fetching come from lookupBeaconDirectory_()
        {
            // Trigger placement notification remotely at the beacon edge node
            NotifyBeaconForPlacementAfterHybridFetchFuncParam tmp_param(key, value, best_placement_edgeset, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            tmp_cache_server_ptr->constCustomFunc(NotifyBeaconForPlacementAfterHybridFetchFuncParam::FUNCNAME, &tmp_param);
            is_finish = tmp_param.isFinishConstRef();
        }

        return is_finish;
    }
}
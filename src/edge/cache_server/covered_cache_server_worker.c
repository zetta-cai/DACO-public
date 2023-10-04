#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <list>
#include <sstream>

#include "common/util.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "message/control_message.h"
#include "message/data_message.h"

namespace covered
{
    const std::string CoveredCacheServerWorker::kClassName("CoveredCacheServerWorker");

    CoveredCacheServerWorker::CoveredCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapper* tmp_edgewrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edgewrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edgewrapper_ptr->getNodeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate CoveredCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    CoveredCacheServerWorker::~CoveredCacheServerWorker() {}

    // (1.1) Access local edge cache

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    bool CoveredCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory lookup
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked indicates if the local uncached key is tracked in local uncached metadata

        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation (note that victim tracker has been updated by getLocalEdgeCache_() before this function)

        // Selective popularity aggregation
        const bool need_placement_calculation = true;
        bool need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // TODO: (END HERE) Process need_hybrid_fetching

        return is_finish;
    }

    bool CoveredCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool need_lookup_beacon_directory = true;

        // Check if key is tracked by local uncached metadata and get local uncached popularity if any
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity);

        bool is_key_tracked = collected_popularity.isTracked(); // Indicate if the local uncached key is tracked in local uncached metadata
        if (!is_key_tracked) // If key is NOT tracked by local uncached metadata (key is cached or key is uncached yet not popular)
        {
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key); // Remove cached directory info of untracked key if any
        }
        else
        {
            CachedDirectory cached_directory;
            bool is_large_popularity_change = true;
            bool has_cached_directory = tmp_covered_cache_manager_ptr->accessDirectoryCacherToCheckPopularityChange(key, collected_popularity.getLocalUncachedPopularity(), cached_directory, is_large_popularity_change);
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
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata

        // Prepare CoveredDirectoryLookupRequest to check directory information in beacon node with popularity collection and victim synchronization
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_directory_lookup_request_ptr = new CoveredDirectoryLookupRequest(key, collected_popularity, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_directory_lookup_request_ptr != NULL);

        return covered_directory_lookup_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // TODO: Process directory lookup response for non-blocking admission placement deployment

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryLookupResponse);

        // Get directory information from the control response message
        const CoveredDirectoryLookupResponse* const covered_directory_lookup_response_ptr = static_cast<const CoveredDirectoryLookupResponse*>(control_response_ptr);
        is_being_written = covered_directory_lookup_response_ptr->isBeingWritten();
        is_valid_directory_exist = covered_directory_lookup_response_ptr->isValidDirectoryExist();
        directory_info = covered_directory_lookup_response_ptr->getDirectoryInfo();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_directory_lookup_response_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_directory_lookup_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // Update DirectoryCacher if necessary
        const Key tmp_key = covered_directory_lookup_response_ptr->getKey();
        if (is_valid_directory_exist) // If with valid dirinfo
        {
            // Check if key is tracked by local uncached metadata and get local uncached popularity if any
            CollectedPopularity collected_popularity;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(tmp_key, collected_popularity);

            bool is_key_tracked = collected_popularity.isTracked(); // Indicate if the local uncached key is tracked in local uncached metadata
            if (is_key_tracked) // If key is tracked by local uncached metadata
            {
                tmp_covered_cache_manager_ptr->updateDirectoryCacherForNewCachedDirectory(tmp_key, CachedDirectory(directory_info, collected_popularity.getLocalUncachedPopularity())); // Add or insert new cached directory for the given key
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

    MessageBase* CoveredCacheServerWorker::getReqToRedirectGet_(const Key& key, const bool& skip_propagation_latency) const
    {
        // TODO: Update/invaidate directory metadata cache

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare redirected get request to fetch data from other edge nodes
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_redirected_get_request_ptr = new CoveredRedirectedGetRequest(key, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_redirected_get_request_ptr != NULL);

        return covered_redirected_get_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag) const
    {
        assert(redirected_response_ptr != NULL);
        assert(redirected_response_ptr->getMessageType() == MessageType::kCoveredRedirectedGetResponse);

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Get value and hitflag from redirected response message
        const CoveredRedirectedGetResponse* const covered_redirected_get_response_ptr = static_cast<const CoveredRedirectedGetResponse*>(redirected_response_ptr);
        value = covered_redirected_get_response_ptr->getValue();
        Hitflag hitflag = covered_redirected_get_response_ptr->getHitflag();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_redirected_get_response_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_redirected_get_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // Invalidate DirectoryCacher if necessary
        if (hitflag == Hitflag::kCooperativeInvalid || hitflag == Hitflag::kGlobalMiss) // Dirinfo is invalid
        {
            const Key tmp_key = covered_redirected_get_response_ptr->getKey();
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(tmp_key); // Remove existing cached directory if any
        }
        // NOTE: If hitflag is kCooperativeHit, we do NOT need to insert/update DirectoryCacher, as it has been done during directory lookup if necessary

        return;
    }

    // (1.4) Update invalid cached objects in local edge cache

    bool CoveredCacheServerWorker::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->removeIfInvalidForGetrsp(key, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->updateIfInvalidForGetrsp(key, value, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }
        
        return is_local_cached_and_invalid;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool CoveredCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;

        // Acquire local write lock for the given key
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        lock_result = tmp_cooperation_wrapper_ptr->acquireLocalWritelockByCacheServer(key, current_edge_idx, all_dirinfo, is_source_cached);
        bool is_global_cached = (lock_result != LockResult::kNoneed);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local acquire write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local acquire write lock
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // NOTE: NO need to update local synced victims, which will be done by updateLocalEdgeCache_() and removeLocalEdgeCache_() after this function

        // Selective popularity aggregation
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid even after cache placement
        const bool need_placement_calculation = false;
        bool need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        //assert(!has_best_placement);
        assert(!is_finish);
        assert(!need_hybrid_fetching);

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_acquire_writelock_request_ptr = new CoveredAcquireWritelockRequest(key, collected_popularity, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
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
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Get result of acquiring write lock
        const CoveredAcquireWritelockResponse* const covered_acquire_writelock_response_ptr = static_cast<const CoveredAcquireWritelockResponse*>(control_response_ptr);
        lock_result = covered_acquire_writelock_response_ptr->getLockResult();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_acquire_writelock_response_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_acquire_writelock_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    // (2.3) Update cached objects in local edge cache

    bool CoveredCacheServerWorker::updateLocalEdgeCache_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_udpate = tmp_edge_wrapper_ptr->getEdgeCachePtr()->update(key, value, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }

        return is_local_cached_after_udpate;
    }

    bool CoveredCacheServerWorker::removeLocalEdgeCache_(const Key& key) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_remove = tmp_edge_wrapper_ptr->getEdgeCachePtr()->remove(key, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    bool CoveredCacheServerWorker::releaseLocalWritelock_(const Key& key, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_finish = false;

        // Release local write lock for the given key
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_edge_idx, current_directory_info, is_source_cached);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local release write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: although acquireLocalWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local release write lock
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation (note that we have updated victim tracker in updateLocalEdgeCache_() or removeLocalEdgeCache_() before this function)

        // Selective popularity aggregation
        const bool is_global_cached = true; // NOTE: invoking releaseLocalWritelock_() means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        const bool need_placement_calculation = true;
        bool need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usgae, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // TODO: (END HERE) Process need_hybrid_fetching

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: although getReqToAcquireBeaconWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_release_writelock_request_ptr = new CoveredReleaseWritelockRequest(key, collected_popularity, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_release_writelock_request_ptr != NULL);

        // Remove existing cached directory if any as key will be local cached
        // NOTE: aquire write lock means global cached key and acquire beacon write lock means neighbor beaconed key (i.e., remote directory) -> even if key is local uncached and tracked, NO need to update previously-collected local uncached popularity in DirectoryCacher if any, as all cache copies in other edge nodes of the given key will become invalid
        // TODO: we may comment the removal as we have removed cached directory in getReqToAcquireBeaconWritelock_() for acquireBeaconWritelock_()
        tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key);

        return covered_release_writelock_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredReleaseWritelockResponse);
        const CoveredReleaseWritelockResponse* covered_release_writelock_response_ptr = static_cast<const CoveredReleaseWritelockResponse*>(control_response_ptr);

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Do nothing for CoveredReleaseWritelockResponse

        // Victim synchronization
        const uint32_t source_edge_idx = covered_release_writelock_response_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_release_writelock_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    // (3) Process redirected requests

    void CoveredCacheServerWorker::processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const
    {
        // Get key and victim syncset from redirected get request
        assert(redirected_request_ptr != NULL);
        assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
        Key tmp_key;
        uint32_t source_edge_idx = redirected_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset;
        if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
        {
            const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_redirected_get_request_ptr->getKey();
            victim_syncset = covered_redirected_get_request_ptr->getVictimSyncsetRef();
        }
        else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
            victim_syncset = covered_placement_redirected_get_request_ptr->getVictimSyncsetRef();
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Access local edge cache for redirected get request
        is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, value);
        is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);

        // Victim synchronization
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);
        
        return;
    }

    MessageBase* CoveredCacheServerWorker::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        // Get key (and placement edgeset) from redirected get request
        assert(redirected_request_ptr != NULL);
        assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
        Key tmp_key;
        Edgeset tmp_placement_edgeset;
        bool skip_propagation_latency = redirected_request_ptr->isSkipPropagationLatency();
        if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
        {
            const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_redirected_get_request_ptr->getKey();
        }
        else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
            tmp_placement_edgeset = covered_placement_redirected_get_request_ptr->getEdgesetRef();
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare redirected get response
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeCacheServerRecvreqSourceAddr();
        MessageBase* response_ptr = NULL;
        if (!redirected_request_ptr->isBackgroundRequest()) // Send back normal redirected get response
        {
            // NOTE: CoveredRedirectedGetResponse will be processed by edge cache server worker of the sender edge node
            response_ptr = new CoveredRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else // Send back redirected get response for non-blocking placement deploymeng
        {
            assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

            // NOTE: CoveredPlacementRedirectedGetResponse will be processed by edge beacon server of the sender edge node
            response_ptr = new CoveredPlacementRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, tmp_placement_edgeset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        assert(response_ptr != NULL);

        return response_ptr;
    }

    // (4.1) Admit uncached objects in local edge cache

    // (4.2) Admit content directory information
}
#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <list>
#include <sstream>

#include "common/util.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "message/control_message.h"

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

    bool CoveredCacheServerWorker::getLocalEdgeCache_(const Key& key, Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_and_valid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->get(key, value, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            updateCacheManagerForLocalSyncedVictims_();
        }
        
        return is_local_cached_and_valid;
    }

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    void CoveredCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, is_being_written, is_valid_directory_exist, directory_info);

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory lookup
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

        // Selective popularity aggregation
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node

        return;
    }

    bool CoveredCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool need_lookup_beacon_directory = true;

        // Check if key is tracked by local uncached metadata and get local uncached popularity if any
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata
        if (!is_key_tracked) // If key is NOT tracked by local uncached metadata (key is cached or key is uncached yet not popular)
        {
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirinfo(key); // Remove cached directory info of untracked key if any
        }
        else
        {
            bool has_cached_dirinfo = tmp_covered_cache_manager_ptr->accessDirectoryCacherForCachedDirinfo(key, directory_info);
            if (has_cached_dirinfo)
            {
                // NOTE: only local uncached object tracked by local uncached metadata can have cached dirinfo
                assert(is_key_tracked == true);

                // TODO: introduce previously-collected popularity in DirectoryCacher

                is_being_written = false; // NOTE: although we do NOT know whether key is being written from directory metadata cache, we can simply assume key is NOT being written temporarily to issue redirected get request; redirected get response will return a hitflag of kCooperativeInvalid if key is being written
                is_valid_directory_exist = true; // NOTE: we ONLY cache valid remote directory for popular local uncached objects in directory metadata cache

                need_lookup_beacon_directory = false; // NO need to send directory lookup request to beacon node if hit directory metadata cache
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
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

        // Prepare CoveredDirectoryLookupRequest to check directory information in beacon node with popularity collection and victim synchronization
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_directory_lookup_request_ptr = new CoveredDirectoryLookupRequest(key, CollectedPopularity(is_key_tracked, local_uncached_popularity), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
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
            Popularity local_uncached_popularity = 0.0;
            bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(tmp_key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

            if (is_key_tracked) // If key is tracked by local uncached metadata
            {
                tmp_covered_cache_manager_ptr->updateDirectoryCacherForNewCachedDirinfo(tmp_key, directory_info); // Add or insert new cached dirinfo for the given key
            }
            else // Key is NOT tracked by local uncached metadata
            {
                tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirinfo(tmp_key); // Remove existing cached dirinfo if any
            }
        }
        else // If with invalid dirinfo
        {
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirinfo(tmp_key); // Remove existing cached dirinfo if any
        }

        return;
    }

    bool CoveredCacheServerWorker::redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Update/invaidate expiration-based local directory cache
        // TODO: If local uncached popularity has large change compared with last sync, explicitly sync latest local uncached popularity to beacon node by piggybacking to update aggregated uncached popularity
        return false;
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
            updateCacheManagerForLocalSyncedVictims_();
        }
        
        return is_local_cached_and_invalid;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    void CoveredCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Acquire local write lock for the given key
        lock_result = tmp_cooperation_wrapper_ptr->acquireLocalWritelockByCacheServer(key, all_dirinfo);
        bool is_global_cached = (lock_result != LockResult::kNoneed);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local acquire write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local acquire write lock
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // Return false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // Selective popularity aggregation
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node

        // NOTE: NO need to update local synced victims, which will be done by updateLocalEdgeCache_() and removeLocalEdgeCache_()

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
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // Return false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_acquire_writelock_request_ptr = new CoveredAcquireWritelockRequest(key, CollectedPopularity(is_key_tracked, local_uncached_popularity), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_acquire_writelock_request_ptr != NULL);

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
            updateCacheManagerForLocalSyncedVictims_();
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
            updateCacheManagerForLocalSyncedVictims_();
        }

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    void CoveredCacheServerWorker::releaseLocalWritelock_(const Key& key, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Release local write lock for the given key
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_directory_info);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local release write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as is_key_tracked MUST be false if key is local cached and will NOT update/add aggregated uncached popularity

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: although acquireLocalWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local release write lock
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // Return false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        // Selective popularity aggregation
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const bool is_global_cached = true; // NOTE: invoking releaseLocalWritelock_() means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node

        // NOTE: NO need to update local synced victims, which will be done by updateLocalEdgeCache_() and removeLocalEdgeCache_()

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
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // Return false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_release_writelock_request_ptr = new CoveredReleaseWritelockRequest(key, CollectedPopularity(is_key_tracked, local_uncached_popularity), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_release_writelock_request_ptr != NULL);

        return;
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

    /*bool CoveredCacheServerWorker::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        // TODO: Piggyback candidate victims in current edge node
        return false;
    }*/

    // (4.1) Admit uncached objects in local edge cache

    void CoveredCacheServerWorker::admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_valid) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->admit(key, value, is_valid, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key is a local synced victim now
        {
            updateCacheManagerForLocalSyncedVictims_();
        }

        return;
    }

    // (4.2) Evict cached objects from local edge cache

    void CoveredCacheServerWorker::evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        tmp_edge_wrapper_ptr->getEdgeCachePtr()->evict(victims, required_size);

        // NOTE: eviction MUST affect victim tracker due to evicting objects with least local rewards (i.e., local synced victims)
        updateCacheManagerForLocalSyncedVictims_();

        return;
    }

    // (4.3) Update content directory information

    void CoveredCacheServerWorker::updateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->updateDirectoryTable(key, is_admit, directory_info, is_being_written);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        tmp_covered_cache_manager_ptr->updateVictimTrackerForSyncedVictimDirinfo(key, is_admit, directory_info);

        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory update
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        if (is_admit) // Admit a new key as local cached object
        {
            // Clear old local uncached popularity (TODO: preserved edge idx / bitmap) for the given key at soure edge node after admission
            tmp_covered_cache_manager_ptr->clearPopularityAggregatorAfterAdmission(key, current_edge_idx);
        }
        else // Evict a victim as local uncached object
        {
            // Prepare local uncached popularity of key for popularity aggregation
            Popularity local_uncached_popularity = 0.0;
            bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

            // Selective popularity aggregation
            tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
        }

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToUpdateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_directory_update_request_ptr = NULL;
        if (is_admit) // Try to admit a new key as local cached object (NOTE: local edge cache has NOT been admitted yet)
        {
            // ONLY need victim synchronization yet without popularity collection/aggregation
            covered_directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        }
        else // Evict a victim as local uncached object (NOTE: local edge cache has already been evicted)
        {
            // Prepare local uncached popularity of key for piggybacking-based popularity collection
            Popularity local_uncached_popularity = 0.0;
            bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata (due to selective metadata preservation)

            // Need BOTH popularity collection and victim synchronization
            covered_directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, CollectedPopularity(is_key_tracked, local_uncached_popularity), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        }
        assert(covered_directory_update_request_ptr != NULL);

        return covered_directory_update_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToUpdateBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);

        // Get is_being_written from control response message
        const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
        is_being_written = covered_directory_update_response_ptr->isBeingWritten();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_directory_update_response_ptr->getSourceIndex();
        const VictimSyncset& victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    // (6) covered-specific utility functions
        
    void CoveredCacheServerWorker::updateCacheManagerForLocalSyncedVictims_() const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Get victim cacheinfos of local synced victims for the current edge node
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalSyncedVictimCacheinfos();

        // Get directory info sets for local synced victimed beaconed by the current edge node
        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_local_synced_victim_dirinfosets;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victim_cacheinfos.begin(); cacheinfo_list_iter != local_synced_victim_cacheinfos.end(); cacheinfo_list_iter++)
        {
            const Key& tmp_key = cacheinfo_list_iter->getKey();
            bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(tmp_key);
            if (current_is_beacon) // Key is beaconed by current edge node
            {
                dirinfo_set_t tmp_dirinfo_set = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getLocalDirectoryInfos(tmp_key);
                local_beaconed_local_synced_victim_dirinfosets.insert(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        // Update local synced victims for the current edge node
        tmp_covered_cache_manager_ptr->updateVictimTrackerForLocalSyncedVictims(local_synced_victim_cacheinfos, local_beaconed_local_synced_victim_dirinfosets); 

        return;
    }
}
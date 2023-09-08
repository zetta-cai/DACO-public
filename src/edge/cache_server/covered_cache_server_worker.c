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
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, is_being_written, is_valid_directory_exist, directory_info);

        // Prepare local uncached popularity of key for popularity aggregation
        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory lookup
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

        // Selective popularity aggregation
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        Popularity local_uncached_popularity = 0.0;
        bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

        // Prepare CoveredDirectoryLookupRequest to check directory information in beacon node with popularity collection and victim synchronization
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* covered_directory_lookup_request_ptr = new CoveredDirectoryLookupRequest(key, is_key_tracked, CollectedPopularity(is_key_tracked, local_uncached_popularity), victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_directory_lookup_request_ptr != NULL);

        return covered_directory_lookup_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // TODO: Process directory lookup response for non-blocking admission placement deployment

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryLookupResponse);

        // Get directory information from the control response message
        const CoveredDirectoryLookupResponse* const covered_directory_lookup_response_ptr = static_cast<const CoveredDirectoryLookupResponse*>(control_response_ptr);
        is_being_written = covered_directory_lookup_response_ptr->isBeingWritten();
        is_valid_directory_exist = covered_directory_lookup_response_ptr->isValidDirectoryExist();
        directory_info = covered_directory_lookup_response_ptr->getDirectoryInfo();

        // Victim synchronization
        const VictimSyncset& victim_syncset = covered_directory_lookup_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

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

        bool affect_victim_tracker = false;
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->removeIfInvalidForGetrsp(key, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->updateIfInvalidForGetrsp(key, value, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            updateCacheManagerForLocalSyncedVictims_();
        }
        
        return is_local_cached_and_invalid;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool CoveredCacheServerWorker::acquireBeaconWritelock_(const Key& key, LockResult& lock_result, EventList& event_list, const bool& skip_propagation_latency)
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Piggyback local uncached popularity for key in current edge node
        return false;
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

    bool CoveredCacheServerWorker::releaseBeaconWritelock_(const Key& key, EventList& event_list, const bool& skip_propagation_latency)
    {
        // TODO: Piggyback candidate victims in current edge node

        // NOTE: NO need to piggyback local uncached popularity for key in current edge node, as it has been done by acquireBeaconWritelock_() (local uncached popularity is NOT changed when processing a single local data request)
        return false;
    }

    // (3) Process redirected requests

    /*bool CoveredCacheServerWorker::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        // TODO: Piggyback candidate victims in current edge node
        return false;
    }*/

    // (4.1) Admit uncached objects in local edge cache

    bool CoveredCacheServerWorker::tryToTriggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // std::ostringstream oss;
        // Util::dumpErrorMsg(instance_name_, "tryToTriggerIndependentAdmission_() should NOT be invoked in CoveredCacheServerWorker!");
        // exit(1);

        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        bool is_finish = false;
        return is_finish;
    }

    // (4.3) Update content directory information

    void CoveredCacheServerWorker::updateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        bool is_global_cached = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->updateDirectoryTable(key, is_admit, directory_info, is_being_written);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        covered_cache_manager_ptr->updateVictimTrackerForSyncedVictimDirinfo(key, is_admit, directory_info);

        // NOTE: NOT need piggyacking-based popularity collection and victim synchronization for local directory update
        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        if (is_admit) // Admit a new key as local cached object
        {
            // Clear old local uncached popularity (TODO: preserved edge idx / bitmap) for the given key at soure edge node after admission
            covered_cache_manager_ptr->clearPopularityAggregatorAfterAdmission(key, current_edge_idx);
        }
        else // Evict a victim as local uncached object
        {
            // Prepare local uncached popularity of key for popularity aggregation
            Popularity local_uncached_popularity = 0.0;
            bool is_key_tracked = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalUncachedPopularity(key, local_uncached_popularity); // If the local uncached key is tracked in local uncached metadata

            // Selective popularity aggregation
            covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, CollectedPopularity(is_key_tracked, local_uncached_popularity), is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
        }

        return;
    }

    MessageBase* CoveredCacheServerWorker::getReqToUpdateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

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
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);

        // Get is_being_written from control response message
        const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
        is_being_written = covered_directory_update_response_ptr->isBeingWritten();

        // Victim synchronization
        const VictimSyncset& victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = tmp_edge_wrapper_ptr->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    // (6) covered-specific utility functions
        
    void CoveredCacheServerWorker::updateCacheManagerForLocalSyncedVictims_() const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

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
        covered_cache_manager_ptr->updateVictimTrackerForLocalSyncedVictims(local_synced_victim_cacheinfos, local_beaconed_local_synced_victim_dirinfosets); 

        return;
    }
}
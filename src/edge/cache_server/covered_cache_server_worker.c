#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <list>
#include <sstream>

#include "common/util.h"
#include "core/victim/victim_cacheinfo.h"

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

    MessageBase* CoveredCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Piggyback local uncached popularity for key in current edge node (END HERE)

        /*checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Prepare directory lookup request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(directory_lookup_request_ptr != NULL);

        return directory_lookup_request_ptr;*/
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(const DynamicArray& control_response_msg_payload, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, EventList& event_list) const
    {
        // TODO: Process directory lookup response for non-blocking admission placement deployment

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

    bool CoveredCacheServerWorker::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // NOTE: If updateBeaconDirectory_() is admitting a local uncached object, beacon node has already removed local uncached popularities of plaecment nodes from aggregated uncached popularity after placement calculation, and only needs to assert node ID NOT exist in aggregate bitmap and remove the preserved node ID if any -> NO need to piggyback local uncached popularity
        // NOTE: If updateBeaconDirectory_() is evicting a local cached object, beacon node only needs to assert node ID NOT exist in aggregate bitmap -> NO need to piggyback local uncached popularity
        return false;
    }

    // (6) covered-specific utility functions
        
    void CoveredCacheServerWorker::updateCacheManagerForLocalSyncedVictims_() const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Get victim cacheinfos of local synced victims for the current edge node
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos = tmp_edge_wrapper_ptr->getEdgeCachePtr()->getLocalSyncedVictimCacheinfos();

        // Get directory info sets for local synced victimed beaconed by the current edge node
        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> beaconed_local_synced_victim_dirinfosets;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victim_cacheinfos.begin(); cacheinfo_list_iter != local_synced_victim_cacheinfos.end(); cacheinfo_list_iter++)
        {
            const Key& tmp_key = cacheinfo_list_iter->getKey();

            uint32_t beacon_edge_idx = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(tmp_key);
            if (beacon_edge_idx == current_edge_idx) // Key is beaconed by current edge node
            {
                dirinfo_set_t tmp_dirinfo_set = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getLocalDirectoryInfos(tmp_key);
                beaconed_local_synced_victim_dirinfosets.insert(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        // Update local synced victims for the current edge node
        tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr()->updateVictimTrackerForLocalSyncedVictims(local_synced_victim_cacheinfos, beaconed_local_synced_victim_dirinfosets); 

        return;
    }
}
#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <list>
#include <sstream>

#include "common/covered_weight.h"
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

    bool CoveredCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
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
        const bool sender_is_beacon = true; // Sender and beacon is the same edge node for placement calculation
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // NOTE: need_hybrid_fetching with best_placement_edgeset is processed in CacheServerWorkerBase::processLocalGetRequest_(), as we do NOT have value yet when lookuping directory information

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
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx); // Current edge node MUST NOT be the beacon edge node for the given key
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata

        // Prepare CoveredDirectoryLookupRequest to check directory information in beacon node with popularity collection and victim synchronization
        MessageBase* covered_directory_lookup_request_ptr = new CoveredDirectoryLookupRequest(key, collected_popularity, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(covered_directory_lookup_request_ptr != NULL);

        return covered_directory_lookup_request_ptr;
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        assert(control_response_ptr != NULL);
        const MessageType message_type = control_response_ptr->getMessageType();

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
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        // Update DirectoryCacher if necessary
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

    MessageBase* CoveredCacheServerWorker::getReqToRedirectGet_(const uint32_t& dst_edge_idx_for_compression, const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

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
        hitflag = covered_redirected_get_response_ptr->getHitflag();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_redirected_get_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_redirected_get_response_ptr->getVictimSyncsetRef();
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

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

    bool CoveredCacheServerWorker::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CacheWrapper* tmp_edge_cache_ptr = tmp_edge_wrapper_ptr->getEdgeCachePtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->removeIfInvalidForGetrsp(key, is_global_cached, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_cache_ptr->updateIfInvalidForGetrsp(key, value, is_global_cached, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }
        
        return is_local_cached_and_invalid;
    }

    // (1.5) Trigger cache placement for getrsp (ONLY for COVERED)

    bool CoveredCacheServerWorker::tryToTriggerCachePlacementForGetrsp_(const Key& key, const Value& value, const CollectedPopularity& collected_popularity_after_fetch_value, const FastPathHint& fast_path_hint, const bool& is_global_cached, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        assert(collected_popularity_after_fetch_value.isTracked()); // MUST be tracked (actually newly-tracked) after fetching value from neighbor/cloud

        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
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

            // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching (ONLY for COVERED)
            if (need_hybrid_fetching)
            {
                assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
                tmp_edge_wrapper_ptr->nonblockNotifyForPlacement(key, value, best_placement_edgeset, skip_propagation_latency);
            }
        }
        #ifdef ENABLE_FAST_PATH_PLACEMENT
        else if (fast_path_hint.isValid()) // Trigger fast-path single-placement calculation if with valid FastPathHint
        {
            assert(!current_is_beacon); // ONLY remote beacon can provide FastPathHint to sender edge node

            // NOTE: although passed param (is_global_cached) is looked up before fetching value, which may be stale, we still use it due to approximate fast-path placement, instead of looking up the latest is_global_cached from remote beacon edge node, which will introduce extra message overhead

            // Get weight parameters from static class atomically
            const WeightInfo weight_info = CoveredWeight::getWeightInfo();
            const Weight local_hit_weight = weight_info.getLocalHitWeight();
            const Weight cooperative_hit_weight = weight_info.getCooperativeHitWeight();

            // Calculate local admission benefit
            DeltaReward local_admission_benefit = 0.0;
            Popularity local_uncached_popularity = collected_popularity_after_fetch_value.getLocalUncachedPopularity();
            if (is_global_cached) // Key is global cached
            {
                Weight w1_minux_w2 = Util::popularityNonegMinus(local_hit_weight, cooperative_hit_weight);
                local_admission_benefit = Util::popularityMultiply(w1_minux_w2, local_uncached_popularity); // w1 - w2
            }
            else // Key is NOT global cached
            {
                DeltaReward tmp_local_admission_benefit0 = Util::popularityMultiply(local_hit_weight, local_uncached_popularity); // w1

                // NOTE: sum_local_uncached_popularity MUST NOT include local uncached popularity of the current edge node
                DeltaReward tmp_local_admission_benefit1 = Util::popularityMultiply(cooperative_hit_weight, fast_path_hint.getSumLocalUncachedPopularity()); // w2

                local_admission_benefit = Util::popularityAdd(tmp_local_admission_benefit0, tmp_local_admission_benefit1);
            }

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
                    std::unordered_map<Key, DirinfoSet, KeyHasher> tmp_local_beaconed_local_cached_victim_dirinfoset = tmp_cooperation_wrapper_ptr->getLocalBeaconedVictimsFromCacheinfos(tmp_local_cached_victim_cacheinfos);

                    // Calculate local eviction cost by VictimTracker (to utilize perkey_victim_dirinfo_)
                    local_eviction_cost = tmp_covered_cache_manager_ptr->accessVictimTrackerForFastPathEvictionCost(tmp_local_cached_victim_cacheinfos, tmp_local_beaconed_local_cached_victim_dirinfoset);
                }

                #ifdef ENABLE_TEMPORARY_DUPLICATION_AVOIDANCE
                // Enforce duplication avoidance when cache is not full
                if (is_global_cached && local_eviction_cost == 0.0)
                {
                    local_admission_benefit = 0.0;
                }
                #endif

                // Trigger local cache placement if local admission benefit is larger than local eviction cost
                if (local_admission_benefit > local_eviction_cost)
                {
                    // Admit dirinfo into remote beacon edge node
                    bool tmp_is_being_written = false;
                    const bool is_background = true; // Similar as only-sender hybrid data fetching
                    bool is_neighbor_cached = false; // NOTE: we do NOT use cooperation wrapper to check is_neighbor_cached, as sender must NOT be the beacon here
                    is_finish = tmp_cache_server_ptr->admitBeaconDirectory_(key, DirectoryInfo(tmp_edge_wrapper_ptr->getNodeIdx()), tmp_is_being_written, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // TODO: Set is_neighbor_cached in admitBeaconDirectory_()
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

    // (2.1) Acquire write lock and block for MSI protocol

    bool CoveredCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
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

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local acquire write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
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
        const bool sender_is_beacon = true; // Sender and beacon is the same edge node for placement calculation
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
    
        // NOTE: local acquire writelock MUST NOT need hybrid data fetching due to NO need for placement calculation (newly-admitted cache copies will still be invalid after cache placement)
        assert(!is_finish); // NO victim fetching due to NO placement calculation
        assert(best_placement_edgeset.size() == 0); // NO placement deployment due to NO placement calculation
        assert(!need_hybrid_fetching); // Also NO hybrid data fetching

        return is_finish;
    }

    MessageBase* CoveredCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx); // Current edge node MUST NOT be the beacon edge node for the given key
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: we NEED popularity aggregation for accumulated changes on local uncached popularity due to directory metadata cache
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

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
        const VictimSyncset& neighbor_victim_syncset = covered_acquire_writelock_response_ptr->getVictimSyncsetRef();
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        return;
    }

    void CoveredCacheServerWorker::processReqToFinishBlock_(MessageBase* control_request_ptr) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredFinishBlockRequest);

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Victim synchronization
        const CoveredFinishBlockRequest* const covered_finish_block_request_ptr = static_cast<const CoveredFinishBlockRequest*>(control_request_ptr);
        const uint32_t source_edge_idx = covered_finish_block_request_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_finish_block_request_ptr->getVictimSyncsetRef();
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

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
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
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
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_udpate = tmp_edge_wrapper_ptr->getEdgeCachePtr()->update(key, value, is_global_cached, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }

        return is_local_cached_after_udpate;
    }

    bool CoveredCacheServerWorker::removeLocalEdgeCache_(const Key& key, const bool& is_global_cached) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_remove = tmp_edge_wrapper_ptr->getEdgeCachePtr()->remove(key, is_global_cached, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            tmp_edge_wrapper_ptr->updateCacheManagerForLocalSyncedVictims();
        }

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    bool CoveredCacheServerWorker::releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency)
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

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for local release write lock on local cached objects, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
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
        const bool sender_is_beacon = true; // Sender and beacon is the same edge node for placement calculation
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        is_finish = tmp_covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(key, current_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, tmp_edge_wrapper_ptr, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usgae, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching (ONLY for COVERED)
        if (need_hybrid_fetching)
        {
            assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
            tmp_edge_wrapper_ptr->nonblockNotifyForPlacement(key, value, best_placement_edgeset, skip_propagation_latency);
        }

        return is_finish;
    }

    MessageBase* CoveredCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx); // Current edge node MUST NOT be the beacon edge node for the given key
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        // NOTE: although getReqToAcquireBeaconWritelock_() has aggregated accumulated changes of local uncached popularity due to directory metadata cache, we still NEED popularity aggregation as local uncached metadata may be updated before releasing the write lock if the global cached key is local uncached
        CollectedPopularity collected_popularity;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ is false if the given key is local cached or the key is local uncached yet NOT tracked in local uncached metadata

        MessageBase* covered_release_writelock_request_ptr = new CoveredReleaseWritelockRequest(key, collected_popularity, victim_syncset, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
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
        CacheServer* tmp_cache_server_ptr = cache_server_worker_param_ptr_->getCacheServerPtr();
        EdgeWrapper* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Victim synchronization
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, tmp_edge_wrapper_ptr->getCooperationWrapperPtr());

        // Do nothing for CoveredReleaseWritelockResponse, yet notify beacon for non-blocking placement notification for CoveredPlacementReleaseWritelockResponse after hybrid data fetching
        if (need_hybrid_fetching)
        {
            // Trigger placement notification remotely at the beacon edge node
            is_finish = tmp_cache_server_ptr->notifyBeaconForPlacementAfterHybridFetch_(tmp_key, value, best_placement_edgeset, edge_cache_server_worker_recvrsp_source_addr_, edge_cache_server_worker_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency);
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
}
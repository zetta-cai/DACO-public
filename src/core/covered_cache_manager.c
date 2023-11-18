#include "core/covered_cache_manager.h"

#include <sstream>

#include "message/control_message.h"

namespace covered
{
    const std::string CoveredCacheManager::kClassName("CoveredCacheManager");

    CoveredCacheManager::CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& topk_edgecnt) : edge_idx_(edge_idx), topk_edgecnt_(topk_edgecnt), popularity_aggregator_(edge_idx, edgecnt, popularity_aggregation_capacity_bytes, topk_edgecnt), victim_tracker_(edge_idx, peredge_synced_victimcnt, peredge_monitored_victimsetcnt), directory_cacher_(edge_idx, popularity_collection_change_ratio)
    {
        // Differentiate different edge nodes
        std::stringstream ss;
        ss << kClassName << " edge" << edge_idx;
        instance_name_ = ss.str();
    }
    
    CoveredCacheManager::~CoveredCacheManager() {}

    // For popularity aggregation

    bool CoveredCacheManager::updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, const bool& need_placement_calculation, const bool& sender_is_beacon, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, FastPathHint* fast_path_hint_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "beginning of updatePopularityAggregatorForAggregatedPopularity for key", key.getKeystr().c_str());
        
        bool is_finish = false;
        bool has_best_placement = false;
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // Double-check preserved edgeset to set is global cached flag to avoid over-estimating (max) admission benefit
        bool tmp_is_global_cached = is_global_cached;
        if (!is_global_cached)
        {
            // Set is_global_cached to true if key is being admitted
            bool is_being_admitted = popularity_aggregator_.isKeyBeingAdmitted(key);
            if (is_being_admitted)
            {
                tmp_is_global_cached = true;
            }
        }

        FastPathHint tmp_fast_path_hint;
        popularity_aggregator_.updateAggregatedUncachedPopularity(key, source_edge_idx, collected_popularity, tmp_is_global_cached, is_source_cached, tmp_fast_path_hint);
        #ifdef ENABLE_FAST_PATH_PLACEMENT
        if (fast_path_hint_ptr != NULL)
        {
            *fast_path_hint_ptr = tmp_fast_path_hint;
        }
        #endif

        #ifdef DEBUG_COVERED_CACHE_MANAGER
        Util::dumpVariablesForDebug(instance_name_, 10, "updatePopularityAggregatorForAggregatedPopularity() for key", key.getKeystr().c_str(), "is_gobal_cached:", Util::toString(is_global_cached).c_str(), "tmp_is_global_cached:", Util::toString(tmp_is_global_cached).c_str(), "need_placement_calculation:", Util::toString(need_placement_calculation).c_str(), "is_tracked_by_source_edge_node:", Util::toString(collected_popularity.isTracked()).c_str());
        #endif
        
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid after cache placement
        if (need_placement_calculation)
        {
            const bool is_tracked_by_source_edge_node = collected_popularity.isTracked();
            // NOTE: NO need to perform placement calculation if key is NOT tracked by source edge node, as removing old local uncached popularity if any will NEVER increase admission benefit
            if (is_tracked_by_source_edge_node)
            {
                // Perform greedy-based placement calculation for trade-off-aware cache placement
                // NOTE: set best_placement_edgeset for preserved edgeset and placement notifications; set best_placement_peredge_synced_victimset for synced victim removal from victim tracker and best_placement_peredge_fetched_victimset for fetched victim removal from victim cache, to avoid duplicate eviction (all for non-blocking placement deployment)
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> best_placement_peredge_synced_victimset;
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> best_placement_peredge_fetched_victimset;
                is_finish = placementCalculation_(key, tmp_is_global_cached, has_best_placement, best_placement_edgeset, best_placement_peredge_synced_victimset, best_placement_peredge_fetched_victimset, edge_wrapper_ptr, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency);
                assert(best_placement_edgeset.size() <= topk_edgecnt_); // At most k placement edge nodes each time
                if (is_finish)
                {
                    return is_finish; // Edge node is NOT running now
                }

                #ifdef DEBUG_COVERED_CACHE_MANAGER
                Util::dumpVariablesForDebug(instance_name_, 6, "after placement calculation for key", key.getKeystr().c_str(), "has_best_placement:", Util::toString(has_best_placement).c_str(), "best_placement_edgeset:", best_placement_edgeset.toString().c_str());
                #endif

                if (has_best_placement)
                {
                    // Preserve placement edgeset + perform local-uncached-popularity removal to avoid duplicate placement, and update max admission benefit with aggregated uncached popularity for non-blocking placement deployment
                    popularity_aggregator_.updatePreservedEdgesetForPlacement(key, best_placement_edgeset, tmp_is_global_cached);

                    // Remove involved synced victims from victim tracker for each edge node in placement edgeset to avoid duplicate eviction
                    // NOTE: synced victims MUST exist in edge-level victim metadata of victim tracker
                    // NOTE: removed synced victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node
                    for (std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>::const_iterator best_placement_peredge_synced_victimset_const_iter = best_placement_peredge_synced_victimset.begin(); best_placement_peredge_synced_victimset_const_iter != best_placement_peredge_synced_victimset.end(); best_placement_peredge_synced_victimset_const_iter++)
                    {
                        const uint32_t tmp_edge_idx = best_placement_peredge_synced_victimset_const_iter->first;
                        const std::unordered_set<Key, KeyHasher>& tmp_synced_victim_keyset_const_ref = best_placement_peredge_synced_victimset_const_iter->second;
                        victim_tracker_.removeVictimsForGivenEdge(tmp_edge_idx, tmp_synced_victim_keyset_const_ref);
                    }

                    // TODO: Remove involved extra fetched victims from victim cache for each edge node in placement edgeset to avoid duplicate eviction
                    // NOTE: extra fetched victims may NOT exist in victim cache
                    // NOTE: removed extra fetched victims should NOT be reused <- if the edge node needs more victims and triggers victim fetching again, the request MUST be later than placement notification request, which has changed the synced victims in the edge node
                    UNUSED(best_placement_peredge_fetched_victimset);

                    // Non-blocking data fetching if with best placement
                    edge_wrapper_ptr->nonblockDataFetchForPlacement(key, best_placement_edgeset, skip_propagation_latency, sender_is_beacon, need_hybrid_fetching);
                }
            }
        }

        return is_finish;
    }

    void CoveredCacheManager::clearPopularityAggregatorForPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        popularity_aggregator_.clearPreservedEdgesetAfterAdmission(key, source_edge_idx);

        // NOTE: all old local uncached popularities have already been cleared right after placement calculation in updatePreservedEdgesetForPlacement()
        // (1) NO need to clear old local uncached popularity for the source edge node
        //updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, false, 0.0, 0, true);
        // (2) Assert old local uncached popularity for the source edge node MUST NOT exist
        assertNoLocalUncachedPopularity(key, source_edge_idx);

        return;
    }

    void CoveredCacheManager::assertNoLocalUncachedPopularity(const Key& key, const uint32_t& source_edge_idx) const
    {
        // Assert old local uncached popularity for the source edge node MUST NOT exist
        AggregatedUncachedPopularity tmp_aggregated_uncached_popularity;
        bool has_aggregated_uncached_popularity = popularity_aggregator_.getAggregatedUncachedPopularity(key, tmp_aggregated_uncached_popularity);
        if (has_aggregated_uncached_popularity)
        {
            assert(tmp_aggregated_uncached_popularity.hasLocalUncachedPopularity(source_edge_idx) == false);
        }
        return;
    }

    // For victim synchronization

    void CoveredCacheManager::updateVictimTrackerForLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const CooperationWrapperBase* cooperation_wrapper_ptr)
    {
        // NOTE: victim cacheinfos of local_synced_victim_cacheinfos and victim dirinfo sets of local_beaconed_local_synced_victim_dirinfosets MUST be complete
        victim_tracker_.updateLocalSyncedVictims(local_cache_margin_bytes, local_synced_victim_cacheinfos, cooperation_wrapper_ptr);
        return;
    }

    void CoveredCacheManager::updateVictimTrackerForLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        victim_tracker_.updateLocalBeaconedVictimDirinfo(key, is_admit, directory_info);
        return;
    }

    VictimSyncset CoveredCacheManager::accessVictimTrackerForLocalVictimSyncset(const uint32_t& dst_edge_idx_for_compression, const uint64_t& latest_local_cache_margin_bytes) const
    {
        // Get current complete/compressed victim syncset from victim tracker
        // NOTE: we perform compression inside VictimTrackker:getLocalVictimSyncsetForSynchronization() for atomicity
        VictimSyncset current_victim_syncset = victim_tracker_.getLocalVictimSyncsetForSynchronization(dst_edge_idx_for_compression, latest_local_cache_margin_bytes);

        return current_victim_syncset;
    }

    void CoveredCacheManager::updateVictimTrackerForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset, const CooperationWrapperBase* cooperation_wrapper_ptr)
    {
        // NOTE: victim cacheinfos and dirinfo sets of neighbor_victim_syncset can be either complete or compressed; while dirinfo sets of local_beaconed_neighbor_synced_victim_dirinfosets MUST be complete

        // NOTE: we perform recovery inside VictimTracker::updateForNeighborVictimSyncset() for atomicity
        victim_tracker_.updateForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, cooperation_wrapper_ptr);
        return;
    }

    // For directory metadata cache

    bool CoveredCacheManager::accessDirectoryCacherForCachedDirectory(const Key& key, CachedDirectory& cached_directory) const
    {
        bool has_cached_directory = directory_cacher_.getCachedDirectory(key, cached_directory);
        return has_cached_directory;
    }

    bool CoveredCacheManager::accessDirectoryCacherToCheckPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const
    {
        bool has_cached_directory = directory_cacher_.checkPopularityChange(key, local_uncached_popularity, cached_directory, is_large_popularity_change);
        return has_cached_directory;
    }

    void CoveredCacheManager::updateDirectoryCacherToRemoveCachedDirectory(const Key& key)
    {
        directory_cacher_.removeCachedDirectoryIfAny(key);
        return;
    }

    void CoveredCacheManager::updateDirectoryCacherForNewCachedDirectory(const Key& key, const CachedDirectory& cached_directory)
    {
        directory_cacher_.updateForNewCachedDirectory(key, cached_directory);
        return;
    }

    // For fast-path single-placement calculation in current edge node (NOT as a beacon node)

    DeltaReward CoveredCacheManager::accessVictimTrackerForFastPathEvictionCost(const std::list<VictimCacheinfo>& curedge_local_cached_victim_cacheinfos, const std::unordered_map<Key, DirinfoSet, KeyHasher>& curedge_local_beaconed_local_cached_victim_dirinfosets) const
    {
        return victim_tracker_.calcEvictionCostForFastPathPlacement(curedge_local_cached_victim_cacheinfos, curedge_local_beaconed_local_cached_victim_dirinfosets);
    }

    uint64_t CoveredCacheManager::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += victim_tracker_.getSizeForCapacity();

        return total_size;
    }

    bool CoveredCacheManager::placementCalculation_(const Key& key, const bool& is_global_cached, bool& has_best_placement, Edgeset& best_placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_synced_victimset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_fetched_victimset, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        bool is_finish = false;
        has_best_placement = false;
        
        // For lazy victim fetching before non-blocking placement deployment
        bool need_victim_fetching = false;
        DeltaReward best_placement_admission_benefit = 0.0;
        Edgeset tmp_best_placement_edgeset; // For preserved edgeset and placement notifications under non-blocking placement deployment
        std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_best_placement_peredge_synced_victimset; // For synced victim removal under non-blocking placement deployment
        std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_best_placement_peredge_fetched_victimset; // For fetched victim removal under non-blocking placement deployment
        Edgeset best_placement_victim_fetch_edgeset;

        AggregatedUncachedPopularity tmp_aggregated_uncached_popularity;
        bool has_aggregated_uncached_popularity = popularity_aggregator_.getAggregatedUncachedPopularity(key, tmp_aggregated_uncached_popularity);

        // Perform placement calculation ONLY if key is still tracked by popularity aggregator (i.e., belonging to a global popular uncached object)
        if (has_aggregated_uncached_popularity)
        {
            const ObjectSize tmp_object_size = tmp_aggregated_uncached_popularity.getObjectSize();
            const uint32_t tmp_topk_list_length = tmp_aggregated_uncached_popularity.getTopkListLength();
            assert(tmp_topk_list_length > 0); // NOTE: we perform placement calculation only when add/update a new local uncached popularity -> at least one local uncached popularity in the top-k list
            assert(tmp_topk_list_length <= popularity_aggregator_.getTopkEdgecnt()); // At most EdgeCLI::covered_topk_edgecnt_ times

            // Greedy-based placement calculation
            PlacementGain max_placement_gain = 0.0;
            for (uint32_t topicnt = 1; topicnt <= tmp_topk_list_length; topicnt++)
            {
                // Consider topi edge nodes ordered by local uncached popularity in a descending order

                // Calculate admission benefit if we place the object with is_global_cached flag into topi edge nodes
                Edgeset tmp_placement_edgeset;
                //const DeltaReward tmp_admission_benefit = tmp_aggregated_uncached_popularity.calcAdmissionBenefit(topicnt, is_global_cached, tmp_placement_edgeset);
                const DeltaReward tmp_admission_benefit = tmp_aggregated_uncached_popularity.calcAdmissionBenefit(edge_idx_, key, topicnt, is_global_cached, tmp_placement_edgeset); // TMPDEBUG23

                // Calculate eviction cost based on tmp_placement_edgeset
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_placement_peredge_synced_victimset;
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_placement_peredge_fetched_victimset;
                Edgeset tmp_placement_victim_fetch_edgeset;
                const DeltaReward tmp_eviction_cost = victim_tracker_.calcEvictionCost(tmp_object_size, tmp_placement_edgeset, tmp_placement_peredge_synced_victimset, tmp_placement_peredge_fetched_victimset, tmp_placement_victim_fetch_edgeset); // NOTE: tmp_eviction_cost may be partial eviction cost if without enough victims

                // Calculate placement gain (admission benefit - eviction cost)
                const DeltaReward tmp_placement_gain = tmp_admission_benefit - tmp_eviction_cost;
                if ((topicnt == 1) || (tmp_placement_gain > max_placement_gain))
                {
                    max_placement_gain = tmp_placement_gain;

                    best_placement_admission_benefit = tmp_admission_benefit;
                    tmp_best_placement_edgeset = tmp_placement_edgeset;
                    tmp_best_placement_peredge_synced_victimset = tmp_placement_peredge_synced_victimset;
                    tmp_best_placement_peredge_fetched_victimset = tmp_placement_peredge_fetched_victimset;
                    best_placement_victim_fetch_edgeset = tmp_placement_victim_fetch_edgeset;
                }
            }

            // Update best placement if any
            if (max_placement_gain > 0.0)
            {
                if (best_placement_victim_fetch_edgeset.size() == 0) // NO need for lazy victim fetching
                {
                    has_best_placement = true;
                    best_placement_edgeset = tmp_best_placement_edgeset;
                    best_placement_peredge_synced_victimset = tmp_best_placement_peredge_synced_victimset;
                    best_placement_peredge_fetched_victimset = tmp_best_placement_peredge_fetched_victimset;
                }
                else // Need lazy victim fetching
                {
                    // Fetch victims ONLY if admission benefit > partial eviction cost under the best placement
                    need_victim_fetching = true;
                }
            }
        }

        // Lazy victim fetching before non-blocking placement deployment
        // NOTE: MINOR cases ONLY if cache margin bytes + size of victims < object size and admission benefit > partial eviction cost (actually, a small per-edge victim cnt is sufficient for most admissions)
        if (need_victim_fetching)
        {
            assert(has_aggregated_uncached_popularity == true);
            const ObjectSize tmp_object_size = tmp_aggregated_uncached_popularity.getObjectSize();

            // TMPDEBUGTMPDEBUG
            Util::dumpVariablesForDebug(instance_name_, 4, "beginning of parallelFetchVictims_ for key", key.getKeystr().c_str(), "best_placement_victim_fetch_edgeset:", best_placement_victim_fetch_edgeset.toString().c_str());

            // Issue CoveredVictimFetchRequest to fetch more victims in parallel (note that CoveredVictimFetchRequest is a foreground message before non-blocking placement deployment)
            // TODO: Maintain a small vicitm cache in each beacon edge node if with frequent lazy victim fetching to avoid degrading directory lookup performance
            std::unordered_map<uint32_t, std::list<VictimCacheinfo>> extra_peredge_victim_cacheinfos;
            std::unordered_map<Key, DirinfoSet, KeyHasher> extra_perkey_victim_dirinfoset;
            is_finish = parallelFetchVictims_(tmp_object_size, best_placement_victim_fetch_edgeset, edge_wrapper_ptr, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, extra_peredge_victim_cacheinfos, extra_perkey_victim_dirinfoset);
            if (is_finish)
            {
                return is_finish; // Edge node is NOT running now
            }

            if (extra_peredge_victim_cacheinfos.size() > 0) // With extra fetched victims
            {
                // Re-calculate eviction cost with extra fetched victims
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_placement_peredge_synced_victimset;
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>> tmp_placement_peredge_fetched_victimset;
                Edgeset tmp_placement_victim_fetch_edgeset;
                const DeltaReward tmp_eviction_cost = victim_tracker_.calcEvictionCost(tmp_object_size, tmp_best_placement_edgeset, tmp_placement_peredge_synced_victimset, tmp_placement_peredge_fetched_victimset, tmp_placement_victim_fetch_edgeset, extra_peredge_victim_cacheinfos, extra_perkey_victim_dirinfoset); // NOTE: tmp_eviction_cost may be partial eviction cost if without enough victims
                assert(tmp_placement_victim_fetch_edgeset.size() == 0); // NOTE: we DISABLE recursive victim fetching

                // Double-check the best placement
                const DeltaReward tmp_placement_gain = best_placement_admission_benefit - tmp_eviction_cost;
                if (tmp_placement_gain > 0.0) // Still with positive placement gain
                {
                    has_best_placement = true;
                    best_placement_edgeset = tmp_best_placement_edgeset; // tmp_best_placement_edgeset is the best placement
                    best_placement_peredge_synced_victimset = tmp_placement_peredge_synced_victimset; // For synced victim removal
                    best_placement_victim_fetch_edgeset = tmp_placement_victim_fetch_edgeset; // For fetched victim removal
                }
            }
        }

        return is_finish;
    }

    bool CoveredCacheManager::parallelFetchVictims_(const ObjectSize& object_size, const Edgeset& best_placement_victim_fetch_edgeset, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos, std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset) const
    {
        const uint32_t victim_fetch_edgecnt = best_placement_victim_fetch_edgeset.size();
        assert(victim_fetch_edgecnt > 0); // At least one edge node for victim fetching
        assert(edge_wrapper_ptr != NULL);
        assert(recvrsp_socket_server_ptr != NULL);

        extra_peredge_victim_cacheinfos.clear(); // NOTE: extra fetched victim cacheinfos MUST be complete
        extra_perkey_victim_dirinfoset.clear(); // NOTE: extra fetched victim dirinfo sets MUST be complete

        bool is_finish = false;
        struct timespec victim_fetch_start_timestamp = Util::getCurrentTimespec();

        // Track whether victim fetch requests to all involved edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::unordered_map<uint32_t, bool> acked_flags;
        for (std::unordered_set<uint32_t>::const_iterator iter_for_ackflag = best_placement_victim_fetch_edgeset.begin(); iter_for_ackflag != best_placement_victim_fetch_edgeset.end(); iter_for_ackflag++)
        {
            acked_flags.insert(std::pair<uint32_t, bool>(*iter_for_ackflag, false));
        }

        // Issue all victim fetch requests simultaneously
        const uint32_t current_edge_idx = edge_wrapper_ptr->getNodeIdx();
        while (acked_edgecnt != victim_fetch_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (victim_fetch_edgecnt - acked_edgecnt) control requests to each involved edge node that has not acknowledged victim fetch request
            for (std::unordered_map<uint32_t, bool>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second) // Skip the edge node that has acknowledged the victim fetch request
                {
                    continue;
                }

                const uint32_t tmp_edge_idx = iter_for_request->first; // Edge node index of an involved edge node that has not acknowledged victim fetch request
                if (tmp_edge_idx == current_edge_idx) // Local victim fetching
                {
                    // Get victim cacheinfos from local edge cache for object size
                    // NOTE: extra fetched victim cacheinfos from local edge cache MUST be complete
                    std::list<VictimCacheinfo> tmp_victim_cacheinfos;
                    bool has_victim_key = edge_wrapper_ptr->getEdgeCachePtr()->fetchVictimCacheinfosForRequiredSize(tmp_victim_cacheinfos, object_size);
                    assert(has_victim_key == true);

                    // Update extra_peredge_victim_cacheinfos
                    extra_peredge_victim_cacheinfos.insert(std::pair<uint32_t, std::list<VictimCacheinfo>>(tmp_edge_idx, tmp_victim_cacheinfos));

                    // Update extra_perkey_victim_dirinfoset
                    // NOTE: extra fetched victim dirinfo sets from local directory table MUST be complete
                    std::unordered_map<Key, DirinfoSet, KeyHasher> local_beaconed_local_fetched_victim_dirinfosets = edge_wrapper_ptr->getCooperationWrapperPtr()->getLocalBeaconedVictimsFromCacheinfos(tmp_victim_cacheinfos);
                    for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator tmp_victim_dirinfosets_const_iter = local_beaconed_local_fetched_victim_dirinfosets.begin(); tmp_victim_dirinfosets_const_iter != local_beaconed_local_fetched_victim_dirinfosets.end(); tmp_victim_dirinfosets_const_iter++)
                    {
                        const Key& tmp_victim_key = tmp_victim_dirinfosets_const_iter->first;
                        if (extra_perkey_victim_dirinfoset.find(tmp_victim_key) == extra_perkey_victim_dirinfoset.end())
                        {
                            extra_perkey_victim_dirinfoset.insert(std::pair<Key, DirinfoSet>(tmp_victim_key, tmp_victim_dirinfosets_const_iter->second));
                        }
                    }

                    // Update ack information
                    assert(!acked_flags[tmp_edge_idx]);
                    acked_flags[tmp_edge_idx] = true;
                    acked_edgecnt += 1;
                }
                else // Remote victim fetching
                {
                    NetworkAddr target_edge_cache_server_recvreq_dst_addr = edge_wrapper_ptr->getTargetDstaddr(DirectoryInfo(tmp_edge_idx)); // Send to cache server of the target edge node for victim fetch processor
                    sendVictimFetchRequest_(tmp_edge_idx, object_size, edge_wrapper_ptr, recvrsp_source_addr, target_edge_cache_server_recvreq_dst_addr, skip_propagation_latency);
                }
            } // End of edgeidx_for_request

            // Receive (blocked_edgecnt - acked_edgecnt) control repsonses from the closest edge nodes
            const uint32_t expected_rspcnt = victim_fetch_edgecnt - acked_edgecnt;
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < expected_rspcnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    if (!edge_wrapper_ptr->isNodeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for CoveredVictimFetchResponse");
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kCoveredVictimFetchResponse);
                    uint32_t tmp_edge_idx = control_response_ptr->getSourceIndex();

                    // Mark the edge node has acknowledged the victim fetch request
                    bool is_match = false;
                    for (std::unordered_map<uint32_t, bool>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_edge_idx) // Match an edge node requiring victim fetching
                        {
                            assert(iter_for_response->second == false); // Original ack flag should be false

                            processVictimFetchResponse_(control_response_ptr, edge_wrapper_ptr, extra_peredge_victim_cacheinfos, extra_perkey_victim_dirinfoset);

                            // Update total bandwidth usage for received victim fetch response
                            BandwidthUsage victim_fetch_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_victim_fetch_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                            victim_fetch_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_victim_fetch_rsp_bandwidth_bytes, 0));
                            total_bandwidth_usage.update(victim_fetch_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one edge node
                    {
                        std::ostringstream oss;
                        oss << "receive VictimFetchResponse from edge node " << tmp_edge_idx << ", which is NOT in the victim fetch edgeset!";
                        Util::dumpWarnMsg(instance_name_, oss.str());
                    } // End of !is_match

                    // Release the control response message
                    delete control_response_ptr;
                    control_response_ptr = NULL;
                } // End of (is_timeout == false)
            } // End of edgeidx_for_response

            if (is_finish) // Edge node is NOT running now
            {
                break;
            }
        } // End of while(acked_edgecnt != blocked_edgecnt)

        // Add intermediate event if with event tracking
        struct timespec victim_fetch_end_timestamp = Util::getCurrentTimespec();
        uint32_t victim_fetch_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(victim_fetch_end_timestamp, victim_fetch_start_timestamp));
        event_list.addEvent(Event::EDGE_VICTIM_FETCH_EVENT_NAME, victim_fetch_latency_us);

        return is_finish;
    }

    void CoveredCacheManager::sendVictimFetchRequest_(const uint32_t& dst_edge_idx_for_compression, const ObjectSize& object_size, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, const NetworkAddr& edge_cache_server_recvreq_dst_addr, const bool& skip_propagation_latency) const
    {
        assert(edge_wrapper_ptr != NULL);

        // Prepare victim syncset for piggybacking-based victim synchronization
        // NOTE: need to fetch more victims means that the cache of the dst edge node does NOT have sufficient space for the newly-admited object (i.e., NOT empty), so there MUST exist the corresponding edge-level victim metadata for the dst edge node -> while we still need to provide cache margin bytes of the current edge node, as we are getting victim syncset of the current edge node, which may NOT have edge-level metadata
        VictimSyncset victim_syncset = edge_wrapper_ptr->getCoveredCacheManagerPtr()->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare victim fetch request to fetch victims from the target edge node
        const uint32_t current_edge_idx = edge_wrapper_ptr->getNodeIdx();
        MessageBase* victim_fetch_request_ptr = new CoveredVictimFetchRequest(object_size, victim_syncset, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
        assert(victim_fetch_request_ptr != NULL);

        // Push CoveredVictimFetchRequest into edge-to-edge propagation simulator to send to the target edge node
        bool is_successful = edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(victim_fetch_request_ptr, edge_cache_server_recvreq_dst_addr);
        assert(is_successful);

        // NOTE: victim_fetch_request_ptr will be released by edge-to-edge propagation simulator
        victim_fetch_request_ptr = NULL;

        return;
    }

    void CoveredCacheManager::processVictimFetchResponse_(const MessageBase* control_respnose_ptr, const EdgeWrapper* edge_wrapper_ptr, std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos, std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset) const
    {
        assert(control_respnose_ptr != NULL);
        assert(control_respnose_ptr->getMessageType() == MessageType::kCoveredVictimFetchResponse);
        assert(edge_wrapper_ptr != NULL);

        const CoveredVictimFetchResponse* const covered_victim_fetch_response_ptr = static_cast<const CoveredVictimFetchResponse*>(control_respnose_ptr);
        CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Victim synchronization
        const uint32_t source_edge_idx = covered_victim_fetch_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_victim_fetch_response_ptr->getVictimSyncsetRef();
        tmp_covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr->getCooperationWrapperPtr());

        // NOTE: cache margin bytes of victim_fetchset will NOT be used
        const VictimSyncset& victim_fetchset = covered_victim_fetch_response_ptr->getVictimFetchsetRef();
        assert(victim_fetchset.isComplete()); // NOTE: extra fetched victim cacheinfos and dirinfo sets in victim fetchset MUST be complete

        // Update extra_peredge_victim_cacheinfos
        std::list<VictimCacheinfo> fetched_victim_cacheinfos;
        bool with_complete_victim_syncset = victim_fetchset.getLocalSyncedVictims(fetched_victim_cacheinfos);
        assert(with_complete_victim_syncset == true); // NOTE: extra fetched victim cacheinfos in victim fetchset MUST be complete
        extra_peredge_victim_cacheinfos.insert(std::pair<uint32_t, std::list<VictimCacheinfo>>(source_edge_idx, fetched_victim_cacheinfos));

        // Update extra_perkey_victim_dirinfoset
        std::unordered_map<Key, DirinfoSet, KeyHasher> neighbor_beaconed_neighbor_fetched_victim_dirinfosets;
        with_complete_victim_syncset = victim_fetchset.getLocalBeaconedVictims(neighbor_beaconed_neighbor_fetched_victim_dirinfosets); // Neighbor beaconed ones of neighbor fetched victims
        assert(with_complete_victim_syncset == true); // NOTE: extra fetched victim dirinfo sets in victim fetchset MUST be complete
        const std::unordered_map<Key, DirinfoSet, KeyHasher> local_beaconed_neighbor_fetched_victim_dirinfosets = edge_wrapper_ptr->getCooperationWrapperPtr()->getLocalBeaconedVictimsFromVictimSyncset(victim_fetchset); // Local beaconed ones of fetched victims (dirinfo sets MUST be complete)
        // Insert neighbor beaconed neighbor fetched victims
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator victim_dirinfosets_const_iter = neighbor_beaconed_neighbor_fetched_victim_dirinfosets.begin(); victim_dirinfosets_const_iter != neighbor_beaconed_neighbor_fetched_victim_dirinfosets.end(); victim_dirinfosets_const_iter++)
        {
            // NOTE: extra fetched victim dirinfo sets from neighbor MUST be complete
            assert(victim_dirinfosets_const_iter->second.isComplete());

            const Key& tmp_victim_key = victim_dirinfosets_const_iter->first;
            if (extra_perkey_victim_dirinfoset.find(tmp_victim_key) == extra_perkey_victim_dirinfoset.end())
            {
                extra_perkey_victim_dirinfoset.insert(std::pair<Key, DirinfoSet>(tmp_victim_key, victim_dirinfosets_const_iter->second));
            }
        }
        // Insert local beaconed neighbor fetched victims
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator victim_dirinfosets_const_iter = local_beaconed_neighbor_fetched_victim_dirinfosets.begin(); victim_dirinfosets_const_iter != local_beaconed_neighbor_fetched_victim_dirinfosets.end(); victim_dirinfosets_const_iter++)
        {
            // NOTE: extra fetched victim dirinfo sets from local directory table MUST be complete
            assert(victim_dirinfosets_const_iter->second.isComplete());

            const Key& tmp_victim_key = victim_dirinfosets_const_iter->first;
            if (extra_perkey_victim_dirinfoset.find(tmp_victim_key) == extra_perkey_victim_dirinfoset.end())
            {
                extra_perkey_victim_dirinfoset.insert(std::pair<Key, DirinfoSet>(tmp_victim_key, victim_dirinfosets_const_iter->second));
            }
        }

        return;
    }
}
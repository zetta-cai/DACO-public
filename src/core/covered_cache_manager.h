/*
 * CoveredCacheManager: COVERED's manager tracks metadata to integrate local edge cache and cooperation wrapper for cooperative-caching-aware cache management (thread safe).
 *
 * NOTE: (1) in baselines with independent cache management, local edge cache (KV data and local cache metadata) and cooperation wrapper (content discovery and request redirection) are orthogonal/independent; (2) however, in COVERED with cooperative-caching-aware cache management, local edge cache needs to provide/update metadata (e.g., local uncached popularity and current-edge-node victims), while cooperation wrapper needs to sync metadata (e.g., popularity aggregation and victim synchronization) to gain a global view of all involved edge nodes for cache management with cooperation awareness -> so we encapsulate CoveredCacheManger to integrate local edge cache and cooperation wrapper for COVERED's core design.
 * 
 * NOTE: DirectoryCacher is used by sender edge node to cache remote valid directory information, while CooperationWrapper is used by beacon edge node to maintain content diretory information and MSI metadata, so we maintain DirectoryCacher in CoveredCacheMananger instead of CooperationWrapper.
 * 
 * NOTE: we focus on metadata scalability on # of objects instead of # of edge nodes, as long as we can guarantee limited metadata overhead per edge node
 * (1) For local uncached metadata, ONLY track the uncached objects with top approximate admission benefits
 * (2) For victim synchronization, track per-edge victims, yet ONLY peredge_victimcnt cached objects with least local rewards in each edge node
 * (2-1) For prev victim syncset (reduce bandwidth overhead of victim synchronization), track per-edge previously-issued victim syncset, yet local synced/beaconed victims in a single victim syncset for each edge node is limited due to peredge_victimcnt limitation
 * (2-2) TODO: For victim metadata cache (reduce overhead of lazy victim fetching), ONLY track the extra fetched victims with top fetching frequencies
 * (3) For popularity aggregation, ONLY track the uncached objects with top max admission benefits
 * (3-1) NOTE: it is okay to maintain per-edge local uncached popularity in aggregated uncached popularity for each object, as metadata overhead of a single local uncached popularity is limited for each edge node -> we ONLY track top-k and sum local uncached popularity just beacuse we ONLY need the partial information for greedy placement calculation
 * (4) For directory metadata cache, track valid remote directory information ONLY for the locally-uncached objects with top approximate admission benefits
 * 
 * By Siyuan Sheng (2023.08.26).
 */

#ifndef COVERED_CACHE_MANAGER_H
#define COVERED_CACHE_MANAGER_H

//#define DEBUG_COVERED_CACHE_MANAGER

#include <list>
#include <string>
#include <unordered_map>

namespace covered
{
    // Forward declaration
    class CoveredCacheManager;
}

#include "common/key.h"
#include "cooperation/directory/directory_info.h"
#include "core/directory_cacher.h"
#include "core/popularity/edgeset.h"
#include "core/popularity_aggregator.h"
#include "core/victim/victim_cacheinfo.h"
#include "core/victim/victim_syncset.h"
#include "core/victim_tracker.h"
#include "edge/edge_wrapper.h"
#include "network/network_addr.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class CoveredCacheManager
    {
    public:
        CoveredCacheManager(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& topk_edgecnt);
        ~CoveredCacheManager();

        // For selective popularity aggregation (may trigger trade-off-aware placement calculation)

        // NOTE: need_placement_calculation works only when key is tracked by local uncached metadata of sender edge node
        // NOTE: sender_is_beacon indicates whether sender is cache server worker in beacon edge node to trigger local placement calculation, or sender is beacon server in beacon edge node to trigger placement calculation for remote requests; best_placement_edgeset is used for non-blocking placement notification if need hybrid data fetching; need_hybrid_fetching MUST be true under sender_is_beacon = true if with best placement yet local edge cache misses
        // NOTE: (for lazy victim fetching) edge_wrapper_ptr is used for issuing victim fetch requests; recvrsp_source_addr and recvrsp_socket_server_ptr are used for receiving victim fetch responses; bandwidth usage, event list, and skip propagation latency is used for victim fetch messages
        // NOTE: (for non-blocking placement deployment) edge_wrapper_ptr is used for issuing non-blocking data fetching and placement notification requests; recvrsp_source_addr and recvrsp_socket_server_ptr are NOT used due to background processing of local placement notification in current beacon edge node; skip propagation latency is used for all messages during non-blocking placement deployment (bandwidth usage and event list are ONLY used by foreground victim fetching instead of background non-blocking placement deployment)
        bool updatePopularityAggregatorForAggregatedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, const bool& need_placement_calculation, const bool& sender_is_beacon, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency); // Return if edge node is finished
        void clearPopularityAggregatorForPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx);

        void assertNoLocalUncachedPopularity(const Key& key, const uint32_t& source_edge_idx) const;

        // For victim synchronization

        void updateVictimTrackerForLocalSyncedVictims(const uint64_t& local_cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victim_cacheinfos, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_local_synced_victim_dirinfosets, const CooperationWrapperBase* cooperation_wrapper_ptr);
        void updateVictimTrackerForLocalBeaconedVictimDirinfo(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);

        VictimSyncset accessVictimTrackerForLocalVictimSyncset(const uint32_t& dst_edge_idx_for_compression, const uint64_t& latest_local_cache_margin_bytes) const; // Get complete/delta victim syncset from victim tracker **for piggybacking-based victim synchronization** (dst_edge_idx_for_compression is used to update prev victim syncset towards dst edge idx; latest_local_cache_margin_bytes is used to replace not-latest cache margin bytes in edge-level victim metadata of the current edge node)
        void updateVictimTrackerForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_neighbor_synced_victim_dirinfosets, const CooperationWrapperBase* cooperation_wrapper_ptr); // Update victim tracker in the current edge node for the received victim syncset from neighbor edge node (neighbor_victim_syncset could be either complete or compressed)

        // For directory metadata cache

        bool accessDirectoryCacherForCachedDirectory(const Key& key, CachedDirectory& cached_directory) const; // Return if key is tracked by directory_cacher_
        bool accessDirectoryCacherToCheckPopularityChange(const Key& key, const Popularity& local_uncached_popularity, CachedDirectory& cached_directory, bool& is_large_popularity_change) const; // Return if key is tracked by directory_cacher_
        void updateDirectoryCacherToRemoveCachedDirectory(const Key& key);
        void updateDirectoryCacherForNewCachedDirectory(const Key& key, const CachedDirectory& cached_directory);

        uint64_t getSizeForCapacity() const;
    private:
        typedef DeltaReward PlacementGain; // Admission benefit - eviction cost for trade-off-aware cache placement and eviction

        static const std::string kClassName;

        // Perform placement calculation only if key belongs to a global popular uncached object (i.e., with large enough max admission benefit)
        // NOTE: best_placement_edgeset is used for perserved edgeset and placement notifications; best_placement_peredge_synced_victimset is used for synced victim removal from victim tracker; best_placement_peredge_fetched_victimset is used for fetched victim removal from victim cache (all for non-blocking placement deployment)
        bool placementCalculation_(const Key& key, const bool& is_global_cached, bool& has_best_placement, Edgeset& best_placement_edgeset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_synced_victimset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& best_placement_peredge_fetched_victimset, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // has_best_placement indicates if the best placement exists (i.e., with positive placement gain) (return if edge node is finished)

        // For lazy victim fetching
        bool parallelFetchVictims_(const ObjectSize& object_size, const Edgeset& best_placement_victim_fetch_edgeset, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos, std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset) const; // For each edge node index in victim fetch edgeset (return if edge node is finished)
        void sendVictimFetchRequest_(const uint32_t& dst_edge_idx_for_compression, const ObjectSize& object_size, const EdgeWrapper* edge_wrapper_ptr, const NetworkAddr& recvrsp_source_addr, const NetworkAddr& edge_cache_server_recvreq_dst_addr, const bool& skip_propagation_latency) const;
        void processVictimFetchResponse_(const MessageBase* control_respnose_ptr, const EdgeWrapper* edge_wrapper_ptr, std::unordered_map<uint32_t, std::list<VictimCacheinfo>>& extra_peredge_victim_cacheinfos, std::unordered_map<Key, DirinfoSet, KeyHasher>& extra_perkey_victim_dirinfoset) const;

        // Const shared variables
        std::string instance_name_;
        const uint32_t topk_edgecnt_; // Come from CLI

        // Non-const shared variables (each should be thread safe)
        // NOTE: we do NOT use per-key fine-grained locking here, as CoveredCacheManager does NOT need strong serializability/atomicity for its componenets, and some components (PopularityAggregator and VictimTracker) CANNOT use per-key fine-grained locking due to accessing multiple keys in one function (e.g., discardGlobalLessPopularObjects_() and updateLocalSyncedVictims())

        // Track aggregated uncached popularity for global admission (thread safe; used by beacon edge node)
        PopularityAggregator popularity_aggregator_;

        // Track per-edge-node least popular victims for placement and eviction (thread safe; used by sender/beacon edge node)
        mutable VictimTracker victim_tracker_;

        // Track cached directory of popular local uncached objects to reduce message overhead (thread safe; used by sender edge node)
        DirectoryCacher directory_cacher_;
    };
}

#endif
/*
 * Use an array of cache structures to simulate multiple cache nodes, use an array of workload generators to simulate multiple clients, and process requests received by different cache nodes in a round-robin manner by one thread in the current physical machine as a single-node simulator for large-scale performance analysis.
 * 
 * NOTE: single node prototype still runs different components in parallel by multiple threads for both hit ratios and absolute performance, while single node simulator only focuses on hit ratios under large scales without absolute performance.
 * 
 * NOTE: we assume that single-node simulator has a global view of all cache nodes to directly calculate local and remote hit ratios -> NO need explicit message transmission for content discovery, request redirection, and COVERED's popularity collection and victim synchronization (but still need local cache access to update local cache statistics, content discovery to select target edge node, request redirection to update remote cache statistics, COVERED's popularity collection and victim synchronization for fast caceh placement, and baseline cache admission/eviction policies for cache management).
 * 
 * NOTE: as the simulator does not consider the absolute performance, NO need message transmissions and also NO need the WAN propagation latency injection (but still measure dynamically changed latencies for COVERED and involved baselines such as LA-Cache -> equal to NO other latencies such as processing latencies in cache nodes, which is acceptable as the propagation latencies are the bottleneck in WAN distributed caching, and single node simulator ONLY focuses on hit ratios instead of absolute performance).
 * 
 * (FUTURE) TODO: NOT simulate directory cache here, as we focus on hit ratios instead of absolute performance in single-node simulator.
 * 
 * By Siyuan Sheng (2024.08.12).
 */

#include <map> // std::multimap
#include <random> // std::mt19937_64, std::uniform_int_distribution
#include <sstream> // std::ostringstream
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

#include "benchmark/evaluator_wrapper.h"
#include "cache/basic_cache_custom_func_param.h"
#include "cache/covered_cache_custom_func_param.h"
#include "cli/single_node_cli.h"
#include "common/bandwidth_usage.h"
#include "common/config.h"
#include "common/key.h"
#include "common/util.h"
#include "common/value.h"
#include "edge/basic_edge_custom_func_param.h"
#include "edge/covered_edge_custom_func_param.h"
#include "edge/basic_edge_wrapper.h"
#include "edge/covered_edge_wrapper.h"
#include "message/message_base.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    // (1) Global structs and classes

    // Object-level information of a global cached object for single-node simulation
    struct GlobalCachedObjinfo
    {
        std::vector<uint32_t> edge_node_idxes; // Which edge nodes caching the object
    };

    // GlobalCachedObjinfo of global cached objects (for all methods)
    struct PerkeyGlobalCachedObjinfo
    {
        bool isGlobalCached(const Key& key) const;
        bool isNeighborCached(const Key& key, const uint32_t& given_edgeidx) const;
        bool getEdgeNodeIdxes(const Key& key, std::vector<uint32_t>& edge_node_idxes) const;
        void addEdgeNode(const Key& key, const uint32_t& given_edgeidx);
        void removeEdgeNode(const Key& key, const uint32_t& given_edgeidx);

        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher> key_global_cached_objinfo_map; // For global cached objects
    };

    // Object-level information of a local uncached object for single-node simulation (ONLY for COVERED)
    struct LocalUncachedObjinfo
    {
        typedef std::multimap<float, uint32_t, std::greater<float>> sorted_popularity_map_t;
        typedef std::unordered_map<uint32_t, sorted_popularity_map_t::iterator> lookup_map_t;

        LocalUncachedObjinfo();
        LocalUncachedObjinfo(const LocalUncachedObjinfo& others); // NOTE: we need to redefine copy constructor due to the same reason as follows (see comment of copy assignment operator)

        void update(const uint32_t& given_edgeidx, const CollectedPopularity& collected_popularity);
        bool remove(const uint32_t& given_edgeidx);
        void getTopkLocalUncachedPopularities(const uint32_t& covered_topk_edgecnt, std::vector<std::pair<uint32_t, float>>& topk_local_uncached_popularities) const;

        // NOTE: default copy assignment operator will copy others.sorted_popularity_map_t::iterator, which could point to sorted_popularity_map_t of temporary variable in the stack and incur segmentation fault -> we must rebuild lookup_map_t based on sorted_popularity_map_t of the current variable
        const struct LocalUncachedObjinfo& operator=(const struct LocalUncachedObjinfo& others);

        uint32_t objsize;
        sorted_popularity_map_t sorted_local_uncached_popularity_edgeidx_map; // Local uncached popularity for each edge node not caching yet tracking the object (descending order of local uncached popularity)
        lookup_map_t edgeidx_multimapiter_lookup_map; // Lookup map for per-edge local uncached popularity
    };

    // LocalUncachedObjinfo of local uncached objects (ONLY for COVERED)
    struct PerkeyLocalUncachedObjinfo
    {
        bool getLocalUncachedObjinfo(const Key& key, LocalUncachedObjinfo& local_uncached_objinfo) const;
        void tryToRemoveLocalUncachedPopularityForEdge(const Key& key, const uint32_t given_edgeidx);
        void addLocalUncachedPopularityForEdge(const Key& key, const uint32_t given_edgeidx, const CollectedPopularity& collected_popularity);

        std::unordered_map<Key, LocalUncachedObjinfo, KeyHasher> key_local_uncached_objinfo_map; // For local uncached objects (ONLY for COVERED)
    };

    // Edge-node-level eviction information for single-node simulation (ONLY for COVERED)
    struct EvictNodeinfo
    {
        uint64_t cache_margin_bytes;
        std::list<VictimCacheinfo> victim_cacheinfos;
    };

    // EvictNodeinfo of all edge nodes (ONLY for COVERED)
    struct PeredgeEvictNodeinfo
    {
        bool getEvictinfo(const uint32_t& given_edgeidx, EvictNodeinfo& evictinfo) const;
        void updateCacheMarginBytesOnly(const uint32_t& given_edgeidx, const uint64_t cache_margin_bytes);
        void updateEvictinfo(const uint32_t& given_edgeidx, const uint64_t cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos);

        std::unordered_map<uint32_t, EvictNodeinfo> edgeidx_evictinfo_map; // For all edge nodes (ONLY for COVERED)
    };

    // (2) Global variables

    // Global information for single-node simulation
    struct PerkeyGlobalCachedObjinfo perkey_global_cached_objinfo; // For global cached objects
    struct PerkeyLocalUncachedObjinfo perkey_local_uncached_objinfo; // For local uncached objects (ONLY for COVERED)
    struct PeredgeEvictNodeinfo peredge_evictinfo; // For all edge nodes (ONLY for COVERED)
    std::unordered_map<uint32_t, uint64_t> peredge_victim_vtime; // For all edge nodes (ONLY for BestGuess)

    // An array of edge wrappers (use the internal cache structures, e.g., local caches in cache wrapper, dht wrapper in cooperative wrapper, and COVERED's weight tuner) to simulate multiple edge nodes
    std::vector<EdgeWrapperBase*> edge_wrapper_ptrs;

    // An array of workload wrappers to simulate multiple client nodes (NO need metadata, e.g., is_warmup and closest edge index, in client (worker) wrapper, which will be maintained by single-node simulator itself)
    std::vector<WorkloadWrapperBase*> workload_wrapper_ptrs; // NOTE: determined by workloadcnt instead of clientcnt to fix memory issue of large-scale simulation (yet each client worker still has an individual workload worker), yet NOT affect simulation results!
    std::vector<uint32_t> closest_edge_idxes; // Closest edge node index for each client node

    // Evaluation variables
    std::mt19937_64 content_discovery_randgen(0); // Used for content discovery simulation

    // Unused variables (NOT really used for caching; just for simulation)
    NetworkAddr network_addr_for_debug("127.0.0.1", Util::UDP_MIN_PORT + 1);
}

namespace covered
{
    // (1) Utility functions

    WorkloadItem genWorkloadItemForClientWorker(const uint32_t& client_workeridx, const uint32_t& clientidx, const uint32_t& perclient_workercnt, const uint32_t& client_workercnt, const uint32_t& perworkload_workercnt, const uint32_t& simulator_workloadcnt);
    uint32_t getClosestEdgeidx(const uint32_t& clientidx);
    EdgeWrapperBase* getEdgeWrapperPtr(const uint32_t& given_edgeidx);
    EdgeWrapperBase* getClosestEdgeWrapperPtr(const uint32_t& clientidx);
    CacheWrapper* getClosestEdgeCacheWrapperPtr(const uint32_t& clientidx);
    uint32_t getBeaconEdgeidx(const Key& cur_key);
    EdgeWrapperBase* getBeaconEdgeWrapperPtr(const Key& cur_key);
    bool isLocalBeacon(const Key& cur_key, const uint32_t& clientidx);

    std::string getStatisticsString(const uint32_t& reqcnt, const uint32_t& local_hitcnt, const uint32_t& remote_hitcnt, const uint64_t& latency_sum, const BandwidthUsage& bandwidth_usage);

    // (2) Common helper functions

    void processRequest(const WorkloadItem& cur_workload_item, const uint32_t& clientidx, const std::string& cache_name, const uint32_t& edgecnt, const uint32_t& covered_topk_edgecnt, uint32_t& reqcnt, uint32_t& local_hitcnt, uint32_t& remote_hitcnt, uint64_t& latency_sum_us, BandwidthUsage& bandwidth_usage);

    bool accessClosestCache(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, Value& fetched_value); // Return is_local_cached_and_valid
    void writeClosestCache(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const std::string& cache_name, const WorkloadItemType& cur_workload_item_type, bool& is_evict, uint32_t& victim_cnt);
    void contentDiscovery(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, const uint32_t& edgecnt, bool& is_remote_hit, uint32_t& target_edge_idx, BandwidthUsage& curpkt_bandwidth_usage);
    void requestRedirection(const Key& cur_key, const uint32_t& clientidx, const uint32_t& target_edge_idx, const std::string& cache_name, Value& fetched_value, BandwidthUsage& curpkt_bandwidth_usage);
    void validateClosestEdgeForFetchedValue(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const std::string& cache_name, const Hitflag& curobj_hitflag, bool& is_evict, uint32_t& victim_cnt);
    uint64_t calcReadLatencyBeforeCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& local_access_latency_us, const uint32_t& content_discovery_latency_us, const uint32_t& request_redirection_latency_us, const uint32_t& cloud_access_latency_us, const Hitflag& hitflag, const std::string& cache_name);
    uint64_t calcWriteLatencyBeforeCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& local_access_latency_us, const uint32_t& acquire_writelock_latency_us, const uint32_t& release_writelock_latency_us, const uint32_t& cloud_access_latency_us, const std::string& cache_name);
    void triggerCacheManagement(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const Hitflag& curobj_hitflag, const std::string& cache_name, const uint64_t& total_latency_us, const uint32_t& covered_topk_edgecnt, bool& is_admit, bool& is_evict, uint32_t& victim_cnt);
    void updateTotalLatencyAfterCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& directory_admit_latency_us, const uint32_t& directory_evict_latency_us, const std::string& cache_name, uint64_t& total_latency_us);
    void admitIntoEdge(const Key& cur_key, const Value& fetched_value, const uint32_t& placement_edgeidx, const Hitflag& hitflag, const std::string& cache_name, const uint64_t& miss_latency_us, bool& is_evict, uint32_t& victim_cnt);
    void evictForCapacity(const uint32_t& placement_edgeidx, const std::string& cache_name, bool& is_evict, uint32_t& victim_cnt);
    uint32_t getDirectoryUpdateLatency(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name);

    // (3) COVERED's helper functions

    bool coveredTryPopularityAggregationForClosestEdge(const Key& cur_key, const uint32_t& clientidx); // Return is_tracked_after_fetch_value
    void coveredVictimSynchronizationForEdge(const uint32_t& given_edgeidx, const bool& only_update_cache_margin_bytes);
    float coveredCalcEvictionCost(const Key& cur_key, const uint32_t& object_size, const std::unordered_set<uint32_t>& placement_edgeset);
    void coveredPlacementCalculation(const Key& cur_key, const uint32_t& covered_topk_edgecnt, bool& has_best_placement, std::unordered_set<uint32_t>& best_placement_edgeset);
    void coveredPlacementDeployment(const Key& key, const Value& fetched_value, const std::unordered_set<uint32_t>& best_placement_edgeset, const Hitflag& hitflag, const std::string& cache_name, const uint64_t miss_latency_us, bool& is_evict, uint32_t& victim_cnt);
    void coveredMetadataUpdate(const Key& cur_key, const uint32_t& notify_edgeidx, const bool& is_neighbor_cached);

    // (4) BestGuess's helper functions

    void bestguessVtimeSynchronizationForEdge(const uint32_t& given_edgeidx);
    uint32_t bestguessGetEdgeidxWithSmallestVtime();

    // (5) Helper functions for bandwidth calculation
    void updateBandwidthUsageForClientRequest(const WorkloadItem& cur_workload_item, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForClientGetResponse(const Key& cur_key, const uint32_t& clientidx, const Value& value, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForClientPutResponse(const Key& cur_key, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForContentDiscovery(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForRequestRedirection(const Key& cur_key, const Value& value, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForGlobalGet(const Key& cur_key, const Value& value, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForGlobalPut(const Key& cur_key, const Value& value, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForDirectoryUpdate(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, const bool& is_admit, const uint32_t& victim_cnt, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForAcquireWritelock(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage);
    void updateBandwidthUsageForReleaseWritelock(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage);
}

int main(int argc, char **argv) {
    // (1) Parse and process different CLI parameters for client/edge/cloud/evaluator

    covered::SingleNodeCLI single_node_cli(argc, argv);
    const std::string main_class_name = covered::Config::getMainClassName();

    // Common configurations
    const uint64_t capacity_bytes = single_node_cli.getCapacityBytes();
    const uint32_t edgecnt = single_node_cli.getEdgecnt(); // Used by each client to calculate closest edge node

    // Edge configurations
    const std::string cache_name = single_node_cli.getCacheName();
    const std::string hash_name = single_node_cli.getHashName();
    const uint32_t keycnt = single_node_cli.getKeycnt();
    const uint32_t percacheserver_workercnt = single_node_cli.getPercacheserverWorkercnt(); // NOT affect single-node simulation, as multiple edge cache server workers still share the same local cache structure
    const covered::CLILatencyInfo cli_latency_info = single_node_cli.getCLILatencyInfo();
    const std::string realnet_option = single_node_cli.getRealnetOption();
    const std::string realnet_expname = single_node_cli.getRealnetExpname();
    // Specific for COVERED (some are obsolete)
    const uint64_t covered_local_uncached_capacity_bytes = single_node_cli.getCoveredLocalUncachedMaxMemUsageBytes(); // Used for local uncached metadata in COVERED
    const uint64_t covered_local_uncached_lru_bytes = single_node_cli.getCoveredLocalUncachedLruMaxBytes(); // Used for local uncached LRU in COVERED
    const uint32_t covered_peredge_synced_victimcnt = single_node_cli.getCoveredPeredgeSyncedVictimcnt();
    const uint32_t covered_peredge_monitored_victimsetcnt = single_node_cli.getCoveredPeredgeMonitoredVictimsetcnt();
    const uint64_t covered_popularity_aggregation_capacity_bytes = single_node_cli.getCoveredPopularityAggregationMaxMemUsageBytes();
    const double covered_popularity_collection_change_ratio = single_node_cli.getCoveredPopularityCollectionChangeRatio();
    const uint32_t covered_topk_edgecnt = single_node_cli.getCoveredTopkEdgecnt();

    // Client configurations
    const uint32_t clientcnt = single_node_cli.getClientcnt();
    const uint32_t perclient_opcnt = single_node_cli.getPerclientOpcnt();
    const uint32_t perclient_workercnt = single_node_cli.getPerclientWorkercnt(); // Affect single-node simulation, as each client worker has its own index information in the workload generator
    const uint32_t client_workercnt = clientcnt * perclient_workercnt;
    const std::string workload_name = single_node_cli.getWorkloadName();
    const float zipf_alpha = single_node_cli.getZipfAlpha();
    const std::string workload_pattern_name = single_node_cli.getWorkloadPatternName();
    const uint32_t dynamic_change_period = single_node_cli.getDynamicChangePeriod();
    const uint32_t dynamic_change_keycnt = single_node_cli.getDynamicChangeKeycnt();

    // Workload configurations
    uint32_t simulator_workloadcnt = single_node_cli.getSimulatorWorkloadcnt(); // physical workloadidx is in [0, simulator_workloadcnt - 1] (used to locate workload generator)
    if (simulator_workloadcnt > clientcnt) // Unnecessary to maintain more than clientcnt workload generators
    {
        simulator_workloadcnt = clientcnt;
    }
    const uint32_t simulator_randomness = single_node_cli.getSimulatorRandomness();
    // NOTE: generate sufficient workload workers, each corresponding to an edge node
    const uint32_t perworkload_workercnt = (client_workercnt - 1) / simulator_workloadcnt + 1; // global_workload_workeridx = local_workload_workeridx + perworkload_workercnt * workloadidx (used to generate request with the specific randgen; each client worker MUST correspond to a specific workload worker)
    assert(perworkload_workercnt * simulator_workloadcnt >= client_workercnt);
    // NOTE: ONLY used for random seeds in workload generators
    const uint32_t workload_random_factor = 1000;
    const uint32_t workload_randombase = simulator_randomness * workload_random_factor;

    // NOTE: NO need to simulate cloud, as we only focus on hit ratios and can assume that we have global information of the dataset -> just get objects based on workload generator

    // Warmup configurations
    const uint32_t warmup_reqcnt_scale = single_node_cli.getWarmupReqcntScale();

    // Stresstest configurations
    const uint32_t stresstest_duration_sec = single_node_cli.getStresstestDurationSec();

    // (2) Initialize global variables for single-node simulation

    // Initialize for edge nodes (refer to src/edge/edge_wrapper_base.c::launchEdge())
    covered::edge_wrapper_ptrs.resize(edgecnt, NULL);
    for (uint32_t edgeidx = 0; edgeidx < edgecnt; edgeidx++)
    {
        // NOTE: NOT use EdgeWrapperBase::launchEdge, which will invoke NodeWrapperBase::start() to launch multiple threads for absolute performance!
        if (cache_name == covered::Util::COVERED_CACHE_NAME)
        {
            covered::edge_wrapper_ptrs[edgeidx] = new covered::CoveredEdgeWrapper(cache_name, capacity_bytes, edgeidx, edgecnt, hash_name, keycnt, covered_local_uncached_capacity_bytes, covered_local_uncached_lru_bytes, percacheserver_workercnt, covered_peredge_synced_victimcnt, covered_peredge_monitored_victimsetcnt, covered_popularity_aggregation_capacity_bytes, covered_popularity_collection_change_ratio, cli_latency_info, covered_topk_edgecnt, realnet_option, realnet_expname);
        }
        else
        {
            covered::edge_wrapper_ptrs[edgeidx] = new covered::BasicEdgeWrapper(cache_name, capacity_bytes, edgeidx, edgecnt, hash_name, keycnt, covered_local_uncached_capacity_bytes, covered_local_uncached_lru_bytes, percacheserver_workercnt, covered_peredge_synced_victimcnt, covered_peredge_monitored_victimsetcnt, covered_popularity_aggregation_capacity_bytes, covered_popularity_collection_change_ratio, cli_latency_info, covered_topk_edgecnt, realnet_option, realnet_expname);
        }
        
        // NOTE: NOT invoke NodeWrapperBase::start() to launch multiple threads for absolute performance!
        assert(covered::edge_wrapper_ptrs[edgeidx] != NULL);
    }

    // Initialize for workload generators (refer to src/benchmark/client_wrapper.c::launchClient())
    // NOTE: if workloadcnt = clientcnt (i.e., perworkload_workercnt = perclient_workercnt), each client has an individual workload generator corresponding to the closest edge node -> here we just use a reasonable workloadcnt to fix the memory issue of large-scale exps (but each client worker ALWAYS has an individual workload worker corresponding to the closest edge node)
    // NOTE: this is acceptable as we only focus on hit ratios and calculated performance instead of absolute performance in single-node simulator, while we have verified that workloadcnt does NOT affect the simulator results (affect absolute performance yet NOT concerned by single-node simulator)
    covered::workload_wrapper_ptrs.resize(simulator_workloadcnt, NULL);
    for (uint32_t workloadidx = 0; workloadidx < simulator_workloadcnt; workloadidx++)
    {
        // NOTE: NOT use ClientWrapper::launchEdge, which will invoke NodeWrapperBase::start() to launch multiple threads for absolute performance!
        // NOTE: use covered::Util::DATASET_KVPAIR_GENERATION_SEED to generate dataset, use workloadidx to pre-generate workload items to approximate workload distribution (if any), and use global_workload_workeridx + randombase to generate requests for the given workload worker
        covered::workload_wrapper_ptrs[workloadidx] = covered::WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(simulator_workloadcnt, workloadidx, keycnt, perclient_opcnt, perworkload_workercnt, workload_name, covered::WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLIENT, zipf_alpha, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, simulator_randomness);
        assert(covered::workload_wrapper_ptrs[workloadidx] != NULL);
    }

    // Initialize for client nodes (refer to src/benchmark/client_wrapper.c::launchClient())
    covered::closest_edge_idxes.resize(clientcnt, 0);
    for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
    {
        // Calculate closest edge node index for each client node (refer to src/benchmark/client_worker_wrapper.c::ClientWorkerWrapper())
        covered::closest_edge_idxes[clientidx] = covered::Util::getClosestEdgeIdx(clientidx, clientcnt, edgecnt);
        assert(covered::closest_edge_idxes[clientidx] >= 0);
        assert(covered::closest_edge_idxes[clientidx] < edgecnt);
    }

    // (3) Warmup phase

    // Calculate per-client-worker warmup reqcnt (refer to src/benchmark/evaluator_wrapper.c::checkWarmupStatus_() and src/benchmark/client_worker_wrapper.c::ClientWorkerWrapper())
    const uint32_t total_warmup_reqcnt = warmup_reqcnt_scale * keycnt;
    const uint32_t total_client_workercnt = clientcnt * perclient_workercnt;
    const uint32_t warmup_reqcnt_limit = (total_warmup_reqcnt - 1) / total_client_workercnt + 1; // Get per-client-worker warmup reqcnt limitation
    assert(warmup_reqcnt_limit > 0);
    assert(warmup_reqcnt_limit * total_client_workercnt >= total_warmup_reqcnt); // Total # of issued warmup reqs MUST >= total # of required warmup reqs

    // Used to dump warmup statistics per interval
    uint32_t warmup_interval_idx = 0;
    const uint32_t warmup_interval_us = SEC2US(1);
    struct timespec warmup_cur_timestamp = covered::Util::getCurrentTimespec();
    struct timespec warmup_prev_timestamp = warmup_cur_timestamp;
    uint32_t warmup_interval_reqcnt = 0;
    uint32_t warmup_interval_local_hitcnt = 0;
    uint32_t warmup_interval_remote_hitcnt = 0;
    uint64_t warmup_interval_latency_sum = 0; // Calculated latency (calculated performance, yet NOT absolute performance)
    covered::BandwidthUsage warmup_interval_bandwidth_usage;
    uint32_t warmup_total_reqcnt = 0;

    // Generate requests via workload generators one by one to simulate cache access
    for (uint32_t warmup_reqidx = 0; warmup_reqidx < warmup_reqcnt_limit; warmup_reqidx++)
    {
        // Each client worker needs to generate warmup_reqcnt_limit requests for warmup
        for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
        {
            for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt; local_client_worker_idx++)
            {
                // Generate workload item by the workload worker to simulate the client worker
                covered::WorkloadItem cur_workload_item = covered::genWorkloadItemForClientWorker(local_client_worker_idx, clientidx, perclient_workercnt, client_workercnt, perworkload_workercnt, simulator_workloadcnt);
                
                covered::processRequest(cur_workload_item, clientidx, cache_name, edgecnt, covered_topk_edgecnt, warmup_interval_reqcnt, warmup_interval_local_hitcnt, warmup_interval_remote_hitcnt, warmup_interval_latency_sum, warmup_interval_bandwidth_usage);
            }
        }

        // Calculate delta time to determine whether to dump warmup statistics during current interval
        warmup_cur_timestamp = covered::Util::getCurrentTimespec();
        double warmup_delta_us = covered::Util::getDeltaTimeUs(warmup_cur_timestamp, warmup_prev_timestamp);
        if (warmup_delta_us >= static_cast<double>(warmup_interval_us))
        {
            // Update warmup total statistics
            warmup_total_reqcnt += warmup_interval_reqcnt;

            // Get warmup interval statistics string
            std::string warmup_interval_statistics_string = covered::getStatisticsString(warmup_interval_reqcnt, warmup_interval_local_hitcnt, warmup_interval_remote_hitcnt, warmup_interval_latency_sum, warmup_interval_bandwidth_usage);

            // Dump warmup statistics per interval
            std::ostringstream oss;
            oss << "[Warmup Statistics at Interval " << warmup_interval_idx << "]" << std::endl;
            oss << "Total reqcnt: " << warmup_total_reqcnt << std::endl;
            oss << warmup_interval_statistics_string << std::endl << std::endl;
            std::cout << oss.str() << std::flush;

            // Reset warmup interval statistics
            warmup_prev_timestamp = warmup_cur_timestamp;
            warmup_interval_reqcnt = 0;
            warmup_interval_local_hitcnt = 0;
            warmup_interval_remote_hitcnt = 0;
            warmup_interval_latency_sum = 0;
            warmup_interval_bandwidth_usage = covered::BandwidthUsage();

            warmup_interval_idx += 1;
        }
    }

    // (4) Stresstest phase

    // Used to dump stresstest statistics during the whole stresstest phase
    uint32_t stresstest_interval_idx = 0;
    const uint32_t stresstest_interval_us = SEC2US(1);
    struct timespec stresstest_start_timestamp = covered::Util::getCurrentTimespec();
    struct timespec stresstest_cur_timestamp = stresstest_start_timestamp;
    struct timespec stresstest_prev_timestamp = stresstest_start_timestamp;
    uint32_t stresstest_total_reqcnt = 0;
    uint32_t stresstest_total_local_hitcnt = 0;
    uint32_t stresstest_total_remote_hitcnt = 0;
    uint64_t stresstest_total_latency_sum = 0; // Calculated latency (calculated performance, yet NOT absolute performance)
    covered::BandwidthUsage stresstest_total_bandwidth_usage;

    // Generate requests via workload generators one by one to simulate cache access until stresstest duration finishes
    while (true)
    {
        // Each client worker continues to generate requests for stresstest
        for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
        {
            for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt; local_client_worker_idx++)
            {
                // Generate workload item by the workload worker to simulate the client worker
                covered::WorkloadItem cur_workload_item = covered::genWorkloadItemForClientWorker(local_client_worker_idx, clientidx, perclient_workercnt, client_workercnt, perworkload_workercnt, simulator_workloadcnt);
                
                covered::processRequest(cur_workload_item, clientidx, cache_name, edgecnt, covered_topk_edgecnt, stresstest_total_reqcnt, stresstest_total_local_hitcnt, stresstest_total_remote_hitcnt, stresstest_total_latency_sum, stresstest_total_bandwidth_usage);
            }
        }

        // Calculate delta time to determine whether to dump stresstest statistics until the current interval
        stresstest_cur_timestamp = covered::Util::getCurrentTimespec();
        double stresstest_delta_us = covered::Util::getDeltaTimeUs(stresstest_cur_timestamp, stresstest_prev_timestamp);
        double stresstest_whole_us = covered::Util::getDeltaTimeUs(stresstest_cur_timestamp, stresstest_start_timestamp);
        if (stresstest_delta_us >= static_cast<double>(stresstest_interval_us) || stresstest_whole_us >= static_cast<double>(SEC2US(stresstest_duration_sec)))
        {
            const bool is_finish = stresstest_whole_us >= static_cast<double>(SEC2US(stresstest_duration_sec));

            // Get stresstest total statistics string
            std::string stresstest_total_statistics_string = covered::getStatisticsString(stresstest_total_reqcnt, stresstest_total_local_hitcnt, stresstest_total_remote_hitcnt, stresstest_total_latency_sum, stresstest_total_bandwidth_usage);

            // Dump stresstest statistics until the current interval
            std::ostringstream oss;
            if (!is_finish)
            {
                oss << "[Stresstest Statistics until Interval " << stresstest_interval_idx << "]" << std::endl;
            }
            else
            {
                oss << "[Final Stresstest Statistics]" << std::endl;
            }
            oss << stresstest_total_statistics_string << std::endl << std::endl;
            std::cout << oss.str() << std::flush;

            // Reset stresstest statistics
            stresstest_prev_timestamp = stresstest_cur_timestamp;

            stresstest_interval_idx += 1;

            if (is_finish)
            {
                break;
            }
        } // End of dump statistics
    } // End of while (true)

    // (5) Free global variables

    for (uint32_t edgeidx = 0; edgeidx < edgecnt; edgeidx++)
    {
        assert(covered::edge_wrapper_ptrs[edgeidx] != NULL);
        delete covered::edge_wrapper_ptrs[edgeidx];
        covered::edge_wrapper_ptrs[edgeidx] = NULL;
    }

    for (uint32_t workloadidx = 0; workloadidx < simulator_workloadcnt; workloadidx++)
    {
        assert(covered::workload_wrapper_ptrs[workloadidx] != NULL);
        delete covered::workload_wrapper_ptrs[workloadidx];
        covered::workload_wrapper_ptrs[workloadidx] = NULL;
    }

    // (6) Dump finish symbol (refer to src/evaluator.c)

    covered::Util::dumpNormalMsg(main_class_name, covered::EvaluatorWrapper::EVALUATOR_FINISH_BENCHMARK_SYMBOL);

    return 0;
}

namespace covered
{
    // (1) Global structs and classes

    // PerkeyGlobalCachedObjinfo
    
    bool PerkeyGlobalCachedObjinfo::isGlobalCached(const Key& key) const
    {
        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher>::const_iterator key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.find(key);
        return (key_global_cached_objinfo_map_iter != key_global_cached_objinfo_map.end());
    }

    bool PerkeyGlobalCachedObjinfo::isNeighborCached(const Key& key, const uint32_t& given_edgeidx) const
    {
        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher>::const_iterator key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.find(key);
        const std::vector<uint32_t>& edge_node_idxes = key_global_cached_objinfo_map_iter->second.edge_node_idxes;
        for (uint32_t i = 0; i < edge_node_idxes.size(); i++)
        {
            if (edge_node_idxes[i] != given_edgeidx)
            {
                return true;
            }
        }

        return false;
    }

    bool PerkeyGlobalCachedObjinfo::getEdgeNodeIdxes(const Key& key, std::vector<uint32_t>& edge_node_idxes) const
    {
        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher>::const_iterator key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.find(key);

        bool is_exist = (key_global_cached_objinfo_map_iter != key_global_cached_objinfo_map.end());
        if (is_exist)
        {
            edge_node_idxes = key_global_cached_objinfo_map_iter->second.edge_node_idxes; // Deep copy
        }

        return is_exist;
    }

    void PerkeyGlobalCachedObjinfo::addEdgeNode(const Key& key, const uint32_t& given_edgeidx)
    {
        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher>::iterator key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.find(key);

        if (key_global_cached_objinfo_map_iter == key_global_cached_objinfo_map.end())
        {
            key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.insert(std::pair<Key, GlobalCachedObjinfo>(key, GlobalCachedObjinfo())).first;
        }
        key_global_cached_objinfo_map_iter->second.edge_node_idxes.push_back(given_edgeidx);

        return;
    }

    void PerkeyGlobalCachedObjinfo::removeEdgeNode(const Key& key, const uint32_t& given_edgeidx)
    {
        std::unordered_map<Key, GlobalCachedObjinfo, KeyHasher>::iterator key_global_cached_objinfo_map_iter = key_global_cached_objinfo_map.find(key);

        if (key_global_cached_objinfo_map_iter != key_global_cached_objinfo_map.end())
        {
            std::vector<uint32_t>& edge_node_idxes_ref = key_global_cached_objinfo_map_iter->second.edge_node_idxes;
            for (uint32_t i = 0; i < edge_node_idxes_ref.size(); i++)
            {
                if (edge_node_idxes_ref[i] == given_edgeidx)
                {
                    edge_node_idxes_ref.erase(edge_node_idxes_ref.begin() + i);
                    break;
                }
            }

            if (edge_node_idxes_ref.size() == 0)
            {
                key_global_cached_objinfo_map.erase(key_global_cached_objinfo_map_iter);
            }
        }

        return;
    }

    // LocalUncachedObjinfo

    LocalUncachedObjinfo::LocalUncachedObjinfo()
    {
        objsize = 0;
        edgeidx_multimapiter_lookup_map.clear();
        sorted_local_uncached_popularity_edgeidx_map.clear();
    }

    LocalUncachedObjinfo::LocalUncachedObjinfo(const LocalUncachedObjinfo& others)
    {
        this->operator=(others);
    }

    void LocalUncachedObjinfo::update(const uint32_t& given_edgeidx, const CollectedPopularity& collected_popularity)
    {
        objsize = collected_popularity.getObjectSize();

        const float tmp_local_uncached_popularity = collected_popularity.getLocalUncachedPopularity();

        lookup_map_t::iterator lookup_map_iter = edgeidx_multimapiter_lookup_map.find(given_edgeidx);
        if (lookup_map_iter == edgeidx_multimapiter_lookup_map.end())
        {
            sorted_popularity_map_t::iterator sorted_popularity_map_iter = sorted_local_uncached_popularity_edgeidx_map.insert(std::pair<float, uint32_t>(tmp_local_uncached_popularity, given_edgeidx)); // Descending order
            lookup_map_iter = edgeidx_multimapiter_lookup_map.insert(std::pair<uint32_t, sorted_popularity_map_t::iterator>(given_edgeidx, sorted_popularity_map_iter)).first;
        }
        else
        {
            sorted_popularity_map_t::iterator sorted_popularity_map_iter = lookup_map_iter->second;
            assert(sorted_popularity_map_iter != sorted_local_uncached_popularity_edgeidx_map.end());
            assert(sorted_popularity_map_iter->second == given_edgeidx);

            // Replace with the latest local uncached popularity
            sorted_local_uncached_popularity_edgeidx_map.erase(sorted_popularity_map_iter);
            sorted_popularity_map_iter = sorted_local_uncached_popularity_edgeidx_map.insert(std::pair<float, uint32_t>(tmp_local_uncached_popularity, given_edgeidx)); // Descending order

            lookup_map_iter->second = sorted_popularity_map_iter;
        }

        return;
    }
    
    bool LocalUncachedObjinfo::remove(const uint32_t& given_edgeidx)
    {
        bool is_empty = false;

        lookup_map_t::iterator lookup_map_iter = edgeidx_multimapiter_lookup_map.find(given_edgeidx);
        if (lookup_map_iter != edgeidx_multimapiter_lookup_map.end())
        {
            sorted_popularity_map_t::iterator sorted_popularity_map_iter = lookup_map_iter->second;
            assert(sorted_popularity_map_iter != sorted_local_uncached_popularity_edgeidx_map.end());
            assert(sorted_popularity_map_iter->second == given_edgeidx);

            sorted_local_uncached_popularity_edgeidx_map.erase(sorted_popularity_map_iter);
            edgeidx_multimapiter_lookup_map.erase(lookup_map_iter);
        }

        if (edgeidx_multimapiter_lookup_map.empty())
        {
            is_empty = true;
        }

        return is_empty;
    }

    void LocalUncachedObjinfo::getTopkLocalUncachedPopularities(const uint32_t& covered_topk_edgecnt, std::vector<std::pair<uint32_t, float>>& topk_local_uncached_popularities) const
    {
        topk_local_uncached_popularities.clear();

        uint32_t cur_paircnt = 0;
        for (sorted_popularity_map_t::const_iterator iter = sorted_local_uncached_popularity_edgeidx_map.begin(); iter != sorted_local_uncached_popularity_edgeidx_map.end(); iter++) // Descending order
        {
            const float tmp_local_uncached_popularity = iter->first;
            const uint32_t tmp_edgeidx = iter->second;

            topk_local_uncached_popularities.push_back(std::pair<uint32_t, float>(tmp_edgeidx, tmp_local_uncached_popularity));

            cur_paircnt += 1;
            if (cur_paircnt >= covered_topk_edgecnt)
            {
                break;
            }
        }

        assert(topk_local_uncached_popularities.size() <= covered_topk_edgecnt);
        return;
    }

    const struct LocalUncachedObjinfo& LocalUncachedObjinfo::operator=(const struct LocalUncachedObjinfo& others)
    {
        if (this != &others)
        {
            objsize = others.objsize;

            edgeidx_multimapiter_lookup_map.clear();
            sorted_local_uncached_popularity_edgeidx_map.clear();

            for (sorted_popularity_map_t::const_iterator iter = others.sorted_local_uncached_popularity_edgeidx_map.begin(); iter != others.sorted_local_uncached_popularity_edgeidx_map.end(); iter++)
            {
                const uint32_t tmp_edgeidx = iter->second;
                const float tmp_local_uncached_popularity = iter->first;

                // NOTE: default copy assignment operator will copy others.sorted_popularity_map_t::iterator, which could point to sorted_popularity_map_t of temporary variable in the stack and incur segmentation fault -> we must rebuild lookup_map_t based on sorted_popularity_map_t of the current variable
                sorted_popularity_map_t::iterator sorted_popularity_map_iter = sorted_local_uncached_popularity_edgeidx_map.insert(std::pair<float, uint32_t>(tmp_local_uncached_popularity, tmp_edgeidx));
                edgeidx_multimapiter_lookup_map.insert(std::pair<uint32_t, sorted_popularity_map_t::iterator>(tmp_edgeidx, sorted_popularity_map_iter));
            }
        }

        return *this;
    }

    // PerkeyLocalUncachedObjinfo

    bool PerkeyLocalUncachedObjinfo::getLocalUncachedObjinfo(const Key& key, LocalUncachedObjinfo& local_uncached_objinfo) const
    {
        std::unordered_map<Key, LocalUncachedObjinfo, KeyHasher>::const_iterator key_local_uncached_objinfo_map_iter = key_local_uncached_objinfo_map.find(key);
        
        bool is_exist = (key_local_uncached_objinfo_map_iter != key_local_uncached_objinfo_map.end());
        if (is_exist)
        {
            local_uncached_objinfo = key_local_uncached_objinfo_map_iter->second; // Deep copy
        }

        return is_exist;
    }

    void PerkeyLocalUncachedObjinfo::tryToRemoveLocalUncachedPopularityForEdge(const Key& key, const uint32_t given_edgeidx)
    {
        std::unordered_map<Key, LocalUncachedObjinfo, KeyHasher>::iterator key_local_uncached_objinfo_map_iter = key_local_uncached_objinfo_map.find(key);
        if (key_local_uncached_objinfo_map_iter != key_local_uncached_objinfo_map.end())
        {
            bool is_empty = key_local_uncached_objinfo_map_iter->second.remove(given_edgeidx);
            if (is_empty)
            {
                key_local_uncached_objinfo_map.erase(key_local_uncached_objinfo_map_iter);
            }
        }

        return;
    }

    void PerkeyLocalUncachedObjinfo::addLocalUncachedPopularityForEdge(const Key& key, const uint32_t given_edgeidx, const CollectedPopularity& collected_popularity)
    {
        std::unordered_map<Key, LocalUncachedObjinfo, KeyHasher>::iterator key_local_uncached_objinfo_map_iter = key_local_uncached_objinfo_map.find(key);
        if (key_local_uncached_objinfo_map_iter == key_local_uncached_objinfo_map.end()) // New local uncached object
        {
            LocalUncachedObjinfo tmp_local_uncached_objinfo;
            tmp_local_uncached_objinfo.update(given_edgeidx, collected_popularity);
            key_local_uncached_objinfo_map.insert(std::pair<Key, LocalUncachedObjinfo>(key, tmp_local_uncached_objinfo));
        }
        else // Existing local uncached object
        {
            key_local_uncached_objinfo_map_iter->second.update(given_edgeidx, collected_popularity);
        }

        return;
    }

    // PeredgeEvictNodeinfo

    bool PeredgeEvictNodeinfo::getEvictinfo(const uint32_t& given_edgeidx, EvictNodeinfo& evictinfo) const
    {
        std::unordered_map<uint32_t, EvictNodeinfo>::const_iterator edgeidx_evictinfo_map_iter = edgeidx_evictinfo_map.find(given_edgeidx);
        
        bool is_exist = (edgeidx_evictinfo_map_iter != edgeidx_evictinfo_map.end());
        if (is_exist)
        {
            evictinfo = edgeidx_evictinfo_map_iter->second; // Deep copy
        }

        return is_exist;
    }

    void PeredgeEvictNodeinfo::updateCacheMarginBytesOnly(const uint32_t& given_edgeidx, const uint64_t cache_margin_bytes)
    {
        // Update cache margin bytes ONLY -> NOT affect victim information -> evictinfo of the given edge node MUST exist
        std::unordered_map<uint32_t, EvictNodeinfo>::iterator edgeidx_evictinfo_map_iter = edgeidx_evictinfo_map.find(given_edgeidx);
        assert(edgeidx_evictinfo_map_iter != edgeidx_evictinfo_map.end());

        edgeidx_evictinfo_map_iter->second.cache_margin_bytes = cache_margin_bytes;

        return;
    }
    
    void PeredgeEvictNodeinfo::updateEvictinfo(const uint32_t& given_edgeidx, const uint64_t cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        std::unordered_map<uint32_t, EvictNodeinfo>::iterator edgeidx_evictinfo_map_iter = edgeidx_evictinfo_map.find(given_edgeidx);
        
        if (edgeidx_evictinfo_map_iter == edgeidx_evictinfo_map.end()) // New edge node
        {
            EvictNodeinfo tmp_evictinfo;
            tmp_evictinfo.cache_margin_bytes = cache_margin_bytes;
            tmp_evictinfo.victim_cacheinfos = victim_cacheinfos;
            edgeidx_evictinfo_map.insert(std::pair<uint32_t, EvictNodeinfo>(given_edgeidx, tmp_evictinfo));
        }
        else // Existing edge node
        {
            edgeidx_evictinfo_map_iter->second.cache_margin_bytes = cache_margin_bytes;
            edgeidx_evictinfo_map_iter->second.victim_cacheinfos = victim_cacheinfos;
        }

        return;
    }
}

namespace covered
{
    // (1) Utility functions

    WorkloadItem genWorkloadItemForClientWorker(const uint32_t& client_workeridx, const uint32_t& clientidx, const uint32_t& perclient_workercnt, const uint32_t& client_workercnt, const uint32_t& perworkload_workercnt, const uint32_t& simulator_workloadcnt)
    {
        uint32_t global_client_workeridx = client_workeridx + clientidx * perclient_workercnt;
        assert(global_client_workeridx < client_workercnt); // Must < client_workercnt

        // NOTE: we need to calculate workloadidx and local_workload_workeridx, as clientcnt may NOT equal to workloadcnt in single-node simulator and we cannot directly use clientidx and local_client_workeridx to generate workload items

        // Get workload generator
        uint32_t workloadidx = global_client_workeridx / perworkload_workercnt;
        assert(workloadidx < simulator_workloadcnt); // Must < simulator_workloadcnt
        WorkloadWrapperBase* cur_client_worker_workload_wrapper_ptr = workload_wrapper_ptrs[workloadidx];
        assert(cur_client_worker_workload_wrapper_ptr != NULL);

        // Get workload item
        uint32_t local_workload_workeridx = global_client_workeridx % perworkload_workercnt;
        assert(local_workload_workeridx < perworkload_workercnt); // Must < perworkload_workercnt
        WorkloadItem cur_workload_item = cur_client_worker_workload_wrapper_ptr->generateWorkloadItem(local_workload_workeridx, nullptr);

        return cur_workload_item;
    }

    uint32_t getClosestEdgeidx(const uint32_t& clientidx)
    {
        assert(clientidx < closest_edge_idxes.size());
        
        const uint32_t curclient_closest_edge_idx = closest_edge_idxes[clientidx];
        return curclient_closest_edge_idx;
    }

    EdgeWrapperBase* getEdgeWrapperPtr(const uint32_t& given_edgeidx)
    {
        EdgeWrapperBase* given_edge_wrapper_ptr = edge_wrapper_ptrs[given_edgeidx];
        assert(given_edge_wrapper_ptr != NULL);
        return given_edge_wrapper_ptr;
    }

    EdgeWrapperBase* getClosestEdgeWrapperPtr(const uint32_t& clientidx)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);
        EdgeWrapperBase* curclient_closest_edge_wrapper_ptr = getEdgeWrapperPtr(curclient_closest_edge_idx);
        return curclient_closest_edge_wrapper_ptr;
    }

    CacheWrapper* getClosestEdgeCacheWrapperPtr(const uint32_t& clientidx)
    {
        EdgeWrapperBase* curclient_closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = curclient_closest_edge_wrapper_ptr->getEdgeCachePtr();
        assert(curclient_closest_edge_cache_wrapper_ptr != NULL);
        return curclient_closest_edge_cache_wrapper_ptr;
    }

    uint32_t getBeaconEdgeidx(const Key& cur_key)
    {
        EdgeWrapperBase* first_client_closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(0); // Any edge wrapper can calculate the beacon edge index deterministically
        CooperationWrapperBase* first_client_closest_edge_cooperation_wrapper_ptr = first_client_closest_edge_wrapper_ptr->getCooperationWrapperPtr();
        assert(first_client_closest_edge_cooperation_wrapper_ptr != NULL);
        const uint32_t beacon_edgeidx = first_client_closest_edge_cooperation_wrapper_ptr->getBeaconEdgeIdx(cur_key);
        return beacon_edgeidx;
    }

    EdgeWrapperBase* getBeaconEdgeWrapperPtr(const Key& cur_key)
    {
        const uint32_t beacon_edgeidx = getBeaconEdgeidx(cur_key);
        EdgeWrapperBase* beacon_edge_wrapper_ptr = getEdgeWrapperPtr(beacon_edgeidx);
        return beacon_edge_wrapper_ptr;
    }

    bool isLocalBeacon(const Key& cur_key, const uint32_t& clientidx)
    {
        const uint32_t closest_edge_idx = getClosestEdgeidx(clientidx);
        const uint32_t beacon_edge_idx = getBeaconEdgeidx(cur_key);
        return closest_edge_idx == beacon_edge_idx;
    }

    std::string getStatisticsString(const uint32_t& reqcnt, const uint32_t& local_hitcnt, const uint32_t& remote_hitcnt, const uint64_t& latency_sum, const BandwidthUsage& bandwidth_usage)
    {
        // Calculate hit ratio and latency statistics
        double local_hitratio = static_cast<double>(local_hitcnt) / static_cast<double>(reqcnt);
        double remote_hitratio = static_cast<double>(remote_hitcnt) / static_cast<double>(reqcnt);
        double global_hitratio = local_hitratio + remote_hitratio;
        uint64_t avg_latency = latency_sum / static_cast<uint64_t>(reqcnt);

        // Calculate coarse-grained bandwidth usage statistics

        //const double perpkt_client_edge_bwcost = B2MB(static_cast<double>(bandwidth_usage.getClientEdgeBandwidthBytes()) / static_cast<double>(reqcnt));
        const double perpkt_cross_edge_control_total_bwcost = B2MB(static_cast<double>(bandwidth_usage.getCrossEdgeControlTotalBandwidthBytes()) / static_cast<double>(reqcnt));
        const double perpkt_cross_edge_data_bwcost = B2MB(static_cast<double>(bandwidth_usage.getCrossEdgeDataBandwidthBytes()) / static_cast<double>(reqcnt));
        const double perpkt_cross_edge_bwcost = perpkt_cross_edge_control_total_bwcost + perpkt_cross_edge_data_bwcost;
        const double perpkt_edge_cloud_bwcost = B2MB(static_cast<double>(bandwidth_usage.getEdgeCloudBandwidthBytes()) / static_cast<double>(reqcnt));

        //const double perpkt_client_edge_msgcnt = static_cast<double>(bandwidth_usage.getClientEdgeMsgcnt()) / static_cast<double>(reqcnt);
        const double perpkt_cross_edge_control_total_msgcnt = static_cast<double>(bandwidth_usage.getCrossEdgeControlTotalMsgcnt()) / static_cast<double>(reqcnt);
        const double perpkt_cross_edge_data_msgcnt = static_cast<double>(bandwidth_usage.getCrossEdgeDataMsgcnt()) / static_cast<double>(reqcnt);
        const double perpkt_cross_edge_msgcnt = perpkt_cross_edge_control_total_msgcnt + perpkt_cross_edge_data_msgcnt;
        const double perpkt_edge_cloud_msgcnt = static_cast<double>(bandwidth_usage.getEdgeCloudMsgcnt()) / static_cast<double>(reqcnt);

        // Calculate fine-grained bandwidth usage statistics

        const double perpkt_cross_edge_control_content_discovery_bwcost = B2MB(static_cast<double>(bandwidth_usage.getCrossEdgeControlContentDiscoveryBandwidthBytes()) / static_cast<double>(reqcnt));
        const double perpkt_cross_edge_control_directory_update_bwcost = B2MB(static_cast<double>(bandwidth_usage.getCrossEdgeControlDirectoryUpdateBandwidthBytes()) / static_cast<double>(reqcnt));
        const double perpkt_cross_edge_control_others_bwcost = B2MB(static_cast<double>(bandwidth_usage.getCrossEdgeControlOthersBandwidthBytes()) / static_cast<double>(reqcnt));

        const double perpkt_cross_edge_control_victimsync_bwcost = B2MB(static_cast<double>(bandwidth_usage.getVictimSyncsetBandwidthBytes()) / static_cast<double>(reqcnt));

        const double perpkt_cross_edge_control_content_discovery_msgcnt = static_cast<double>(bandwidth_usage.getCrossEdgeControlContentDiscoveryMsgcnt()) / static_cast<double>(reqcnt);
        const double perpkt_cross_edge_control_directory_update_msgcnt = static_cast<double>(bandwidth_usage.getCrossEdgeControlDirectoryUpdateMsgcnt()) / static_cast<double>(reqcnt);
        const double perpkt_cross_edge_control_others_msgcnt = static_cast<double>(bandwidth_usage.getCrossEdgeControlOthersMsgcnt()) / static_cast<double>(reqcnt);

        // Dump markdown string to help collect statistics
        std::ostringstream oss;
        oss << "Reqcnt: " << reqcnt << ", LocalHitcnt: " << local_hitcnt << ", RemoteHitcnt: " << remote_hitcnt << std::endl;
        oss << "| Global Hit Ratio (%) (Local + Cooperative) | Avg Latency (ms) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge (control + data) / edge-cloud) | Per-request Msgcnt (cross-edge (control + data) / edge-cloud) |" << std::endl;
        oss << "| " << global_hitratio * 100 << " (" << local_hitratio * 100 << " + " << remote_hitratio * 100 << ") | " << static_cast<double>(avg_latency) / 1000.0 << " | " << perpkt_cross_edge_bwcost << " (" << perpkt_cross_edge_control_total_bwcost << " + " << perpkt_cross_edge_data_bwcost << ") / " << perpkt_edge_cloud_bwcost << " | " << perpkt_cross_edge_msgcnt << " (" << perpkt_cross_edge_control_total_msgcnt << " + " << perpkt_cross_edge_data_msgcnt << ") / " << perpkt_edge_cloud_msgcnt << " |" << std::endl;
        // NOTE: inclusive means that victimsync bandwidth cost is already included in cross-edge bandwidth cost (e.g., content discovery, directory update, and others), which is not an independent bandwidth cost and NO need to add to total cross-edge bandwidth cost
        oss << "| Cross-edge Per-request Bandwidth Cost (Content Discovery / Directory Update / Others) (MiB/pkt) | Cross-edge Per-request Msgcnt (Content Discovery / Directory Update / Others) | Cross-edge Per-request Victimsync Bandwidth Cost (MiB/pkt; inclusive) |" << std::endl;
        oss << "| " << perpkt_cross_edge_control_content_discovery_bwcost << " / " << perpkt_cross_edge_control_directory_update_bwcost << " / " << perpkt_cross_edge_control_others_bwcost << " | " << perpkt_cross_edge_control_content_discovery_msgcnt << " / " << perpkt_cross_edge_control_directory_update_msgcnt << " / " << perpkt_cross_edge_control_others_msgcnt << " | " << perpkt_cross_edge_control_victimsync_bwcost << " |";

        return oss.str();
    }

    // (2) Common helper functions

    void processRequest(const WorkloadItem& cur_workload_item, const uint32_t& clientidx, const std::string& cache_name, const uint32_t& edgecnt, const uint32_t& covered_topk_edgecnt, uint32_t& reqcnt, uint32_t& local_hitcnt, uint32_t& remote_hitcnt, uint64_t& latency_sum_us, BandwidthUsage& bandwidth_usage)
    {
        WorkloadItemType cur_workload_item_type = cur_workload_item.getItemType();
        Key cur_key = cur_workload_item.getKey();
        Value cur_value = cur_workload_item.getValue();
        Value fetched_value;

        // Simulate the corresponding closest edge node
        EdgeWrapperBase* curclient_closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(clientidx);

        // Used to calculate latency (calculated performance, yet NOT absolute performance)
        bool is_admit = false;
        bool is_evict = false;
        uint32_t victim_cnt = 0;
        // Common latency
        uint32_t local_access_latency_us = 0;
        uint32_t cloud_access_latency_us = 0;
        uint32_t directory_admit_latency_us = 0;
        uint32_t directory_evict_latency_us = 0;
        // For read request
        uint32_t content_discovery_latency_us = 0;
        uint32_t request_redirection_latency_us = 0;
        // For write request
        uint32_t acquire_writelock_latency_us = 0;
        uint32_t release_writelock_latency_us = 0;
        // Total latency
        uint64_t total_latency_us = 0; // ONLY used for LA-Cache
        // Total bandwidth usage
        BandwidthUsage curpkt_bandwidth_usage; // Bandwidth usage of current packet

        // Update statistics
        reqcnt += 1;

        // Client-edge bandwidth usage (get/put request)
        updateBandwidthUsageForClientRequest(cur_workload_item, clientidx, curpkt_bandwidth_usage);

        if (cur_workload_item_type == WorkloadItemType::kWorkloadItemGet)
        {
            // Access local cache in the closest edge node for local hit/miss (update cache statistics in the closest edge node) (refer to src/edge/cache_server/basic_edge_wrapper.c::getLocalEdgeCache_())
            bool is_local_cached_and_valid = accessClosestCache(cur_key, clientidx, cache_name, fetched_value);
            local_access_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToclientPropagationSimulatorParamPtr()->genPropagationLatency(); // RTT: including both client-to-edge req and edge-to-client rsp

            // Check if any other edge node caches the object
            Hitflag curobj_hitflag = Hitflag::kGlobalMiss;
            if (!is_local_cached_and_valid) // Local miss
            {
                bool is_remote_hit = false;
                uint32_t target_edge_idx = 0;

                // Simulate content discovery (refer to src/cooperation/directory_table.c::lookup())
                contentDiscovery(cur_key, clientidx, cache_name, edgecnt, is_remote_hit, target_edge_idx, curpkt_bandwidth_usage);
                if (!isLocalBeacon(cur_key, clientidx))
                {
                    content_discovery_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->genPropagationLatency();
                }

                if (is_remote_hit) // Remote hit
                {
                    // Access local cache in the remote edge node to simulate request redirection for remote hit (update cache statistics in the remote edge node) (refer to src/edge/cache_server/basic_cache_server_redirection_processor.c::processReqForRedirectedGet_())
                    requestRedirection(cur_key, clientidx, target_edge_idx, cache_name, fetched_value, curpkt_bandwidth_usage);
                    request_redirection_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->genPropagationLatency();

                    // Update statistics
                    remote_hitcnt += 1;

                    curobj_hitflag = Hitflag::kCooperativeHit;
                } // End of remote hit
                else // Global miss
                {
                    // Copy dataset value to fetched_value to simulate cloud access
                    fetched_value = cur_value;
                    cloud_access_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeTocloudPropagationSimulatorParamPtr()->genPropagationLatency();

                    // Edge-cloud bandwidth usage (cloud access request and response)
                    updateBandwidthUsageForGlobalGet(cur_key, fetched_value, clientidx, curpkt_bandwidth_usage);

                    curobj_hitflag = Hitflag::kGlobalMiss;
                } // End of global miss
            } // End of local miss
            else // Local hit
            {
                // NOTE: fetched_value has already been set by accessClosestCache() before

                // Update statistics
                local_hitcnt += 1;

                curobj_hitflag = Hitflag::kLocalHit;
            } // End of local hit

            // Access local cache in the closest edge node to simulate value validation if necessary for fetched value (update value-related statistics in the closest edge node) (refer to src/edge/cache_server/basic_cache_server_worker.c::tryToUpdateInvalidLocalEdgeCache_())
            validateClosestEdgeForFetchedValue(cur_key, fetched_value, clientidx, cache_name, curobj_hitflag, is_evict, victim_cnt);
            if (is_evict)
            {
                directory_evict_latency_us += getDirectoryUpdateLatency(cur_key, clientidx, cache_name); // NOTE: may trigger multiple times of eviction for a request (each time is parallel)

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, false, victim_cnt, curpkt_bandwidth_usage);

                is_evict = false;
                victim_cnt = 0;
            }

            // Calculate cache miss latency for LA-Cache and update COVERED's parameters based on hitflag
            total_latency_us = calcReadLatencyBeforeCacheManagement(cur_key, clientidx, local_access_latency_us, content_discovery_latency_us, request_redirection_latency_us, cloud_access_latency_us, curobj_hitflag, cache_name);

            // Trigger cache management
            triggerCacheManagement(cur_key, fetched_value, clientidx, curobj_hitflag, cache_name, total_latency_us, covered_topk_edgecnt, is_admit, is_evict, victim_cnt);
            if (is_admit)
            {
                directory_admit_latency_us = getDirectoryUpdateLatency(cur_key, clientidx, cache_name);

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, true, 0, curpkt_bandwidth_usage);

                is_admit = false;
            }
            if (is_evict)
            {
                directory_evict_latency_us += getDirectoryUpdateLatency(cur_key, clientidx, cache_name); // NOTE: may trigger multiple times of eviction for a request (each time is parallel)

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, false, victim_cnt, curpkt_bandwidth_usage);

                is_evict = false;
                victim_cnt = 0;
            }

            // Update total latency with directory update latencies
            updateTotalLatencyAfterCacheManagement(cur_key, clientidx, directory_admit_latency_us, directory_evict_latency_us, cache_name, total_latency_us);

            // Client-edge bandwidth usage (get response)
            updateBandwidthUsageForClientGetResponse(cur_key, clientidx, fetched_value, curpkt_bandwidth_usage);

            // Update statistics
            latency_sum_us += total_latency_us;
        } // End of GET request
        else if (cur_workload_item_type == WorkloadItemType::kWorkloadItemPut || cur_workload_item_type == WorkloadItemType::kWorkloadItemDel)
        {
            // NOTE: NO need to acquire writelock, invalidate caches, write cloud, and release writelock (but still simulate the latency)
            // For MSI protocol (with WAN delays ONLY if the object is cached by other cache nodes and beacon node is remote)c
            const bool is_local_beacon = isLocalBeacon(cur_key, clientidx);
            const bool is_neighbor_cached = perkey_global_cached_objinfo.isNeighborCached(cur_key, getClosestEdgeidx(clientidx));
            if (is_neighbor_cached && !is_local_beacon)
            {
                acquire_writelock_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->genPropagationLatency();
                release_writelock_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->genPropagationLatency();

                // Cross-edge bandwidth usage (acquire writelock request and response)
                updateBandwidthUsageForAcquireWritelock(cur_key, clientidx, cache_name, curpkt_bandwidth_usage);

                // Cross-edge bandwidth usage (release writelock request and response)
                updateBandwidthUsageForReleaseWritelock(cur_key, clientidx, cache_name, curpkt_bandwidth_usage);
            }

            // Simulate cloud update for write acknowledgement
            if (cur_workload_item_type == WorkloadItemType::kWorkloadItemPut)
            {
                fetched_value = cur_value;
            }
            else
            {
                fetched_value = Value(); // is_deleted is true by default
            }
            cloud_access_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeTocloudPropagationSimulatorParamPtr()->genPropagationLatency();

            // Edge-cloud bandwidth usage (cloud access request and response)
            updateBandwidthUsageForGlobalPut(cur_key, fetched_value, clientidx, curpkt_bandwidth_usage);

            // Access local cache in each involved edge node to simulate cache update after write acknowledgement (update cache statistics in the closest edge node)
            writeClosestCache(cur_key, fetched_value, clientidx, cache_name, cur_workload_item_type, is_evict, victim_cnt);
            local_access_latency_us = curclient_closest_edge_wrapper_ptr->getEdgeToclientPropagationSimulatorParamPtr()->genPropagationLatency();
            if (is_evict)
            {
                directory_evict_latency_us += getDirectoryUpdateLatency(cur_key, clientidx, cache_name); // NOTE: may trigger multiple times of eviction for a request (each time is parallel)

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, false, victim_cnt, curpkt_bandwidth_usage);

                is_evict = false;
                victim_cnt = 0;
            }

            // Calculate cache miss latency for LA-Cache and update COVERED's parameters based on remote beacon access
            total_latency_us = calcWriteLatencyBeforeCacheManagement(cur_key, clientidx, local_access_latency_us, acquire_writelock_latency_us, release_writelock_latency_us, cloud_access_latency_us, cache_name);

            // Trigger cache management
            Hitflag curobj_hitflag = Hitflag::kGlobalMiss;
            if (cache_name == Util::BESTGUESS_CACHE_NAME && perkey_global_cached_objinfo.isGlobalCached(cur_key))
            {
                // NOTE: BestGuess does NOT admit the object if it has been cached by other cache nodes
                curobj_hitflag = Hitflag::kCooperativeHit;
            }
            triggerCacheManagement(cur_key, fetched_value, clientidx, curobj_hitflag, cache_name, total_latency_us, covered_topk_edgecnt, is_admit, is_evict, victim_cnt);
            if (is_admit)
            {
                directory_admit_latency_us = getDirectoryUpdateLatency(cur_key, clientidx, cache_name);

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, true, 0, curpkt_bandwidth_usage);

                is_admit = false;
            }
            if (is_evict)
            {
                directory_evict_latency_us += getDirectoryUpdateLatency(cur_key, clientidx, cache_name); // NOTE: may trigger multiple times of eviction for a request (each time is parallel)

                // Cross-edge bandwidth usage (directory update request and response)
                updateBandwidthUsageForDirectoryUpdate(cur_key, clientidx, cache_name, false, victim_cnt, curpkt_bandwidth_usage);

                is_evict = false;
                victim_cnt = 0;
            }

            // Update total latency with directory update latencies
            updateTotalLatencyAfterCacheManagement(cur_key, clientidx, directory_admit_latency_us, directory_evict_latency_us, cache_name, total_latency_us);

            // Client-edge bandwidth usage (put response)
            updateBandwidthUsageForClientPutResponse(cur_key, clientidx, curpkt_bandwidth_usage);

            // Update statistics (NOTE: write MUST be cache miss -> NO need to update local hitcnt and remote hitcnt)
            latency_sum_us += total_latency_us;
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid workload item type: " << WorkloadItem::workloadItemTypeToString(cur_workload_item_type);
            Util::dumpErrorMsg("processRequest()", oss.str());
            exit(1);
        }

        // Update bandwidth usage
        bandwidth_usage.update(curpkt_bandwidth_usage);

        return;
    }

    bool accessClosestCache(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, Value& fetched_value)
    {
        const uint32_t closest_edgeidx = getClosestEdgeidx(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

        bool closest_edge_is_redirected = false;
        bool affect_victim_tracker = false;
        bool is_local_cached_and_valid = curclient_closest_edge_cache_wrapper_ptr->get(cur_key, closest_edge_is_redirected, fetched_value, affect_victim_tracker);

        if (cache_name == Util::COVERED_CACHE_NAME && affect_victim_tracker)
        {
            // Simulate victim synchronization of closest edge node (refer to src/edge/covered_edge_wrapper.c::updateCacheManagerForLocalSyncedVictimsInternal_())
            coveredVictimSynchronizationForEdge(closest_edgeidx, false);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME)
        {
            // Simulate vtime synchronization of closest edge node for BestGuess
            bestguessVtimeSynchronizationForEdge(closest_edgeidx);
        }

        return is_local_cached_and_valid;
    }

    void writeClosestCache(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const std::string& cache_name, const WorkloadItemType& cur_workload_item_type, bool& is_evict, uint32_t& victim_cnt)
    {
        const uint32_t closest_edgeidx = getClosestEdgeidx(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

        // Update closest cache if the object is locally cached based on the request type (refer to src/edge/cache_server/cache_server_worker_base.c::processLocalWriteRequest_())
        bool unused_is_local_cached_and_invalid = false;
        bool is_global_cached = perkey_global_cached_objinfo.isGlobalCached(cur_key);
        bool affect_victim_tracker = false;
        if (cur_workload_item_type == WorkloadItemType::kWorkloadItemDel) // value is deleted
        {
            assert(fetched_value.isDeleted());

            unused_is_local_cached_and_invalid = curclient_closest_edge_cache_wrapper_ptr->removeIfInvalidForGetrsp(cur_key, is_global_cached, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            unused_is_local_cached_and_invalid = curclient_closest_edge_cache_wrapper_ptr->updateIfInvalidForGetrsp(cur_key, fetched_value, is_global_cached, affect_victim_tracker);
            
            // NOTE: update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_())
            evictForCapacity(getClosestEdgeidx(clientidx), cache_name, is_evict, victim_cnt);
        }
        UNUSED(unused_is_local_cached_and_invalid);

        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            // Simulate victim synchronization of closest edge node (refer to src/edge/covered_edge_wrapper.c::updateCacheManagerForLocalSyncedVictimsInternal_())
            coveredVictimSynchronizationForEdge(closest_edgeidx, !affect_victim_tracker);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME)
        {
            // Simulate vtime synchronization of closest edge node for BestGuess
            bestguessVtimeSynchronizationForEdge(closest_edgeidx);
        }

        return;
    }

    void contentDiscovery(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, const uint32_t& edgecnt, bool& is_remote_hit, uint32_t& target_edge_idx, BandwidthUsage& curpkt_bandwidth_usage)
    {
        EdgeWrapperBase* beacon_edge_wrapper_ptr = getBeaconEdgeWrapperPtr(cur_key);

        std::vector<uint32_t> curobj_edge_node_idxes;
        bool is_exist_global_cached_objinfo = perkey_global_cached_objinfo.getEdgeNodeIdxes(cur_key, curobj_edge_node_idxes);
        // NOTE: ONLY consider content discovery and request redirection for non-single-node caches
        if (!Util::isSingleNodeCache(cache_name) && is_exist_global_cached_objinfo) // Global cached
        {
            assert(curobj_edge_node_idxes.size() > 0);

            // Remove the closest edge node (source edge node must NOT cache the object)
            for (uint32_t i = 0; i < curobj_edge_node_idxes.size(); i++)
            {
                if (curobj_edge_node_idxes[i] == getClosestEdgeidx(clientidx))
                {
                    curobj_edge_node_idxes.erase(curobj_edge_node_idxes.begin() + i);
                    break;
                }
            }

            if (curobj_edge_node_idxes.size() == 0) // Not cached by other edge nodes
            {
                is_remote_hit = false;
            }
            else // Cached by other edge nodes
            {
                // Get target edge node index
                if (Util::isMagnetLikeCache(cache_name)) // MagNet-like caches
                {
                    std::list<DirectoryInfo> curobj_dirinfo_set;
                    for (uint32_t i = 0; i < curobj_edge_node_idxes.size(); i++)
                    {
                        curobj_dirinfo_set.push_back(DirectoryInfo(curobj_edge_node_idxes[i]));
                    }

                    // Select edge node based on clustering (refer to src/edge/beacon_server/basic_beacon_server.c::processReqToLookupLocalDirectory_())
                    bool unused_is_being_written = false;
                    bool curobj_is_valid_directory_exist = true; // NOTE: src/edge/basic_edge_wrapper.c::clusterForMagnet_() will perform clustering only if is_valid_directory_exist is true (i.e., curobj_dirinfo_set is valid)
                    DirectoryInfo curobj_dirinfo;
                    ClusterForMagnetFuncParam tmp_param_for_clustering(cur_key, curobj_dirinfo_set, unused_is_being_written, curobj_is_valid_directory_exist, curobj_dirinfo);
                    beacon_edge_wrapper_ptr->constCustomFunc(ClusterForMagnetFuncParam::FUNCNAME, &tmp_param_for_clustering); // Simulate clustering in beacon edge node
                    UNUSED(unused_is_being_written);

                    if (curobj_is_valid_directory_exist) // Specific edge node caches the object
                    {
                        target_edge_idx = curobj_dirinfo.getTargetEdgeIdx();
                        assert(target_edge_idx < edgecnt);

                        is_remote_hit = true;
                    }
                    else // NOT cached by the specific edge node
                    {
                        is_remote_hit = false;
                    }
                } // End of MagNet-like caches
                else // Other baselines and COVERED
                {
                    // Randomly select a valid edge node as the target edge node (refer to src/cooperation/directory_table.c::lookup())
                    std::uniform_int_distribution<uint32_t> uniform_dist(0, curobj_edge_node_idxes.size() - 1); // Range of [0, # of directory info - 1]
                    uint32_t random_number = uniform_dist(content_discovery_randgen);
                    assert(random_number < curobj_edge_node_idxes.size());
                    target_edge_idx = curobj_edge_node_idxes[random_number];
                    assert(target_edge_idx < edgecnt);

                    is_remote_hit = true;
                } // End of other baselines and COVERED
            }

            // Cross-edge bandwidth usage (content discovery request and response)
            updateBandwidthUsageForContentDiscovery(cur_key, clientidx, cache_name, curpkt_bandwidth_usage);
        } // End of global cached
        else // Global uncached
        {
            is_remote_hit = false;
        } // End of global uncached

        return;
    }

    void requestRedirection(const Key& cur_key, const uint32_t& clientidx, const uint32_t& target_edge_idx, const std::string& cache_name, Value& fetched_value, BandwidthUsage& curpkt_bandwidth_usage)
    {
        EdgeWrapperBase* curobj_target_edge_wrapper_ptr = getEdgeWrapperPtr(target_edge_idx);
        CacheWrapper* curobj_target_edge_cache_wrapper_ptr = curobj_target_edge_wrapper_ptr->getEdgeCachePtr();
        assert(curobj_target_edge_cache_wrapper_ptr != NULL);

        bool target_edge_is_redirected = true;
        bool affect_victim_tracker = false;
        bool is_cooperative_cached_and_valid = curobj_target_edge_cache_wrapper_ptr->get(cur_key, target_edge_is_redirected, fetched_value, affect_victim_tracker);

        if (cache_name == Util::COVERED_CACHE_NAME && affect_victim_tracker)
        {
            // Simulate victim synchronization of target edge node (refer to src/edge/covered_edge_wrapper.c::updateCacheManagerForLocalSyncedVictimsInternal_())
            coveredVictimSynchronizationForEdge(target_edge_idx, false);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME)
        {
            // Simulate vtime synchronization of target edge node for BestGuess
            bestguessVtimeSynchronizationForEdge(target_edge_idx);
        }

        // Cross-edge bandwidth usage (request redirection request and response)
        updateBandwidthUsageForRequestRedirection(cur_key, fetched_value, clientidx, cache_name, curpkt_bandwidth_usage);

        assert(is_cooperative_cached_and_valid);

        return;
    }

    void validateClosestEdgeForFetchedValue(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const std::string& cache_name, const Hitflag& curobj_hitflag, bool& is_evict, uint32_t& victim_cnt)
    {
        const uint32_t closest_edgeidx = getClosestEdgeidx(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

        bool unused_is_local_cached_and_invalid = false;
        bool is_global_cached = (curobj_hitflag == Hitflag::kLocalHit || curobj_hitflag == Hitflag::kCooperativeHit);
        bool affect_victim_tracker = false;
        if (fetched_value.isDeleted()) // value is deleted
        {
            unused_is_local_cached_and_invalid = curclient_closest_edge_cache_wrapper_ptr->removeIfInvalidForGetrsp(cur_key, is_global_cached, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            unused_is_local_cached_and_invalid = curclient_closest_edge_cache_wrapper_ptr->updateIfInvalidForGetrsp(cur_key, fetched_value, is_global_cached, affect_victim_tracker);
            
            // NOTE: update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_())
            evictForCapacity(getClosestEdgeidx(clientidx), cache_name, is_evict, victim_cnt);
        }
        UNUSED(unused_is_local_cached_and_invalid);

        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            // Simulate victim synchronization of closest edge node (refer to src/edge/covered_edge_wrapper.c::updateCacheManagerForLocalSyncedVictimsInternal_())
            coveredVictimSynchronizationForEdge(closest_edgeidx, !affect_victim_tracker);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME)
        {
            // Simulate vtime synchronization of closest edge node for BestGuess
            bestguessVtimeSynchronizationForEdge(closest_edgeidx);
        }

        return;
    }

    uint64_t calcReadLatencyBeforeCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& local_access_latency_us, const uint32_t& content_discovery_latency_us, const uint32_t& request_redirection_latency_us, const uint32_t& cloud_access_latency_us, const Hitflag& hitflag, const std::string& cache_name)
    {
        // Calculate total response latency based on hitflag
        uint64_t total_latency_us = 0;
        if (hitflag == Hitflag::kLocalHit)
        {
            total_latency_us = local_access_latency_us;
        }
        else if (hitflag == Hitflag::kCooperativeHit)
        {
            total_latency_us = local_access_latency_us + content_discovery_latency_us + request_redirection_latency_us;
        }
        else // Global miss
        {
            total_latency_us = local_access_latency_us + content_discovery_latency_us + cloud_access_latency_us;
        }

        // Update COVERED's parameters
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            EdgeWrapperBase* closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(clientidx);

            // Update probability p (ONLY for content discovery) (refer to src/edge/cache_server/covered_cache_server_worker.c::lookupLocalDirectory_() and getReqToLookupBeaconDirectory_())
            WeightTuner& weight_tuner_ref = closest_edge_wrapper_ptr->getWeightTunerRef();
            const bool is_local_beacon = isLocalBeacon(cur_key, clientidx);
            if (is_local_beacon)
            {
                weight_tuner_ref.incrLocalBeaconAccessCnt();
            }
            else
            {
                weight_tuner_ref.incrRemoteBeaconAccessCnt();
            }

            // Update WAN delays (refer to src/edge/cache_server/covered_cache_server_worker.c and src/edge/cache_server/covered_cache_server.c)
            if (hitflag == Hitflag::kLocalHit)
            {
                // Do nothing
            }
            else if (hitflag == Hitflag::kCooperativeHit)
            {
                if (!is_local_beacon)
                {
                    weight_tuner_ref.updateEwmaCrossedgeLatency(content_discovery_latency_us);
                }
                weight_tuner_ref.updateEwmaCrossedgeLatency(request_redirection_latency_us);
            }
            else // Global miss
            {
                if (!is_local_beacon)
                {
                    weight_tuner_ref.updateEwmaCrossedgeLatency(content_discovery_latency_us);
                }
                weight_tuner_ref.updateEwmaEdgecloudLatency(cloud_access_latency_us);
            }
        }

        return total_latency_us;
    }

    uint64_t calcWriteLatencyBeforeCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& local_access_latency_us, const uint32_t& acquire_writelock_latency_us, const uint32_t& release_writelock_latency_us, const uint32_t& cloud_access_latency_us, const std::string& cache_name)
    {
        const bool is_local_beacon = isLocalBeacon(cur_key, clientidx);
        const bool is_neighbor_cached = perkey_global_cached_objinfo.isNeighborCached(cur_key, getClosestEdgeidx(clientidx));
        const bool is_access_remote_beacon = is_neighbor_cached && !is_local_beacon;

        // Calculate total response latency based on hitflag
        uint64_t total_latency_us = local_access_latency_us + cloud_access_latency_us;
        if (is_access_remote_beacon)
        {
            total_latency_us += (acquire_writelock_latency_us + release_writelock_latency_us);
        }

        // Update COVERED's parameters
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            EdgeWrapperBase* closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(clientidx);

            // Update WAN delays (refer to src/edge/cache_server/covered_cache_server_worker.c and src/edge/cache_server/covered_cache_server.c)
            WeightTuner& weight_tuner_ref = closest_edge_wrapper_ptr->getWeightTunerRef();
            if (is_access_remote_beacon)
            {
                weight_tuner_ref.updateEwmaCrossedgeLatency(acquire_writelock_latency_us);
                weight_tuner_ref.updateEwmaCrossedgeLatency(release_writelock_latency_us);
            }
            weight_tuner_ref.updateEwmaEdgecloudLatency(cloud_access_latency_us);
        }

        return total_latency_us;
    }

    void triggerCacheManagement(const Key& cur_key, const Value& fetched_value, const uint32_t& clientidx, const Hitflag& curobj_hitflag, const std::string& cache_name, const uint64_t& total_latency_us, const uint32_t& covered_topk_edgecnt, bool& is_admit, bool& is_evict, uint32_t& victim_cnt)
    {
        is_admit = false;
        is_evict = false;
        victim_cnt = 0;

        const uint32_t curclient_closest_edgeidx = getClosestEdgeidx(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

        if (cache_name == Util::COVERED_CACHE_NAME) // COVERED
        {
            // Try to simulate popularity aggregation
            bool is_tracked_after_fetch_value = coveredTryPopularityAggregationForClosestEdge(cur_key, clientidx);

            if (is_tracked_after_fetch_value) // Local uncached yet tracked object
            {
                // Simulate fast cache placement in the beacon node (refer to src/core/covered_cache_manager.c::placementCalculation_())
                bool has_best_placement = false;
                std::unordered_set<uint32_t> best_placement_edgeset;
                coveredPlacementCalculation(cur_key, covered_topk_edgecnt, has_best_placement, best_placement_edgeset);

                if (has_best_placement)
                {
                    is_admit = true;
                    coveredPlacementDeployment(cur_key, fetched_value, best_placement_edgeset, curobj_hitflag, cache_name, total_latency_us, is_evict, victim_cnt);
                }
            }
        } // End of COVERED
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // BestGuess (Hint)
        {
            if (curobj_hitflag == Hitflag::kGlobalMiss) // ONLY for global uncached objects
            {
                is_admit = true;

                // BestGuess evicts with approximate global LRU
                uint32_t placement_edgeidx = bestguessGetEdgeidxWithSmallestVtime();
                admitIntoEdge(cur_key, fetched_value, placement_edgeidx, curobj_hitflag, cache_name, total_latency_us, is_evict, victim_cnt);
            }
        }
        else if (Util::isMagnetLikeCache(cache_name)) // MagNet-like caches
        {
            if (curobj_hitflag == Hitflag::kGlobalMiss) // ONLY for objects retrieved from cloud
            {
                if (curclient_closest_edge_cache_wrapper_ptr->needIndependentAdmit(cur_key, fetched_value))
                {
                    is_admit = true;
                    admitIntoEdge(cur_key, fetched_value, curclient_closest_edgeidx, curobj_hitflag, cache_name, total_latency_us, is_evict, victim_cnt);
                }
            }
        }
        else // Single-node caches and Shark-like caches
        {
            if (curclient_closest_edge_cache_wrapper_ptr->needIndependentAdmit(cur_key, fetched_value))
            {
                is_admit = true;
                admitIntoEdge(cur_key, fetched_value, curclient_closest_edgeidx, curobj_hitflag, cache_name, total_latency_us, is_evict, victim_cnt);
            }
        }

        return;
    }

    void updateTotalLatencyAfterCacheManagement(const Key& cur_key, const uint32_t& clientidx, const uint32_t& directory_admit_latency_us, const uint32_t& directory_evict_latency_us, const std::string& cache_name, uint64_t& total_latency_us)
    {
        total_latency_us += (directory_admit_latency_us + directory_evict_latency_us);

        // Update COVERED's parameters
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            EdgeWrapperBase* closest_edge_wrapper_ptr = getClosestEdgeWrapperPtr(clientidx);
            const bool is_local_beacon = isLocalBeacon(cur_key, clientidx);

            // Update WAN delays (refer to src/edge/cache_server/covered_cache_server_worker.c and src/edge/cache_server/covered_cache_server.c)
            WeightTuner& weight_tuner_ref = closest_edge_wrapper_ptr->getWeightTunerRef();
            if (!is_local_beacon)
            {
                if (directory_admit_latency_us > 0)
                {
                    weight_tuner_ref.updateEwmaCrossedgeLatency(directory_admit_latency_us);
                }

                if (directory_evict_latency_us > 0)
                {
                    weight_tuner_ref.updateEwmaCrossedgeLatency(directory_evict_latency_us);
                }
            }
        }

        return;
    }

    void admitIntoEdge(const Key& cur_key, const Value& fetched_value, const uint32_t& placement_edgeidx, const Hitflag& hitflag, const std::string& cache_name, const uint64_t& miss_latency_us, bool& is_evict, uint32_t& victim_cnt)
    {
        is_evict = false;
        victim_cnt = 0;

        assert(placement_edgeidx < edge_wrapper_ptrs.size());
        const uint32_t beacon_edgeidx = getBeaconEdgeidx(cur_key);
        EdgeWrapperBase* placement_edge_wrapper_ptr = getEdgeWrapperPtr(placement_edgeidx);
        CacheWrapper* placement_edge_cache_wrapper_ptr = placement_edge_wrapper_ptr->getEdgeCachePtr();

        // Get original cached edge nodes before admission
        std::vector<uint32_t> original_cached_edge_nodes;
        bool is_neighbor_cached = false;
        bool is_exist_original_global_cached_objinfo = perkey_global_cached_objinfo.getEdgeNodeIdxes(cur_key, original_cached_edge_nodes);
        if (is_exist_original_global_cached_objinfo)
        {
            assert(original_cached_edge_nodes.size() > 0);

            is_neighbor_cached = true;
        }

        // NOTE: the edge node should NOT cache the object
        for (uint32_t i = 0; i < original_cached_edge_nodes.size(); i++)
        {
            if (original_cached_edge_nodes[i] == placement_edgeidx)
            {
                std::ostringstream oss;
                oss << "placement edge node already caches key " << cur_key.getKeyDebugstr();
                Util::dumpWarnMsg("admitIntoEdge()", oss.str());
                return;
            }
        }

        // Admit object into the given placement edge node and evict if necessary (refer to src/edge/cache_server/basic_cache_server.c::admitLocalEdgeCache_())
        bool affect_victim_tracker = false; // If key is a local synced victim now
        const bool is_valid = true; // must NOT being written in single-node simulator
        placement_edge_cache_wrapper_ptr->admit(cur_key, fetched_value, is_neighbor_cached, is_valid, beacon_edgeidx, affect_victim_tracker, miss_latency_us);

        // Add placement edge index into global cached information of the object for all methods after admission
        perkey_global_cached_objinfo.addEdgeNode(cur_key, placement_edgeidx);
        if (cache_name == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // Remove placement edge index from local uncached information of the object for COVERED after admission
            perkey_local_uncached_objinfo.tryToRemoveLocalUncachedPopularityForEdge(cur_key, placement_edgeidx);

            // Update victim information of the placement edge node for COVERED after admission
            coveredVictimSynchronizationForEdge(placement_edgeidx, !affect_victim_tracker);

            // Metadata update (enable is_neighghbor_cached of the object) for COVERED after admission (refer to src/edge/covered_edge_wrapper.c::processMetadataUpdateRequirementInternal_() and src/edge/cache_server/cache_server_metadata_update_processor.c::processMetadataUpdateRequest_())
            bool is_from_single_to_multiple = (original_cached_edge_nodes.size() == 1);
            if (is_from_single_to_multiple)
            {
                const uint32_t notify_edgeidx = original_cached_edge_nodes[0];
                const bool is_neighbor_cached = true; // Enable is_neighbor_cached flag
                coveredMetadataUpdate(cur_key, notify_edgeidx, is_neighbor_cached);
            }
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // ONLY for BestGuess
        {
            // Update vtime information of placement edge node for BestGuess after admission
            bestguessVtimeSynchronizationForEdge(placement_edgeidx);
        }

        // Evict after admission
        evictForCapacity(placement_edgeidx, cache_name, is_evict, victim_cnt);

        return;
    }

    void evictForCapacity(const uint32_t& placement_edgeidx, const std::string& cache_name, bool& is_evict, uint32_t& victim_cnt)
    {
        // Refer to src/edge/cache_server/cache_server_base.c::evictForCapacity_()

        is_evict = false;
        victim_cnt = 0;

        EdgeWrapperBase* edge_wrapper_ptr = getEdgeWrapperPtr(placement_edgeidx);
        CacheWrapper* edge_cache_wrapper_ptr = edge_wrapper_ptr->getEdgeCachePtr();
        assert(edge_cache_wrapper_ptr != NULL);

        // Evict victims from local edge cache in the placement edge node and also update cache size usage
        std::unordered_map<Key, Value, KeyHasher> total_victims;
        total_victims.clear();
        while (true) // Evict until used bytes <= capacity bytes
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint64_t used_bytes = edge_wrapper_ptr->getSizeForCapacity();
            uint64_t capacity_bytes = edge_wrapper_ptr->getCapacityBytes();
            if (used_bytes <= capacity_bytes) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                std::unordered_map<Key, Value, KeyHasher> tmp_victims;
                tmp_victims.clear();

                // NOTE: we calculate required size after locking to avoid duplicate evictions for the same part of required size
                uint64_t required_size = used_bytes - capacity_bytes;

                // Evict unpopular objects from local edge cache for cache capacity
                edge_cache_wrapper_ptr->evict(tmp_victims, required_size);

                total_victims.insert(tmp_victims.begin(), tmp_victims.end());

                continue;
            }
        }

        is_evict = total_victims.size() > 0;
        victim_cnt = total_victims.size();

        // Update global cached information of placement edge node for all methods after eviction
        for (std::unordered_map<Key, Value, KeyHasher>::iterator total_victims_iter = total_victims.begin(); total_victims_iter != total_victims.end(); total_victims_iter++)
        {
            perkey_global_cached_objinfo.removeEdgeNode(total_victims_iter->first, placement_edgeidx);
        }

        if (cache_name == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // NOTE: NO need to update local uncached information of the victims in the current edge node for COVERED after eviction, as the victims were a local cached object (i.e., NO local uncached infomration) before
            // (FUTURE) TODO: Add the current placement_edgeidx into local uncached information of each selected victim for COVERED after eviction, if using metadata preservation

            // Update victim information of the current edge node for COVERED after eviction
            // NOTE: victims MUST be changed
            coveredVictimSynchronizationForEdge(placement_edgeidx, false);

            // Metadata update (disable is_neighghbor_cached of each victim) for COVERED after eviction (refer to src/edge/covered_edge_wrapper.c::processMetadataUpdateRequirementInternal_() and src/edge/cache_server/cache_server_metadata_update_processor.c::processMetadataUpdateRequest_())
            for (std::unordered_map<Key, Value, KeyHasher>::iterator total_victims_iter = total_victims.begin(); total_victims_iter != total_victims.end(); total_victims_iter++)
            {
                const Key tmp_victim_key = total_victims_iter->first;

                std::vector<uint32_t> tmp_victim_cached_edge_nodes;
                bool unused_is_exist_global_cached_objinfo = perkey_global_cached_objinfo.getEdgeNodeIdxes(tmp_victim_key, tmp_victim_cached_edge_nodes);
                UNUSED(unused_is_exist_global_cached_objinfo);
                bool is_from_multiple_to_single = (tmp_victim_cached_edge_nodes.size() == 1);
                if (is_from_multiple_to_single)
                {
                    const uint32_t notify_edgeidx = tmp_victim_cached_edge_nodes[0];
                    const bool is_neighbor_cached = false; // Disable is_neighbor_cached flag
                    coveredMetadataUpdate(tmp_victim_key, notify_edgeidx, is_neighbor_cached);
                }
            }
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // ONLY for BestGuess
        {
            // Update vtime information of current edge node for BestGuess after eviction
            bestguessVtimeSynchronizationForEdge(placement_edgeidx);
        }

        return;
    }

    uint32_t getDirectoryUpdateLatency(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name)
    {
        uint32_t directory_update_latency_us = 0;

        const bool is_local_beacon = isLocalBeacon(cur_key, clientidx);
        if (!is_local_beacon)
        {
            // NOTE: single-node caches do NOT need to update directory information; COVERED and BestGuess use non-blocking directory update due to beacon-triggered cache management (refer to src/edge/cache_server/cache_server_worker_base.c::admitObject_())
            if (!Util::isSingleNodeCache(cache_name) && cache_name != Util::COVERED_CACHE_NAME && cache_name != Util::BESTGUESS_CACHE_NAME)
            {
                directory_update_latency_us = getClosestEdgeWrapperPtr(clientidx)->getEdgeToedgePropagationSimulatorParamPtr()->genPropagationLatency();
            }
        }

        return directory_update_latency_us;
    }

    // (3) COVERED's helper functions

    bool coveredTryPopularityAggregationForClosestEdge(const Key& cur_key, const uint32_t& clientidx)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);
        CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

        // Check if the object is tracked by local uncached metadata in the closest cache node for COVERED (NOTE: already update value-related statistics even if it is the first request on the object) (refer to src/edge/cache_server/covered_edge_wrapper.c::getLocalEdgeCache_())
        GetCollectedPopularityParam tmp_param(cur_key);
        curclient_closest_edge_cache_wrapper_ptr->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);
        const CollectedPopularity tmp_collected_popularity_after_fetch_value = tmp_param.getCollectedPopularityRef();
        const bool is_tracked_after_fetch_value = tmp_collected_popularity_after_fetch_value.isTracked();

        if (is_tracked_after_fetch_value) // Local uncached yet tracked object
        {
            // Simulate popularity aggregation (refer to src/core/popularity_aggregator.c::updateAggregatedUncachedPopularity())
            perkey_local_uncached_objinfo.addLocalUncachedPopularityForEdge(cur_key, curclient_closest_edge_idx, tmp_collected_popularity_after_fetch_value);
        } // End of local uncached yet tracked object
        else // Local cached, or local uncached and untracked object
        {
            // Remove unnecessary popularity information
            perkey_local_uncached_objinfo.tryToRemoveLocalUncachedPopularityForEdge(cur_key, curclient_closest_edge_idx);
        } // End of local cached, or local uncached and untracked object
        
        return is_tracked_after_fetch_value;
    }

    void coveredVictimSynchronizationForEdge(const uint32_t& given_edgeidx, const bool& only_update_cache_margin_bytes)
    {
        EdgeWrapperBase* given_edge_wrapper_ptr = getEdgeWrapperPtr(given_edgeidx);
        CacheWrapper* given_edge_cache_wrapper_ptr = given_edge_wrapper_ptr->getEdgeCachePtr();
        assert(given_edge_cache_wrapper_ptr != NULL);

        uint64_t local_cache_margin_bytes = given_edge_wrapper_ptr->getCacheMarginBytes();
        if (only_update_cache_margin_bytes) // Update cache margin bytes ONLY
        {
            peredge_evictinfo.updateCacheMarginBytesOnly(given_edgeidx, local_cache_margin_bytes);
        }
        else
        {
            GetLocalSyncedVictimCacheinfosParam tmp_param;
            given_edge_cache_wrapper_ptr->constCustomFunc(GetLocalSyncedVictimCacheinfosParam::FUNCNAME, &tmp_param); // NOTE: victim cacheinfos from local edge cache MUST be complete
            
            peredge_evictinfo.updateEvictinfo(given_edgeidx, local_cache_margin_bytes, tmp_param.getVictimCacheinfosConstRef());
        }

        return;
    }

    float coveredCalcEvictionCost(const Key& cur_key, const uint32_t& object_size, const std::unordered_set<uint32_t>& placement_edgeset)
    {
        assert(placement_edgeset.size() > 0);

        EdgeWrapperBase* beacon_edge_wrapper_ptr = getBeaconEdgeWrapperPtr(cur_key);

        float eviction_cost = 0.0;
        std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher> pervictim_edgeset;
        std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher> pervictim_cacheinfos;

        // (1) Find victims from placement edgeset (refer to src/core/victim_tracker.c::findVictimsForPlacement_())

        for (std::unordered_set<uint32_t>::const_iterator placement_edgeset_const_iter = placement_edgeset.begin(); placement_edgeset_const_iter != placement_edgeset.end(); placement_edgeset_const_iter++)
        {
            uint32_t tmp_edge_idx = *placement_edgeset_const_iter;
            EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr(tmp_edge_idx);

            // NOTE: victim information of the current edge node MUST exist
            EvictNodeinfo tmp_evictinfo;
            bool is_exist_evictinfo = peredge_evictinfo.getEvictinfo(tmp_edge_idx, tmp_evictinfo);
            if (!is_exist_evictinfo) // No eviction information -> treat the edge node as empty
            {
                continue;
            }
            const uint64_t tmp_cache_margin_bytes = tmp_evictinfo.cache_margin_bytes;
            const std::list<VictimCacheinfo>& tmp_synced_victim_cacheinfos = tmp_evictinfo.victim_cacheinfos;

            // Check if the current edge node needs more victims, and fetch more victims for the current edge node if necessary
            uint64_t tmp_required_bytes = (object_size > tmp_cache_margin_bytes) ? (object_size - tmp_cache_margin_bytes) : 0;
            uint32_t tmp_synced_victim_bytes = 0; // Total bytes of victims synced by the current edge node
            bool need_extra_victims = false;
            std::list<VictimCacheinfo> tmp_extra_victim_cacheinfos;
            if (tmp_required_bytes > 0) // Without sufficient cache space
            {
                // Sum up object sizes of synced victims in the current edge node
                for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = tmp_synced_victim_cacheinfos.begin(); cacheinfo_list_const_iter != tmp_synced_victim_cacheinfos.end(); cacheinfo_list_const_iter++) // Note that victim_cacheinfos_ follows the ascending order of local rewards
                {
                    uint32_t tmp_victim_object_size = 0;
                    cacheinfo_list_const_iter->getObjectSize(tmp_victim_object_size);
                    tmp_synced_victim_bytes += tmp_victim_object_size;
                }

                if (tmp_synced_victim_bytes < tmp_required_bytes)
                {
                    // Fetch more victims for the current edge node (refer to src/edge/cache_server/cache_server_victim_fetch_processor.c::processVictimFetchRequest_())
                    bool has_victim_key = tmp_edge_wrapper_ptr->getEdgeCachePtr()->fetchVictimCacheinfosForRequiredSize(tmp_extra_victim_cacheinfos, object_size); // NOTE: victim cacheinfos from local edge cache MUST be complete
                    assert(has_victim_key == true);

                    need_extra_victims = true;
                }
            }

            // Final victims of the current edge node
            const std::list<VictimCacheinfo>* tmp_final_victim_cacheinfos_ptr = NULL;
            if (need_extra_victims)
            {
                tmp_final_victim_cacheinfos_ptr = &tmp_extra_victim_cacheinfos;
            }
            else
            {
                tmp_final_victim_cacheinfos_ptr = &tmp_synced_victim_cacheinfos;
            }
            assert(tmp_final_victim_cacheinfos_ptr != NULL);

            // Find victims in the current edge node if without sufficient cache space
            uint64_t tmp_saved_bytes = 0;
            if (tmp_required_bytes > 0) // Without sufficient cache space
            {
                assert(tmp_final_victim_cacheinfos_ptr->size() > 0);

                for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = tmp_final_victim_cacheinfos_ptr->begin(); cacheinfo_list_const_iter != tmp_final_victim_cacheinfos_ptr->end(); cacheinfo_list_const_iter++) // Note that victim_cacheinfos_ follows the ascending order of local rewards
                {
                    const VictimCacheinfo& tmp_victim_cacheinfo = *cacheinfo_list_const_iter;
                    assert(tmp_victim_cacheinfo.isComplete()); // NOTE: victim cacheinfos in edge-level victim metadata of victim tracker MUST be complete
                    const Key& tmp_victim_key = tmp_victim_cacheinfo.getKey();

                    // Update per-victim edgeset
                    if (pervictim_edgeset.find(tmp_victim_key) == pervictim_edgeset.end())
                    {
                        pervictim_edgeset.insert(std::pair<Key, std::unordered_set<uint32_t>>(tmp_victim_key, std::unordered_set<uint32_t>()));
                    }
                    pervictim_edgeset[tmp_victim_key].insert(tmp_edge_idx);

                    // Update per-victim cacheinfos
                    if (pervictim_cacheinfos.find(tmp_victim_key) == pervictim_cacheinfos.end())
                    {
                        pervictim_cacheinfos.insert(std::pair<Key, std::list<VictimCacheinfo>>(tmp_victim_key, std::list<VictimCacheinfo>()));
                    }
                    pervictim_cacheinfos[tmp_victim_key].push_back(tmp_victim_cacheinfo);

                    // Check if we have found sufficient victims for the required bytes
                    ObjectSize tmp_victim_object_size = 0;
                    bool with_complete_victim_object_size = tmp_victim_cacheinfo.getObjectSize(tmp_victim_object_size);
                    assert(with_complete_victim_object_size == true); // NOTE: victim cacheinfo in victim_cacheinfos_ MUST be complete
                    tmp_saved_bytes += tmp_victim_object_size;

                    if (tmp_saved_bytes >= tmp_required_bytes) // With sufficient victims for the required bytes
                    {
                        break;
                    }
                } // End of tmp_victim_cacheinfos_ref in the tmp_edge_idx
            } // End of (object_size > tmp_cache_margin_bytes)

            assert(tmp_saved_bytes >= tmp_required_bytes);
        } // End of involved edge nodes

        // (2) Calculate eviction cost (refer to src/core/victim_tracker.c::coveredCalcEvictionCost())

        // Enumerate found victims to calculate eviction cost
        for (std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>::const_iterator pervictim_edgeset_const_iter = pervictim_edgeset.begin(); pervictim_edgeset_const_iter != pervictim_edgeset.end(); pervictim_edgeset_const_iter++)
        {
            const Key& tmp_victim_key = pervictim_edgeset_const_iter->first;
            const std::unordered_set<uint32_t>& tmp_victim_edgeset_ref = pervictim_edgeset_const_iter->second;

            // Check if the victim edgeset is the last copies of the given key (refer to src/core/victim_tracker.c::isLastCopiesForVictimEdgeset_())
            bool is_last_copies = true;
            std::vector<uint32_t> tmp_victim_cached_edge_node_idxes;
            bool is_exist_victim_global_cached_objinfo = perkey_global_cached_objinfo.getEdgeNodeIdxes(tmp_victim_key, tmp_victim_cached_edge_node_idxes);
            assert(is_exist_victim_global_cached_objinfo);

            for (uint32_t i = 0; i < tmp_victim_cached_edge_node_idxes.size(); i++)
            {
                uint32_t tmp_victim_cached_edge_idx = tmp_victim_cached_edge_node_idxes[i];
                if (tmp_victim_edgeset_ref.find(tmp_victim_cached_edge_idx) == tmp_victim_edgeset_ref.end())
                {
                    is_last_copies = false;
                    break;
                }
            }

            // NOTE: we add each pair of edgeidx and cacheinfo of a victim simultaneously before -> tmp_victim_cacheinfos MUST exist and has the same size as tmp_victim_edgeset_ref
            assert(pervictim_cacheinfos.find(tmp_victim_key) != pervictim_cacheinfos.end());
            const std::list<VictimCacheinfo>& tmp_victim_cacheinfos = pervictim_cacheinfos[tmp_victim_key];
            if (tmp_victim_cacheinfos.size() != tmp_victim_edgeset_ref.size())
            {
                std::ostringstream oss;
                oss << "victim " << tmp_victim_key.getKeyDebugstr() << " has " << tmp_victim_cacheinfos.size() << " cacheinfos but " << tmp_victim_edgeset_ref.size() << " edges in edgeset";
                Util::dumpErrorMsg("coveredCalcEvictionCost()", oss.str());
                exit(1);
            }

            // Calculate eviction cost based on is_last_copies and tmp_victim_cacheinfos
            for (std::list<VictimCacheinfo>::const_iterator victim_cacheinfo_list_const_iter = tmp_victim_cacheinfos.begin(); victim_cacheinfo_list_const_iter != tmp_victim_cacheinfos.end(); victim_cacheinfo_list_const_iter++)
            {
                assert(victim_cacheinfo_list_const_iter->isComplete()); // NOTE: victim cacheinfo from edge-level victim metadata of victim tracker MUST be complete

                // Get local cached and redirected cache popularity of the given victim at the given edge node
                Popularity tmp_local_cached_popularity = 0.0;
                Popularity tmp_redirected_cached_popularity = 0.0;
                bool with_complete_local_cached_popularity = victim_cacheinfo_list_const_iter->getLocalCachedPopularity(tmp_local_cached_popularity);
                assert(with_complete_local_cached_popularity); // NOTE: victim cacheinfo of pervictim_cacheinfos (from peredge_victim_metadata_ in victim tracker) MUST be complete
                bool with_complete_redirected_cached_popularity = victim_cacheinfo_list_const_iter->getRedirectedCachedPopularity(tmp_redirected_cached_popularity);
                assert(with_complete_redirected_cached_popularity); // NOTE: victim cacheinfo of pervictim_cacheinfos (from peredge_victim_metadata_ in victim tracker) MUST be complete

                CalcLocalCachedRewardFuncParam tmp_param(tmp_local_cached_popularity, tmp_redirected_cached_popularity, is_last_copies);
                beacon_edge_wrapper_ptr->constCustomFunc(CalcLocalCachedRewardFuncParam::FUNCNAME, &tmp_param); // Simulate eviction cost calculation in the beacon edge node
                Reward tmp_eviction_cost = tmp_param.getRewardConstRef();
                eviction_cost += tmp_eviction_cost;
            } // End of victim cacheinfos of the current victim
        } // End of involved victims

        return eviction_cost;
    }

    void coveredPlacementCalculation(const Key& cur_key, const uint32_t& covered_topk_edgecnt, bool& has_best_placement, std::unordered_set<uint32_t>& best_placement_edgeset)
    {
        EdgeWrapperBase* beacon_edge_wrapper_ptr = getBeaconEdgeWrapperPtr(cur_key);

        has_best_placement = false;
        
        float max_placement_gain = 0.0;
        std::unordered_set<uint32_t> tmp_best_placement_edgeset; // For preserved edgeset and placement notifications under non-blocking placement deployment

        // (1) Get top-k list of local uncached popularity

        // NOTE: local uncached popularity MUST exist, as we only invoke coveredPlacementCalculation() after invoking coveredTryPopularityAggregationForClosestEdge(), which returns true (i.e., the object is local tracked)
        LocalUncachedObjinfo local_uncached_objinfo;
        bool is_exist_local_uncached_objinfo = perkey_local_uncached_objinfo.getLocalUncachedObjinfo(cur_key, local_uncached_objinfo);
        assert(is_exist_local_uncached_objinfo);

        // Convert into top-k list
        std::vector<std::pair<uint32_t, float>> topk_local_uncached_popularities; // <edgeidx, local uncached popularity>
        local_uncached_objinfo.getTopkLocalUncachedPopularities(covered_topk_edgecnt, topk_local_uncached_popularities);
        const uint32_t tmp_topk_list_length = topk_local_uncached_popularities.size();

        assert(tmp_topk_list_length <= covered_topk_edgecnt);
        assert(tmp_topk_list_length > 0);

        // Calculation total local uncached popularity
        float all_local_uncached_popularity_sum = 0.0;
        for (uint32_t i = 0; i < tmp_topk_list_length; i++)
        {
            all_local_uncached_popularity_sum += topk_local_uncached_popularities[i].second;
        }

        // (2) Greedy-based placement calculation

        // Consider top-k edge nodes with largest local uncached popularity to calculate admission benefit and eviction cost
        const uint32_t tmp_object_size = local_uncached_objinfo.objsize;
        std::unordered_set<uint32_t> tmp_placement_edgeset;
        float topi_local_uncached_popularity_sum = 0.0;
        for (uint32_t topicnt = 1; topicnt <= tmp_topk_list_length; topicnt++)
        {
            // Consider topi edge nodes ordered by local uncached popularity in a descending order

            // Get top-i edge indexes and the sum of their local uncached popularities
            const uint32_t topi_edgeidx = topk_local_uncached_popularities[topicnt - 1].first;
            const float topi_local_uncached_popularity = topk_local_uncached_popularities[topicnt - 1].second;
            tmp_placement_edgeset.insert(topi_edgeidx);
            topi_local_uncached_popularity_sum += topi_local_uncached_popularity;
            assert(Util::isApproxLargerEqual(all_local_uncached_popularity_sum, topi_local_uncached_popularity_sum));

            // Calculate admission benefit (refer to src/core/popularity/aggregated_uncached_popularity.c::calcAdmissionBenefit())
            Popularity sum_minus_topi = Util::popularityNonegMinus(all_local_uncached_popularity_sum, topi_local_uncached_popularity_sum);
            const bool is_global_cached = perkey_global_cached_objinfo.isGlobalCached(cur_key);
            CalcLocalUncachedRewardFuncParam tmp_param(topi_local_uncached_popularity_sum, is_global_cached, sum_minus_topi);
            beacon_edge_wrapper_ptr->constCustomFunc(CalcLocalUncachedRewardFuncParam::FUNCNAME, &tmp_param); // Simulate admission benefit calculation in beacon edge node
            const float tmp_admission_benefit = tmp_param.getRewardConstRef();

            // Calculate eviction cost based on tmp_placement_edgeset (refer to src/core/victim_tracker.c::coveredCalcEvictionCost())
            const float tmp_eviction_cost = coveredCalcEvictionCost(cur_key, tmp_object_size, tmp_placement_edgeset);

            // Calculate placement gain (admission benefit - eviction cost)
            const float tmp_placement_gain = tmp_admission_benefit - tmp_eviction_cost;
            if ((topicnt == 1) || (tmp_placement_gain > max_placement_gain))
            {
                max_placement_gain = tmp_placement_gain;

                tmp_best_placement_edgeset = tmp_placement_edgeset;
            }
        }

        // Update best placement if any
        if (max_placement_gain > 0.0) // Still with positive placement gain
        {
            has_best_placement = true;
            best_placement_edgeset = tmp_best_placement_edgeset; // tmp_best_placement_edgeset is the best placement
        }

        return;
    }

    void coveredPlacementDeployment(const Key& cur_key, const Value& fetched_value, const std::unordered_set<uint32_t>& best_placement_edgeset, const Hitflag& hitflag, const std::string& cache_name, const uint64_t miss_latency_us, bool& is_evict, uint32_t& victim_cnt)
    {
        assert(best_placement_edgeset.size() > 0);

        for (std::unordered_set<uint32_t>::const_iterator best_placement_edgeset_iter = best_placement_edgeset.begin(); best_placement_edgeset_iter != best_placement_edgeset.end(); best_placement_edgeset_iter++)
        {
            const uint32_t tmp_placement_edgeidx = *best_placement_edgeset_iter;
            admitIntoEdge(cur_key, fetched_value, tmp_placement_edgeidx, hitflag, cache_name, miss_latency_us, is_evict, victim_cnt);
        }

        return;
    }

    void coveredMetadataUpdate(const Key& cur_key, const uint32_t& notify_edgeidx, const bool& is_neighbor_cached)
    {
        EdgeWrapperBase* notify_edge_wrapper_ptr = getEdgeWrapperPtr(notify_edgeidx);
        CacheWrapper* notify_edge_cache_wrapper_ptr = notify_edge_wrapper_ptr->getEdgeCachePtr();
        assert(notify_edge_cache_wrapper_ptr != NULL);

        UpdateIsNeighborCachedFlagFuncParam tmp_param(cur_key, is_neighbor_cached);
        notify_edge_cache_wrapper_ptr->customFunc(UpdateIsNeighborCachedFlagFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    // (4) BestGuess's helper functions

    void bestguessVtimeSynchronizationForEdge(const uint32_t& given_edgeidx)
    {
        // Update vtime information for BestGuess (many use cases, e.g., refer to src/edge/cache_server/basic_cache_server_worker.c::getReqToLookupBeaconDirectory_())

        EdgeWrapperBase* edge_wrapper_ptr = getEdgeWrapperPtr(given_edgeidx);

        // Get local victim vtime for vtime synchronization
        GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
        edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
        const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

        // Update vtime for the given edge node
        std::unordered_map<uint32_t, uint64_t>::iterator peredge_victim_vtime_iter = peredge_victim_vtime.find(given_edgeidx);
        if (peredge_victim_vtime_iter == peredge_victim_vtime.end())
        {
            peredge_victim_vtime.insert(std::pair<uint32_t, uint64_t>(given_edgeidx, local_victim_vtime));
        }
        else
        {
            peredge_victim_vtime_iter->second = local_victim_vtime;
        }

        return;
    }

    uint32_t bestguessGetEdgeidxWithSmallestVtime()
    {
        assert(peredge_victim_vtime.size() > 0);

        uint32_t placement_edgeidx = 0;
        uint64_t minimum_vtime = 0;
        for (std::unordered_map<uint32_t, uint64_t>::iterator peredge_victim_vtime_iter = peredge_victim_vtime.begin(); peredge_victim_vtime_iter != peredge_victim_vtime.end(); peredge_victim_vtime_iter++)
        {
            if (peredge_victim_vtime_iter == peredge_victim_vtime.begin() || minimum_vtime > peredge_victim_vtime_iter->second)
            {
                placement_edgeidx = peredge_victim_vtime_iter->first;
                minimum_vtime = peredge_victim_vtime_iter->second;
            }
        }

        return placement_edgeidx;
    }

    // (5) Helper functions for bandwidth calculation

    void updateBandwidthUsageForClientRequest(const WorkloadItem& cur_workload_item, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage)
    {
        MessageBase* unused_req_ptr = MessageBase::getRequestFromWorkloadItem(cur_workload_item, clientidx, network_addr_for_debug, false, false, false, 0);
        assert(unused_req_ptr != NULL);

        curpkt_bandwidth_usage.updateClientEdgeBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

        delete unused_req_ptr;
        unused_req_ptr = NULL;

        return;
    }

    void updateBandwidthUsageForClientGetResponse(const Key& cur_key, const uint32_t& clientidx, const Value& value, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = new LocalGetResponse(cur_key, value, Hitflag::kLocalHit, 0, 0, curclient_closest_edge_idx, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
        assert(unused_req_ptr != NULL);

        curpkt_bandwidth_usage.updateClientEdgeBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

        delete unused_req_ptr;
        unused_req_ptr = NULL;

        return;
    }

    void updateBandwidthUsageForClientPutResponse(const Key& cur_key, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = new LocalPutResponse(cur_key, Hitflag::kGlobalMiss, 0, 0, curclient_closest_edge_idx, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
        assert(unused_req_ptr != NULL);

        curpkt_bandwidth_usage.updateClientEdgeBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

        delete unused_req_ptr;
        unused_req_ptr = NULL;

        return;
    }

    void updateBandwidthUsageForContentDiscovery(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = NULL;
        MessageBase* unused_rsp_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME) // For COVERED
        {
            CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = getClosestEdgeCacheWrapperPtr(clientidx);

            // Check if the object is tracked by local uncached metadata in the closest cache node for COVERED, which caches directory information for tracked objects
            GetCollectedPopularityParam tmp_param(cur_key);
            curclient_closest_edge_cache_wrapper_ptr->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);
            const CollectedPopularity tmp_collected_popularity_before_content_discovery = tmp_param.getCollectedPopularityRef();
            const bool is_tracked_before_content_discovery = tmp_collected_popularity_before_content_discovery.isTracked();

            // Simulate DirectoryCacher for COVERED
            bool with_content_discovery_bandwidth_usage = true;
            if (is_tracked_before_content_discovery)
            {
                // Treat tracked object as with cached directory, i.e., no content discovery bandwidth cost
                with_content_discovery_bandwidth_usage = false;
            }

            // Simulate content discovery request and response
            if (with_content_discovery_bandwidth_usage)
            {
                unused_req_ptr = new covered::CoveredDirectoryLookupRequest(cur_key, tmp_collected_popularity_before_content_discovery, VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
                assert(unused_req_ptr != NULL);

                unused_rsp_ptr = new covered::CoveredDirectoryLookupResponse(cur_key, false, true, DirectoryInfo(), VictimSyncset(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
                assert(unused_rsp_ptr != NULL);
            }
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // For BestGuess
        {
            unused_req_ptr = new BestGuessDirectoryLookupRequest(cur_key, BestGuessSyncinfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new BestGuessDirectoryLookupResponse(cur_key, false, true, DirectoryInfo(), BestGuessSyncinfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else // For other baselines
        {
            unused_req_ptr = new DirectoryLookupRequest(cur_key, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new DirectoryLookupResponse(cur_key, false, true, DirectoryInfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }

        // Cross-edge bandwidth usage (content discovery request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlContentDiscoveryBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Cross-edge bandwidth usage (content discovery response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlContentDiscoveryBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForRequestRedirection(const Key& cur_key, const Value& value, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = NULL;
        MessageBase* unused_rsp_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME) // For COVERED
        {
            unused_req_ptr = new CoveredRedirectedGetRequest(cur_key, VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new CoveredRedirectedGetResponse(cur_key, value, Hitflag::kCooperativeHit, VictimSyncset(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // For BestGuess
        {
            unused_req_ptr = new BestGuessRedirectedGetRequest(cur_key, BestGuessSyncinfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new BestGuessRedirectedGetResponse(cur_key, value, Hitflag::kCooperativeHit, BestGuessSyncinfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else // For other baselines
        {
            unused_req_ptr = new RedirectedGetRequest(cur_key, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new RedirectedGetResponse(cur_key, value, Hitflag::kCooperativeHit, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }

        // Cross-edge bandwidth usage (request redirection request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeDataBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Cross-edge bandwidth usage (request redirection response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeDataBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForGlobalGet(const Key& cur_key, const Value& value, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = new GlobalGetRequest(cur_key, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
        assert(unused_req_ptr != NULL);

        MessageBase* unused_rsp_ptr = new GlobalGetResponse(cur_key, value, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
        assert(unused_rsp_ptr != NULL);

        // Edge-cloud bandwidth usage (cloud access request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateEdgeCloudBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Edge-cloud bandwidth usage (cloud access response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateEdgeCloudBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForGlobalPut(const Key& cur_key, const Value& value, const uint32_t& clientidx, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = new GlobalPutRequest(cur_key, value, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
        assert(unused_req_ptr != NULL);

        MessageBase* unused_rsp_ptr = new GlobalPutResponse(cur_key, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
        assert(unused_rsp_ptr != NULL);

        // Edge-cloud bandwidth usage (cloud access request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateEdgeCloudBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Edge-cloud bandwidth usage (cloud access response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateEdgeCloudBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForDirectoryUpdate(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, const bool& is_admit, const uint32_t& victim_cnt, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = NULL;
        MessageBase* unused_rsp_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME) // For COVERED
        {
            if (is_admit)
            {
                unused_req_ptr = new CoveredDirectoryUpdateRequest(cur_key, is_admit, DirectoryInfo(), VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            }
            else
            {
                unused_req_ptr = new CoveredDirectoryUpdateRequest(cur_key, is_admit, DirectoryInfo(), CollectedPopularity(), VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            }
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new CoveredDirectoryUpdateResponse(cur_key, false, false, VictimSyncset(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // For BestGuess
        {
            unused_req_ptr = new BestGuessDirectoryUpdateRequest(cur_key, is_admit, DirectoryInfo(), BestGuessSyncinfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new BestGuessDirectoryUpdateResponse(cur_key, false, BestGuessSyncinfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else // For other baselines
        {
            unused_req_ptr = new DirectoryUpdateRequest(cur_key, is_admit, DirectoryInfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new DirectoryUpdateResponse(cur_key, false, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }

        // Get msg count
        uint32_t msgcnt = 1;
        if (!is_admit) // Evict multiple victims
        {
            msgcnt = victim_cnt;
            
            assert(msgcnt > 0);
            assert(msgcnt <= 10000); // NOTE: # of evicted victims should not be too large
        }

        // Cross-edge bandwidth usage (directory update request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlDirectoryUpdateBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize() * msgcnt, msgcnt);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Cross-edge bandwidth usage (directory update response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlDirectoryUpdateBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize() * msgcnt, msgcnt);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForAcquireWritelock(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = NULL;
        MessageBase* unused_rsp_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME) // For COVERED
        {
            unused_req_ptr = new CoveredAcquireWritelockRequest(cur_key, CollectedPopularity(), VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new CoveredAcquireWritelockResponse(cur_key, LockResult::kSuccess, VictimSyncset(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // For BestGuess
        {
            unused_req_ptr = new BestGuessAcquireWritelockRequest(cur_key, BestGuessSyncinfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new BestGuessAcquireWritelockResponse(cur_key, LockResult::kSuccess, BestGuessSyncinfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else // For other baselines
        {
            unused_req_ptr = new AcquireWritelockRequest(cur_key, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new AcquireWritelockResponse(cur_key, LockResult::kSuccess, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }

        // Cross-edge bandwidth usage (acquire writelock request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlOthersBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Cross-edge bandwidth usage (acquire writelock response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlOthersBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }

    void updateBandwidthUsageForReleaseWritelock(const Key& cur_key, const uint32_t& clientidx, const std::string& cache_name, BandwidthUsage& curpkt_bandwidth_usage)
    {
        const uint32_t curclient_closest_edge_idx = getClosestEdgeidx(clientidx);

        MessageBase* unused_req_ptr = NULL;
        MessageBase* unused_rsp_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME) // For COVERED
        {
            unused_req_ptr = new CoveredReleaseWritelockRequest(cur_key, CollectedPopularity(), VictimSyncset(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new CoveredReleaseWritelockResponse(cur_key, VictimSyncset(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else if (cache_name == Util::BESTGUESS_CACHE_NAME) // For BestGuess
        {
            unused_req_ptr = new BestGuessReleaseWritelockRequest(cur_key, BestGuessSyncinfo(), curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new BestGuessReleaseWritelockResponse(cur_key, BestGuessSyncinfo(), 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }
        else // For other baselines
        {
            unused_req_ptr = new ReleaseWritelockRequest(cur_key, curclient_closest_edge_idx, network_addr_for_debug, ExtraCommonMsghdr());
            assert(unused_req_ptr != NULL);

            unused_rsp_ptr = new ReleaseWritelockResponse(cur_key, 0, network_addr_for_debug, BandwidthUsage(), EventList(), ExtraCommonMsghdr());
            assert(unused_rsp_ptr != NULL);
        }

        // Cross-edge bandwidth usage (release writelock request)
        if (unused_req_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlOthersBandwidthAndMsgcnt(unused_req_ptr->getMsgBandwidthSize(), 1);

            delete unused_req_ptr;
            unused_req_ptr = NULL;
        }

        // Cross-edge bandwidth usage (release writelock response)
        if (unused_rsp_ptr != NULL)
        {
            curpkt_bandwidth_usage.updateCrossEdgeControlOthersBandwidthAndMsgcnt(unused_rsp_ptr->getMsgBandwidthSize(), 1);

            delete unused_rsp_ptr;
            unused_rsp_ptr = NULL;
        }

        return;
    }
}
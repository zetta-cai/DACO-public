/*
 * Use an array of cache structures to simulate multiple cache nodes, use an array of workload generators to simulate multiple clients, and process requests received by different cache nodes in a round-robin manner by one thread in the current physical machine as a single-node simulator for large-scale performance analysis.
 * 
 * NOTE: single node prototype still runs different components in parallel by multiple threads for both hit ratios and absolute performance, while single node simulator only focuses on hit ratios under large scales without absolute performance.
 * 
 * NOTE: we assume that single-node simulator has a global view of all cache nodes to directly calculate local and remote hit ratios -> NO need content discovery, request redirection, and COVERED's popularity collection and victim synchronization (but still need local cache access to update cache statistics and cache admission/eviction policies for cache management).
 * 
 * NOTE: as the simulator does not consider the absolute performance, NO need message transmissions and also NO need the WAN propagation latency injection (but still measure dynamically changed latencies for COVERED and involved baselines such as LA-Cache -> equal to NO other latencies such as processing latencies in cache nodes, which is acceptable as the propagation latencies are the bottleneck in WAN distributed caching, and single node simulator ONLY focuses on hit ratios instead of absolute performance).
 * 
 * By Siyuan Sheng (2024.08.12).
 */

#include <map> // std::unordered_map
#include <random> // std::mt19937_64, std::uniform_int_distribution
#include <sstream> // std::ostringstream
#include <vector> // std::vector

#include "cli/single_node_cli.h"
#include "common/config.h"
#include "common/key.h"
#include "common/util.h"
#include "common/value.h"
#include "edge/basic_edge_wrapper.h"
#include "edge/covered_edge_wrapper.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    // (1) Global structs and classes

    // Global information of a cached object for single-node simulation
    struct CachedGlobalInfo
    {
        std::vector<uint32_t> edge_node_idxes; // Which edge nodes caching the object
    };

    // (2) Global variables

    // Global information for single-node simulation
    std::unordered_map<covered::Key, covered::CachedGlobalInfo, covered::KeyHasher> perkey_cached_global_info;

    // An array of edge wrappers (use the internal cache structures, e.g., local caches in cache wrapper, dht wrapper in cooperative wrapper, and COVERED's weight tuner) to simulate multiple edge nodes
    std::vector<covered::EdgeWrapperBase*> edge_wrapper_ptrs;

    // An array of workload wrappers to simulate multiple client nodes (NO need metadata, e.g., is_warmup and closest edge index, in client (worker) wrapper, which will be maintained by single-node simulator itself)
    std::vector<covered::WorkloadWrapperBase*> workload_wrapper_ptrs;
    std::vector<uint32_t> closest_edge_idxes; // Closest edge node index for each client node

    // Evaluation variables
    std::mt19937_64 content_discovery_randgen(0); // Used for content discovery simulation
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
    const std::string workload_name = single_node_cli.getWorkloadName();
    const float zipf_alpha = single_node_cli.getZipfAlpha();

    // NOTE: NO need to simulate cloud, as we only focus on hit ratios and can assume that we have global information of the dataset -> just get objects based on workload generator

    // Warmup configurations
    const uint32_t warmup_reqcnt_scale = single_node_cli.getWarmupReqcntScale();

    // (2) Initialize global variables for single-node simulation

    covered::perkey_cached_global_info.clear();

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

    // Initialize for client nodes (refer to src/benchmark/client_wrapper.c::launchClient())
    covered::workload_wrapper_ptrs.resize(clientcnt, NULL);
    covered::closest_edge_idxes.resize(clientcnt, 0);
    for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
    {
        // NOTE: NOT use ClientWrapper::launchEdge, which will invoke NodeWrapperBase::start() to launch multiple threads for absolute performance!
        covered::workload_wrapper_ptrs[clientidx] = covered::WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(clientcnt, clientidx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, covered::WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLIENT, zipf_alpha);
        assert(covered::workload_wrapper_ptrs[clientidx] != NULL);

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

    // Used to dump warmup status per interval
    const uint32_t warmup_interval_us = SEC2US(1);
    struct timespec warmup_cur_timestamp = covered::Util::getCurrentTimespec();
    struct timespec warmup_prev_timestamp = warmup_cur_timestamp;
    uint32_t warmup_interval_reqcnt = 0;
    uint32_t warmup_interval_local_hitcnt = 0;
    uint32_t warmup_interval_remote_hitcnt = 0;

    // Generate requests via workload generators one by one to simulate cache access
    for (uint32_t warmup_reqidx = 0; warmup_reqidx < warmup_reqcnt_limit; warmup_reqidx++)
    {
        // Each client worker needs to generate warmup_reqcnt_limit requests for warmup
        for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
        {
            // Simulate the corresponding client node
            covered::WorkloadWrapperBase* curclient_workload_wrapper_ptr = covered::workload_wrapper_ptrs[clientidx];

            // Simulate the corresponding edge node
            const uint32_t curclient_closest_edge_idx = covered::closest_edge_idxes[clientidx];
            covered::EdgeWrapperBase* curclient_closest_edge_wrapper_ptr = covered::edge_wrapper_ptrs[curclient_closest_edge_idx];
            assert(curclient_closest_edge_wrapper_ptr != NULL);
            covered::CacheWrapper* curclient_closest_edge_cache_wrapper_ptr = curclient_closest_edge_wrapper_ptr->getEdgeCachePtr();
            assert(curclient_closest_edge_cache_wrapper_ptr != NULL);

            for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt; local_client_worker_idx++)
            {
                covered::WorkloadItem cur_workload_item = curclient_workload_wrapper_ptr->generateWorkloadItem(local_client_worker_idx);
                covered::WorkloadItemType cur_workload_item_type = cur_workload_item.getItemType();
                covered::Key cur_key = cur_workload_item.getKey();
                covered::Value cur_value = cur_workload_item.getValue();

                if (cur_workload_item_type == covered::WorkloadItemType::kWorkloadItemGet)
                {
                    // Access local cache in the closest edge node for local hit/miss (update cache statistics in the closest edge node) (refer to src/edge/cache_server/basic_edge_wrapper.c::getLocalEdgeCache_())
                    bool closest_edge_is_redirected = false;
                    covered::Value closest_edge_value;
                    bool unused_affect_victim_tracker = false;
                    bool is_local_cached_and_valid = curclient_closest_edge_cache_wrapper_ptr->get(cur_key, closest_edge_is_redirected, closest_edge_value, unused_affect_victim_tracker);
                    UNUSED(unused_affect_victim_tracker);

                    // Check if any other edge node caches the object
                    if (!is_local_cached_and_valid) // Local miss
                    {
                        // Simulate content discovery (refer to src/cooperation/directory_table.c::lookup())
                        std::unordered_map<covered::Key, covered::CachedGlobalInfo, covered::KeyHasher>::iterator perkey_cached_global_info_iter = covered::perkey_cached_global_info.find(cur_key);
                        if (perkey_cached_global_info_iter != covered::perkey_cached_global_info.end()) // Remote hit
                        {
                            std::vector<uint32_t> curobj_edge_node_idxes = perkey_cached_global_info_iter->second.edge_node_idxes;
                            assert(curobj_edge_node_idxes.size() > 0);

                            // Remove the closest edge node (source edge node must NOT cache the object)
                            for (uint32_t i = 0; i < curobj_edge_node_idxes.size(); i++)
                            {
                                if (curobj_edge_node_idxes[i] == curclient_closest_edge_idx)
                                {
                                    curobj_edge_node_idxes.erase(curobj_edge_node_idxes.begin() + i);
                                    break;
                                }
                            }

                            // Randomly select a valid edge node as the target edge node
                            std::uniform_int_distribution<uint32_t> uniform_dist(0, curobj_edge_node_idxes.size() - 1); // Range of [0, # of directory info - 1]
                            uint32_t random_number = uniform_dist(covered::content_discovery_randgen);
                            assert(random_number < curobj_edge_node_idxes.size());
                            const uint32_t target_edge_idx = curobj_edge_node_idxes[random_number];
                            assert(target_edge_idx < edgecnt);

                            // Access local cache in the remote edge node to simulate request redirection for remote hit (update cache statistics in the remote edge node) (refer to src/edge/cache_server/basic_cache_server_redirection_processor.c::processReqForRedirectedGet_())
                            covered::EdgeWrapperBase* curobj_target_edge_wrapper_ptr = covered::edge_wrapper_ptrs[target_edge_idx];
                            assert(curobj_target_edge_wrapper_ptr != NULL);
                            covered::CacheWrapper* curobj_target_edge_cache_wrapper_ptr = curobj_target_edge_wrapper_ptr->getEdgeCachePtr();
                            assert(curobj_target_edge_cache_wrapper_ptr != NULL);
                            bool target_edge_is_redirected = true;
                            covered::Value target_edge_value;
                            bool is_cooperative_cached_and_valid = curobj_target_edge_cache_wrapper_ptr->get(cur_key, target_edge_is_redirected, target_edge_value, unused_affect_victim_tracker);
                            UNUSED(unused_affect_victim_tracker);

                            // Update warmup status
                            assert(is_cooperative_cached_and_valid);
                            warmup_interval_remote_hitcnt += 1;
                        }
                        else // Global miss
                        {
                            // NOTE: NO need to simulate cloud access for global misses
                        }
                    }
                    else // Local hit
                    {
                        // Update warmup status
                        warmup_interval_local_hitcnt += 1;
                    }

                    // TODO: Update value-related statistics

                    // TODO: Check if the object is tracked by local uncached metadata for COVERED (NOTE: already update value-related statistics even if it is the first request on the object) (refer to src/edge/cache_server/covered_edge_wrapper.c::getLocalEdgeCache_())
                }
                else if (cur_workload_item_type == covered::WorkloadItemType::kWorkloadItemPut || cur_workload_item_type == covered::WorkloadItemType::kWorkloadItemDel)
                {
                    // TODO: Access local cache in the closest edge node for local hit/miss (update cache statistics in the closest edge node)

                    // TODO: Access local cache in the remote edge node to simulate request redirection for remote hit (update cache statistics in the remote edge node)

                    // TODO: validate local cache to simulate write acknowledgement

                    // NOTE: write MUST be cache miss
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid workload item type: " << covered::WorkloadItem::workloadItemTypeToString(cur_workload_item_type);
                    covered::Util::dumpErrorMsg(main_class_name, oss.str());
                    exit(1);
                }
            }
        }

        // Calculate delta time to determine whether to dump warmup status
        warmup_cur_timestamp = covered::Util::getCurrentTimespec();
        double warmup_delta_us = covered::Util::getDeltaTimeUs(warmup_cur_timestamp, warmup_prev_timestamp);
        if (warmup_delta_us >= static_cast<double>(warmup_interval_us))
        {
            // TODO: Dump warmup status per interval

            // Reset warmup status
            warmup_prev_timestamp = warmup_cur_timestamp;
            warmup_interval_reqcnt = 0;
            warmup_interval_local_hitcnt = 0;
            warmup_interval_remote_hitcnt = 0;
        }
    }

    // (4) TODO: Stresstest phase

    // (5) Free global variables

    for (uint32_t edgeidx = 0; edgeidx < edgecnt; edgeidx++)
    {
        assert(covered::edge_wrapper_ptrs[edgeidx] != NULL);
        delete covered::edge_wrapper_ptrs[edgeidx];
        covered::edge_wrapper_ptrs[edgeidx] = NULL;
    }

    for (uint32_t clientidx = 0; clientidx < clientcnt; clientidx++)
    {
        assert(covered::workload_wrapper_ptrs[clientidx] != NULL);
        delete covered::workload_wrapper_ptrs[clientidx];
        covered::workload_wrapper_ptrs[clientidx] = NULL;
    }
}
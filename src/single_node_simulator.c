/*
 * Use an array of cache structures to simulate multiple cache nodes, use an array of workload generators to simulate multiple clients, and process requests received by different cache nodes in a round-robin manner by one thread in the current physical machine as a single-node simulator for large-scale performance analysis.
 * 
 * NOTE: single node prototype still runs different components in parallel by multiple threads for both hit ratios and absolute performance, while single node simulator only focuses on hit ratios under large scales without absolute performance.
 * 
 * NOTE: we assume that single-node simulator has a global view of all cache nodes to directly calculate local and remote hit ratios -> NO need local cache access, content discovery, and request redirection (but still need cache admission/eviction policies).
 * 
 * By Siyuan Sheng (2024.08.12).
 */

#include <map> // std::unordered_map
#include <vector> // std::vector

#include "cli/single_node_cli.h"
#include "common/config.h"
#include "common/key.h"
#include "common/util.h"
#include "common/value.h"

namespace covered
{
    // Global information of a cached object for single-node simulation
    struct CachedGlobalInfo
    {
        std::vector<uint32_t> cache_node_idxes; // Cache node indexes caching this object
    };
}

int main(int argc, char **argv) {
    // Global information for single-node simulation
    std::unordered_map<covered::Key, covered::CachedGlobalInfo, covered::KeyHasher> perkey_cached_global_info;

    // (1) Parse and process different CLI parameters for client/edge/cloud/evaluator
    covered::SingleNodeCLI single_node_cli(argc, argv);
    const std::string main_class_name = covered::Config::getMainClassName();
}
#include "cli/edge_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgeCLI::kClassName("EdgeCLI");

    EdgeCLI::EdgeCLI() : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        cache_name_ = "";
        hash_name_ = "";
        percacheserver_workercnt_ = 0;

        // ONLY for COVERED
        covered_local_uncached_max_mem_usage_bytes_ = 0;
        covered_peredge_synced_victimcnt_ = 0;
        covered_peredge_monitored_victimsetcnt_ = 0;
        covered_popularity_aggregation_max_mem_usage_bytes_ = 0;
        covered_popularity_collection_change_ratio_ = double(0.0);
        covered_topk_edgecnt_ = 0;
    }

    EdgeCLI::EdgeCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    EdgeCLI::~EdgeCLI() {}

    std::string EdgeCLI::getCacheName() const
    {
        return cache_name_;
    }

    std::string EdgeCLI::getHashName() const
    {
        return hash_name_;
    }

    uint32_t EdgeCLI::getPercacheserverWorkercnt() const
    {
        return percacheserver_workercnt_;
    }

    // ONLY for COVERED

    uint64_t EdgeCLI::getCoveredLocalUncachedMaxMemUsageBytes() const
    {
        return covered_local_uncached_max_mem_usage_bytes_;
    }

    uint32_t EdgeCLI::getCoveredPeredgeSyncedVictimcnt() const
    {
        return covered_peredge_synced_victimcnt_;
    }

    uint32_t EdgeCLI::getCoveredPeredgeMonitoredVictimsetcnt() const
    {
        return covered_peredge_monitored_victimsetcnt_;
    }

    uint64_t EdgeCLI::getCoveredPopularityAggregationMaxMemUsageBytes() const
    {
        return covered_popularity_aggregation_max_mem_usage_bytes_;
    }

    double EdgeCLI::getCoveredPopularityCollectionChangeRatio() const
    {
        return covered_popularity_collection_change_ratio_;
    }

    uint32_t EdgeCLI::getCoveredTopkEdgecnt() const
    {
        return covered_topk_edgecnt_;
    }

    void EdgeCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            EdgescaleCLI::addCliParameters_();
            PropagationCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("cache_name", boost::program_options::value<std::string>()->default_value(Util::LRU_CACHE_NAME), "cache name (e.g., cachelib, lruk, gdsize, gdsf, lfuda, lfu, lhd, lru, segcache, and covered)")
                ("hash_name", boost::program_options::value<std::string>()->default_value(Util::MMH3_HASH_NAME, "the type of consistent hashing for DHT (e.g., mmh3)"))
                ("percacheserver_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each cache server")
                ("covered_local_uncached_max_mem_usage_mb", boost::program_options::value<uint64_t>()->default_value(1), "the maximum memory usage for local uncached metadata in units of MiB (only for COVERED)")
                ("covered_peredge_synced_victimcnt", boost::program_options::value<uint32_t>()->default_value(3), "per-edge number of victims synced to each neighbor (only for COVERED)")
                ("covered_peredge_monitored_victimsetcnt", boost::program_options::value<uint32_t>()->default_value(3), "per-edge number of monitored victim syncsets for each neighbor (only for COVERED)")
                ("covered_popularity_aggregation_max_mem_usage_mb", boost::program_options::value<uint64_t>()->default_value(1), "the maximum memory usage for popularity aggregation in units of MiB (only for COVERED)")
                ("covered_popularity_collection_change_ratio", boost::program_options::value<double>()->default_value(0.0), "the ratio for local uncached popularity changes to trigger popularity collection (only for COVERED)")
                ("covered_topk_edgecnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of top-k edge nodes for popularity aggregation and trade-off-aware cache placement (only for COVERED)")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void EdgeCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            EdgescaleCLI::setParamAndConfig_(main_class_name);
            PropagationCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            std::string cache_name = argument_info_["cache_name"].as<std::string>();
            std::string hash_name = argument_info_["hash_name"].as<std::string>();
            uint32_t percacheserver_workercnt = argument_info_["percacheserver_workercnt"].as<uint32_t>();
            // ONLY for COVERED
            uint64_t covered_local_uncached_max_mem_usage_bytes = MB2B(argument_info_["covered_local_uncached_max_mem_usage_mb"].as<uint64_t>()); // In units of bytes
            uint32_t covered_peredge_synced_victimcnt = argument_info_["covered_peredge_synced_victimcnt"].as<uint32_t>();
            uint32_t covered_peredge_monitored_victimsetcnt = argument_info_["covered_peredge_monitored_victimsetcnt"].as<uint32_t>();
            uint64_t covered_popularity_aggregation_max_mem_usage_bytes = MB2B(argument_info_["covered_popularity_aggregation_max_mem_usage_mb"].as<uint64_t>()); // In units of bytes
            double covered_popularity_collection_change_ratio = argument_info_["covered_popularity_collection_change_ratio"].as<double>();
            uint32_t covered_topk_edgecnt = argument_info_["covered_topk_edgecnt"].as<uint32_t>();

            // Store edgecnt CLI parameters for dynamic configurations
            cache_name_ = cache_name;
            checkCacheName_();
            hash_name_ = hash_name;
            checkHashName_();
            percacheserver_workercnt_ = percacheserver_workercnt;
            // ONLY for COVERED
            if (cache_name == Util::COVERED_CACHE_NAME)
            {
                const uint64_t capacity_bytes = EdgescaleCLI::getCapacityBytes();
                if (capacity_bytes * Config::getCoveredLocalUncachedMaxMemUsageRatio() >= covered_local_uncached_max_mem_usage_bytes)
                {
                    covered_local_uncached_max_mem_usage_bytes_ = covered_local_uncached_max_mem_usage_bytes;
                }
                else
                {
                    covered_local_uncached_max_mem_usage_bytes_ = capacity_bytes * Config::getCoveredLocalUncachedMaxMemUsageRatio();
                }
                covered_peredge_synced_victimcnt_ = covered_peredge_synced_victimcnt;
                covered_peredge_monitored_victimsetcnt_ = covered_peredge_monitored_victimsetcnt;
                if (capacity_bytes * Config::getCoveredPopularityAggregationMaxMemUsageRatio() >= covered_popularity_aggregation_max_mem_usage_bytes)
                {
                    covered_popularity_aggregation_max_mem_usage_bytes_ = covered_popularity_aggregation_max_mem_usage_bytes;
                }
                else
                {
                    covered_popularity_aggregation_max_mem_usage_bytes_ = capacity_bytes * Config::getCoveredPopularityAggregationMaxMemUsageRatio();
                }
                covered_popularity_collection_change_ratio_ = covered_popularity_collection_change_ratio;
                covered_topk_edgecnt_ = covered_topk_edgecnt;
            }

            verifyIntegrity_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void EdgeCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            EdgescaleCLI::dumpCliParameters_();
            PropagationCLI::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Cache name: " << cache_name_ << std::endl;
            oss << "Hash name: " << hash_name_ << std::endl;
            oss << "Per-cache-server worker count:" << percacheserver_workercnt_;
            if (cache_name_ == Util::COVERED_CACHE_NAME)
            {
                // ONLY for COVERED
                oss << std::endl << "Covered local uncached max mem usage (bytes): " << covered_local_uncached_max_mem_usage_bytes_ << std::endl;
                oss << "Covered per-edge-node synced victim count:" << covered_peredge_synced_victimcnt_ << std::endl;
                oss << "Covered per-edge-node monitored victim syncset count:" << covered_peredge_monitored_victimsetcnt_ << std::endl;
                oss << "Covered popularity aggregation max mem usage (bytes): " << covered_popularity_aggregation_max_mem_usage_bytes_ << std::endl;
                oss << "Covered popularity collection change ratio: " << covered_popularity_collection_change_ratio_ << std::endl;
                oss << "Covered top-k edge count: " << covered_topk_edgecnt_;
                Util::dumpDebugMsg(kClassName, oss.str());   
            }

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void EdgeCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            is_create_required_directories_ = true;
        }
        return;
    }

    void EdgeCLI::checkCacheName_() const
    {
        if (cache_name_ != Util::CACHELIB_CACHE_NAME && cache_name_ != Util::LRUK_CACHE_NAME && cache_name_ != Util::GDSIZE_CACHE_NAME && cache_name_ != Util::GDSF_CACHE_NAME && cache_name_ != Util::LFUDA_CACHE_NAME && cache_name_ != Util::LFU_CACHE_NAME && cache_name_ != Util::LHD_CACHE_NAME && cache_name_ != Util::LRU_CACHE_NAME && cache_name_ != Util::SEGCACHE_CACHE_NAME && cache_name_ != Util::COVERED_CACHE_NAME)
        {
            std::ostringstream oss;
            oss << "cache name " << cache_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeCLI::checkHashName_() const
    {
        if (hash_name_ != Util::MMH3_HASH_NAME)
        {
            std::ostringstream oss;
            oss << "hash name " << hash_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeCLI::verifyIntegrity_() const
    {
        assert(percacheserver_workercnt_ > 0);
        // ONLY for COVERED
        if (cache_name_ == Util::COVERED_CACHE_NAME)
        {
            const uint64_t capacity_bytes = EdgescaleCLI::getCapacityBytes();
            assert(covered_local_uncached_max_mem_usage_bytes_ > 0 && covered_local_uncached_max_mem_usage_bytes_ < capacity_bytes);
            assert(covered_peredge_synced_victimcnt_ > 0);
            assert(covered_peredge_monitored_victimsetcnt_ > 0);
            assert(covered_popularity_aggregation_max_mem_usage_bytes_ > 0 && covered_popularity_aggregation_max_mem_usage_bytes_ < capacity_bytes);
            assert(covered_popularity_collection_change_ratio_ >= 0.0);
            assert(covered_topk_edgecnt_ > 0 && covered_topk_edgecnt_ <= getEdgecnt());
        }
        
        return;
    }
}
#include "cli/edge_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgeCLI::DEFAULT_CACHE_NAME = "lru"; // NOTE: NOT use UTil::LRU_CACHE_NAME due to undefined initialization order of C++ static variables
    const std::string EdgeCLI::DEFAULT_HASH_NAME = "mmh3"; // NOTE: NOT use UTil::MMH3_HASH_NAME due to undefined initialization order of C++ static variables
    const uint32_t EdgeCLI::DEFAULT_PERCACHESERVER_WORKERCNT = 1;
    const uint64_t EdgeCLI::DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB = 1;
    const uint32_t EdgeCLI::DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT = 3;
    const uint32_t EdgeCLI::DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT = 3;
    const uint64_t EdgeCLI::DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB = 1;
    const double EdgeCLI::DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO = double(0.1);
    const uint32_t EdgeCLI::DEFAULT_COVERED_TOPK_EDGECNT = 1;

    const std::string EdgeCLI::kClassName("EdgeCLI");

    EdgeCLI::EdgeCLI() : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        cache_name_ = "";
        hash_name_ = "";
        percacheserver_workercnt_ = 0;

        // ONLY used by COVERED
        covered_local_uncached_max_mem_usage_bytes_ = 0;
        covered_peredge_synced_victimcnt_ = 0;
        covered_peredge_monitored_victimsetcnt_ = 0;
        covered_popularity_aggregation_max_mem_usage_bytes_ = 0;
        covered_popularity_collection_change_ratio_ = double(0.0);
        covered_topk_edgecnt_ = 0;
    }

    EdgeCLI::EdgeCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
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

    // ONLY used by COVERED

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

    std::string EdgeCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << EdgescaleCLI::toCliString();
            oss << PropagationCLI::toCliString();
            if (cache_name_ != DEFAULT_CACHE_NAME)
            {
                oss << " --cache_name " << cache_name_;
            }
            if (hash_name_ != DEFAULT_HASH_NAME)
            {
                oss << " --hash_name " << hash_name_;
            }
            if (percacheserver_workercnt_ != DEFAULT_PERCACHESERVER_WORKERCNT)
            {
                oss << " --percacheserver_workercnt " << percacheserver_workercnt_;
            }
            // ONLY used by COVERED
            if (cache_name_ == Util::COVERED_CACHE_NAME)
            {
                if (covered_local_uncached_max_mem_usage_bytes_ != MB2B(DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB))
                {
                    oss << " --covered_local_uncached_max_mem_usage_mb " << B2MB(covered_local_uncached_max_mem_usage_bytes_);
                }
                if (covered_peredge_synced_victimcnt_ != DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT)
                {
                    oss << " --covered_peredge_synced_victimcnt " << covered_peredge_synced_victimcnt_;
                }
                if (covered_peredge_monitored_victimsetcnt_ != DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT)
                {
                    oss << " --covered_peredge_monitored_victimsetcnt " << covered_peredge_monitored_victimsetcnt_;
                }
                if (covered_popularity_aggregation_max_mem_usage_bytes_ != MB2B(DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB))
                {
                    oss << " --covered_popularity_aggregation_max_mem_usage_mb " << B2MB(covered_popularity_aggregation_max_mem_usage_bytes_);
                }
                if (covered_popularity_collection_change_ratio_ != DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO)
                {
                    oss << " --covered_popularity_collection_change_ratio " << covered_popularity_collection_change_ratio_;
                }
                if (covered_topk_edgecnt_ != DEFAULT_COVERED_TOPK_EDGECNT)
                {
                    oss << " --covered_topk_edgecnt " << covered_topk_edgecnt_;
                }
            }

            is_to_cli_string_ = true;
        }

        return oss.str();
    }

    void EdgeCLI::clearIsToCliString()
    {
        EdgescaleCLI::clearIsToCliString();
        PropagationCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
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
                ("cache_name", boost::program_options::value<std::string>()->default_value(DEFAULT_CACHE_NAME), "cache name (e.g., arc, arc+, bestguess, cachelib, cachelib+, fifo, fifo+, frozenhot, frozenhot+, glcache, glcache+, lrb, lrb+, lruk, lruk+, gdsize, gdsize+, gdsf, gdsf+, lfuda, lfuda+, lfu, lfu+, lhd, lhd+, lru, shark, s3fifo, s3fifo+, segcache, segcache+, sieve, sieve+, slru, slru+, wtinylfu, wtinylfu+, and covered)")
                ("hash_name", boost::program_options::value<std::string>()->default_value(DEFAULT_HASH_NAME, "the type of consistent hashing for DHT (e.g., mmh3)"))
                ("percacheserver_workercnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PERCACHESERVER_WORKERCNT), "the number of worker threads for each cache server")
                ("covered_local_uncached_max_mem_usage_mb", boost::program_options::value<uint64_t>()->default_value(DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB), "the maximum memory usage for local uncached metadata in units of MiB (only used by COVERED)")
                ("covered_peredge_synced_victimcnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT), "per-edge number of victims synced to each neighbor (only used by COVERED)")
                ("covered_peredge_monitored_victimsetcnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT), "per-edge number of monitored victim syncsets for each neighbor (only used by COVERED)")
                ("covered_popularity_aggregation_max_mem_usage_mb", boost::program_options::value<uint64_t>()->default_value(DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB), "the maximum memory usage for popularity aggregation in units of MiB (only used by COVERED)")
                ("covered_popularity_collection_change_ratio", boost::program_options::value<double>()->default_value(DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO), "the ratio for local uncached popularity changes to trigger popularity collection (only used by COVERED)")
                ("covered_topk_edgecnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_COVERED_TOPK_EDGECNT), "the number of top-k edge nodes for popularity aggregation and trade-off-aware cache placement (only used by COVERED)")
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
            // ONLY used by COVERED
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
            // ONLY used by COVERED
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
                // ONLY used by COVERED
                oss << std::endl << "Covered local uncached max mem usage (bytes): " << covered_local_uncached_max_mem_usage_bytes_ << std::endl;
                oss << "Covered per-edge-node synced victim count:" << covered_peredge_synced_victimcnt_ << std::endl;
                oss << "Covered per-edge-node monitored victim syncset count:" << covered_peredge_monitored_victimsetcnt_ << std::endl;
                oss << "Covered popularity aggregation max mem usage (bytes): " << covered_popularity_aggregation_max_mem_usage_bytes_ << std::endl;
                oss << "Covered popularity collection change ratio: " << covered_popularity_collection_change_ratio_ << std::endl;
                oss << "Covered top-k edge count: " << covered_topk_edgecnt_;
            }
            Util::dumpDebugMsg(kClassName, oss.str());

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
        if (cache_name_ != Util::ARC_CACHE_NAME && cache_name_ != Util::EXTENDED_ARC_CACHE_NAME && cache_name_ != Util::BESTGUESS_CACHE_NAME && cache_name_ != Util::CACHELIB_CACHE_NAME && cache_name_ != Util::EXTENDED_CACHELIB_CACHE_NAME && cache_name_ != Util::FIFO_CACHE_NAME && cache_name_ != Util::EXTENDED_FIFO_CACHE_NAME && cache_name_ != Util::FROZENHOT_CACHE_NAME && cache_name_ != Util::EXTENDED_FROZENHOT_CACHE_NAME && cache_name_ != Util::GLCACHE_CACHE_NAME && cache_name_ != Util::EXTENDED_GLCACHE_CACHE_NAME && cache_name_ != Util::LRUK_CACHE_NAME && cache_name_ != Util::EXTENDED_LRUK_CACHE_NAME && cache_name_ != Util::GDSIZE_CACHE_NAME && cache_name_ != Util::EXTENDED_GDSIZE_CACHE_NAME && cache_name_ != Util::GDSF_CACHE_NAME && cache_name_ != Util::EXTENDED_GDSF_CACHE_NAME && cache_name_ != Util::LFUDA_CACHE_NAME && cache_name_ != Util::EXTENDED_LFUDA_CACHE_NAME && cache_name_ != Util::LFU_CACHE_NAME && cache_name_ != Util::EXTENDED_LFU_CACHE_NAME && cache_name_ != Util::LHD_CACHE_NAME && cache_name_ != Util::EXTENDED_LHD_CACHE_NAME && cache_name_ != Util::LRB_CACHE_NAME && cache_name_ != Util::EXTENDED_LRB_CACHE_NAME && cache_name_ != Util::LRU_CACHE_NAME &&cache_name_ != Util::EXTENDED_LRU_CACHE_NAME  && cache_name_ != Util::S3FIFO_CACHE_NAME && cache_name_ != Util::EXTENDED_S3FIFO_CACHE_NAME && cache_name_ != Util::SEGCACHE_CACHE_NAME && cache_name_ != Util::EXTENDED_SEGCACHE_CACHE_NAME && cache_name_ != Util::SIEVE_CACHE_NAME && cache_name_ != Util::EXTENDED_SIEVE_CACHE_NAME && cache_name_ != Util::SLRU_CACHE_NAME && cache_name_ != Util::EXTENDED_SLRU_CACHE_NAME && cache_name_ != Util::WTINYLFU_CACHE_NAME && cache_name_ != Util::EXTENDED_WTINYLFU_CACHE_NAME && cache_name_ != Util::COVERED_CACHE_NAME)
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
        // ONLY used by COVERED
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
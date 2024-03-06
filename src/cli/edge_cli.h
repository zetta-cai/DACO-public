/*
 * EdgeCLI: parse and process edge CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGE_CLI_H
#define EDGE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/edgescale_cli.h"
#include "cli/propagation_cli.h"
#include "cli/workload_cli.h"

namespace covered
{
    class EdgeCLI : virtual public WorkloadCLI, virtual public EdgescaleCLI, virtual public PropagationCLI
    {
    public:
        EdgeCLI();
        EdgeCLI(int argc, char **argv);
        virtual ~EdgeCLI();

        std::string getCacheName() const;
        std::string getHashName() const;
        uint32_t getPercacheserverWorkercnt() const;

        // ONLY used by COVERED
        uint64_t getCoveredLocalUncachedMaxMemUsageBytes() const;
        uint32_t getCoveredPeredgeSyncedVictimcnt() const;
        uint32_t getCoveredPeredgeMonitoredVictimsetcnt() const;
        uint64_t getCoveredPopularityAggregationMaxMemUsageBytes() const;
        double getCoveredPopularityCollectionChangeRatio() const;
        uint32_t getCoveredTopkEdgecnt() const;

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const std::string DEFAULT_CACHE_NAME;
        static const std::string DEFAULT_HASH_NAME;
        static const uint32_t DEFAULT_PERCACHESERVER_WORKERCNT;
        static const uint64_t DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB;
        static const uint32_t DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT;
        static const uint32_t DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT;
        static const uint64_t DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB;
        static const double DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO;
        static const uint32_t DEFAULT_COVERED_TOPK_EDGECNT;

        static const std::string kClassName;

        void checkCacheName_() const;
        void checkHashName_() const;
        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;

        std::string cache_name_;
        std::string hash_name_;
        uint32_t percacheserver_workercnt_;

        // ONLY used by COVERED
        uint64_t covered_local_uncached_max_mem_usage_bytes_;
        uint32_t covered_peredge_synced_victimcnt_; // Max # of victim cacheinfos tracked by an edge node for each neighbor
        uint32_t covered_peredge_monitored_victimsetcnt_; // Max # of victim syncsets monitored by an edge node for each neighbor
        uint64_t covered_popularity_aggregation_max_mem_usage_bytes_;
        double covered_popularity_collection_change_ratio_;
        uint32_t covered_topk_edgecnt_;

    protected:
        virtual void addCliParameters_() override;
        virtual void setParamAndConfig_(const std::string& main_class_name) override;
        virtual void verifyAndDumpCliParameters_(const std::string& main_class_name) override;
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif
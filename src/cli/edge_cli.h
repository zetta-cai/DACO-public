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

namespace covered
{
    class EdgeCLI : virtual public EdgescaleCLI, virtual public PropagationCLI
    {
    public:
        EdgeCLI();
        EdgeCLI(int argc, char **argv);
        virtual ~EdgeCLI();

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        std::string getHashName() const;
        uint32_t getPercacheserverWorkercnt() const;
        uint32_t getPeredgeSyncedVictimcnt() const;

        // ONLY for COVERED
        uint64_t getCoveredLocalUncachedMaxMemUsageBytes() const;
        uint64_t getCoveredPopularityAggregationMaxMemUsageBytes() const;
        double getCoveredPopularityCollectionChangeRatio() const;
        uint32_t getCoveredTopkEdgecnt() const;
    private:
        static const std::string kClassName;

        void checkCacheName_() const;
        void checkHashName_() const;
        void verifyIntegrity_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        std::string cache_name_;
        uint64_t capacity_bytes_;
        std::string hash_name_;
        uint32_t percacheserver_workercnt_;
        uint32_t peredge_synced_victimcnt_;

        // ONLY for COVERED
        uint64_t covered_local_uncached_max_mem_usage_bytes_;
        uint64_t covered_popularity_aggregation_max_mem_usage_bytes_;
        double covered_popularity_collection_change_ratio_;
        uint32_t covered_topk_edgecnt_;

    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif
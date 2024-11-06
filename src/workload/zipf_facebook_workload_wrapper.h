/*
 * ZipfFacebookWorkloadWrapper: use Zipfian power law distribution as mentioned in CacheLib paper to tune skewness of Facebook CDN workload, yet still follow the original key size and value size distribution (thread safe).
 * 
 * See more notes in src/workload/facebook_workload_wrapper.h.
 * 
 * By Siyuan Sheng (2024.04.02).
 */

#ifndef ZIPF_FACEBOOK_WORKLOAD_WRAPPER_H
#define ZIPF_FACEBOOK_WORKLOAD_WRAPPER_H

#include <string>
#include <unordered_map>
#include <vector>

//#include <cachelib/cachebench/util/CacheConfig.h>

//#include <cachelib/cachebench/workload/GeneratorBase.h>
#include "workload/cachebench/generator_base.h"

#include "workload/workload_item.h"
#include "workload/cachebench/cachebench_config.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ZipfFacebookWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        ZipfFacebookWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase);
        virtual ~ZipfFacebookWorkloadWrapper();

        // Access by multiple client workers (thread safe)
        virtual uint32_t getPracticalKeycnt() const override;
        virtual WorkloadItem getDatasetItem(const uint32_t itemidx) override; // Get a dataset key-value pair item with the index of itemidx

        // Get average/min/max dataset key/value size
        virtual double getAvgDatasetKeysize() const override;
        virtual double getAvgDatasetValuesize() const override;
        virtual uint32_t getMinDatasetKeysize() const override;
        virtual uint32_t getMinDatasetValuesize() const override;
        virtual uint32_t getMaxDatasetKeysize() const override;
        virtual uint32_t getMaxDatasetValuesize() const override;

        // For warmup speedup
        virtual void quickDatasetGet(const Key& key, Value& value) const override;
        virtual void quickDatasetPut(const Key& key, const Value& value) override;
        virtual void quickDatasetDel(const Key& key) override;
    private:
        static const std::string kClassName;

        // Access by the single thread of client wrapper (NO need to be thread safe)
        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // Access by multiple client workers (thread safe)
        virtual WorkloadItem generateWorkloadItem_(const uint32_t& local_client_worker_idx) override; // NOTE: randomly select an item without modifying any variable -> thread safe

        // Utility functions for dynamic workload patterns
        virtual uint32_t getLargestRank_(const uint32_t local_client_worker_idx) const override;
        virtual void getRankedKeys_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) const override;
        virtual void getRandomKeys_(const uint32_t local_client_worker_idx, const uint32_t random_keycnt, std::vector<std::string>& random_keys) const override;

        // (1) Facebook-specific helper functions

        // For role of clients, dataset loader, and cloud
        std::unique_ptr<covered::GeneratorBase> makeGenerator_(const StressorConfig& config, const uint32_t& client_idx, const uint32_t& perclient_workercnt);

        // Common utilities
        void checkPointers_() const;

        // (2) Const shared variables
        std::string instance_name_;
        const float zipf_alpha_;

        // (3) Other shared variables
        // For clients, dataset loader, and cloud
        std::discrete_distribution<>* op_pool_dist_ptr_;
        //facebook::cachelib::cachebench::CacheConfig facebook_cache_config_;
        StressorConfig facebook_stressor_config_;
        std::unique_ptr<covered::GeneratorBase> workload_generator_; // Equivalent to all trace files with workload items, dataset items, and dataset statistics
        // For clients
        std::vector<std::mt19937_64*> client_worker_item_randgen_ptrs_;
        std::optional<uint64_t> last_reqid_; // NOT thread safe yet UNUSED in Facebook CDN workload
    };
}

#endif
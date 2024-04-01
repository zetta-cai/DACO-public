/*
 * FacebookWorkloadWrapper: encapsulate the config parser and workload generator (generate keycnt dataset key-value pairs with Util::DATASET_KVPAIR_GENERATION_SEED as the seed and opcnt/clientcnt workload requests/items with client_idx as the seed) in cachebench for Facebook/Meta CDN trace (https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_Overview/) (thread safe).
 * 
 * NOTE: CacheBench generates workload items by the following random seeds.
 * -> We use Util::DATASET_KVPAIR_GENERATION_SEED as deterministic seed to generate the same key-value pairs (i.e., dataset) for all clients.
 * -> We use client index as deterministic seed to generate different items (i.e., workloads) for all clients, yet with the same items for all workers of a specific client.
 * -> We use global worker index as deterministic seed to choose the same items for all running times of a specific worker.
 * 
 * NOTE: (i) as Facebook/Meta CDN provides workload generator without I/O overhead for loading trace files, NO need to distinguish loading and evaluation phases -> (ii) Instead, we can always generate dataset and use dataset key indices to reduce space cost of workload items.
 * 
 * NOTE: see more notes on source code of cachelib in docs/cachebench.md.
 * 
 * NOTE: FacebookWorkloadWrapper is for OSDI'20 Facebook CDN trace, while FbphotoWorkloadWrapper is for SOSP'13 Facebook photo caching trace
 * 
 * By Siyuan Sheng (2023.04.19).
 */

#ifndef FACEBOOK_WORKLOAD_WRAPPER_H
#define FACEBOOK_WORKLOAD_WRAPPER_H

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
    class FacebookWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        FacebookWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role);
        virtual ~FacebookWorkloadWrapper();

        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) override; // NOTE: randomly select an item without modifying any variable -> thread safe
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

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // Facebook-specific helper functions

        // (1) For role of clients, dataset loader, and cloud

        std::unique_ptr<covered::GeneratorBase> makeGenerator_(const StressorConfig& config, const uint32_t& client_idx);

        // (2) Common utilities

        void checkPointers_() const;

        // Const shared variables
        std::string instance_name_;

        // Const shared variables
        // (1) For clients, dataset loader, and cloud
        std::discrete_distribution<>* op_pool_dist_ptr_;
        //facebook::cachelib::cachebench::CacheConfig facebook_cache_config_;
        StressorConfig facebook_stressor_config_;
        std::unique_ptr<covered::GeneratorBase> workload_generator_; // Equivalent to all trace files with workload items, dataset items, and dataset statistics
        // (2) For clients
        std::vector<std::mt19937_64*> client_worker_item_randgen_ptrs_;
        std::optional<uint64_t> last_reqid_; // NOT thread safe yet UNUSED in Facebook CDN workload
    };
}

#endif
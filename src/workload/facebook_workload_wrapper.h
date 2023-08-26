/*
 * FacebookWorkloadWrapper: encapsulate the config parser and workload generator in cachebench for Facebook CDN trace (https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_Overview/) (thread safe).
 *
 * NOTE: see notes on source code of cachelib in docs/cachebench.md.
 * 
 * By Siyuan Sheng (2023.04.19).
 */

#ifndef FACEBOOK_WORKLOAD_WRAPPER_H
#define FACEBOOK_WORKLOAD_WRAPPER_H

#include <string>

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
        FacebookWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt);
        virtual ~FacebookWorkloadWrapper();

        virtual uint32_t getPracticalKeycnt() const;

        // Get average dataset key/value size
        virtual double getAvgDatasetKeysize() const override;
        virtual double getAvgDatasetValuesize() const override;
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // NOTE: randomly select an item without modifying any variable -> thread safe
        virtual WorkloadItem generateWorkloadItemInternal_(std::mt19937_64& request_randgen) override;

        // Get a dataset key-value pair item with the index of itemidx
        virtual WorkloadItem getDatasetItemInternal_(const uint32_t itemidx) override;

        std::unique_ptr<covered::GeneratorBase> makeGenerator_(const StressorConfig& config, const uint32_t& client_idx);

        // Const shared variables
        std::string instance_name_;
        std::discrete_distribution<>* op_pool_dist_ptr_;

        // Non-const shared variables
        std::optional<uint64_t> last_reqid_; // NOT thread safe yet UNUSED in Facebook CDN workload

        // Const shared variables
        //facebook::cachelib::cachebench::CacheConfig facebook_cache_config_;
        StressorConfig facebook_stressor_config_;
        std::unique_ptr<covered::GeneratorBase> workload_generator_;
    };
}

#endif
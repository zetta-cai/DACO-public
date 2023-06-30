/*
 * FacebookWorkloadWrapper: encapsulate the config parser and workload generator in cachebench for Facebook CDN trace (https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_Overview/).
 * 
 * By Siyuan Sheng (2023.04.19).
 */

#ifndef FACEBOOK_WORKLOAD_WRAPPER_H
#define FACEBOOK_WORKLOAD_WRAPPER_H

#include <string>

//#include <cachelib/cachebench/util/CacheConfig.h>
#include <cachelib/cachebench/workload/GeneratorBase.h>

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
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_(const uint32_t& client_idx) override;

        virtual WorkloadItem generateItemInternal_(std::mt19937_64& request_randgen) override;

        std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> makeGenerator_(const StressorConfig& config, const uint32_t& client_idx);

        // Come from Param
        const uint32_t clientcnt_;
        const uint32_t keycnt_;
        const uint32_t opcnt_;
        const uint32_t perclient_workercnt_;

        std::string instance_name_;

        std::discrete_distribution<>* op_pool_dist_ptr_;
        std::optional<uint64_t> last_reqid_;

        //facebook::cachelib::cachebench::CacheConfig facebook_cache_config_;
        StressorConfig facebook_stressor_config_;
        std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> workload_generator_;
    };
}

#endif
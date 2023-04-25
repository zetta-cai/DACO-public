/*
 * FacebookWorkload: encapsulate the config parser and workload generator in cachebench for Facebook CDN trace.
 * 
 * By Siyuan Sheng (2023.04.19).
 */

#ifndef FACEBOOK_WORKLOAD_H
#define FACEBOOK_WORKLOAD_H

#include <string>

//#include <cachelib/cachebench/util/CacheConfig.h>
#include <cachelib/cachebench/workload/GeneratorBase.h>

#include "common/request.h"
#include "workload/workload_base.h"
#include "workload/cachebench/cachebench_config.h"

namespace covered
{
    class FacebookWorkload : public WorkloadBase
    {
    public:
        FacebookWorkload();
        virtual ~FacebookWorkload();
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_(const uint32_t& global_client_idx) override;

        virtual Request generateReqInternal_(std::mt19937_64& request_randgen) override;

        std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> makeGenerator_(const StressorConfig& config, const uint32_t& global_client_idx);

        std::discrete_distribution<>* op_pool_dist_ptr_;
        std::optional<uint64_t> last_reqid_;

        //facebook::cachelib::cachebench::CacheConfig facebook_cache_config_;
        StressorConfig facebook_stressor_config_;
        std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> workload_generator_;
    };
}

#endif
#include "workload/facebook_workload_wrapper.h"

#include <memory> // std::make_unique
#include <random> // std::mt19937_64, std::discrete_distribution

#include <cachelib/cachebench/workload/KVReplayGenerator.h>
#include <cachelib/cachebench/workload/OnlineGenerator.h>
#include <cachelib/cachebench/workload/PieceWiseReplayGenerator.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "workload/cachebench/workload_generator.h"

namespace covered
{
    const std::string FacebookWorkloadWrapper::kClassName("FacebookWorkloadWrapper");

    // TODO: give random seeds for key-value pair generation (0) and request generation (global_client_idx)
    FacebookWorkloadWrapper::FacebookWorkloadWrapper() : WorkloadWrapperBase()
    {
        last_reqid_ = std::nullopt; // Not used by WorkloadGenerator for Facebook CDN trace
    }

    FacebookWorkloadWrapper::~FacebookWorkloadWrapper()
    {
        if (op_pool_dist_ptr_ != NULL)
        {
            delete op_pool_dist_ptr_;
            op_pool_dist_ptr_ = NULL;
        }
        workload_generator_->markFinish();
        workload_generator_->markShutdown();
    }

    void FacebookWorkloadWrapper::initWorkloadParameters_()
    {
        // Load workload config file for Facebook CDN trace
        CacheBenchConfig facebook_config(Config::getFacebookConfigFilepath());
        //facebook_cache_config_ = facebook_config.getCacheConfig();
        facebook_stressor_config_ = facebook_config.getStressorConfig();
        return;
    }

    void FacebookWorkloadWrapper::overwriteWorkloadParameters_()
    {
        uint32_t perworker_opcnt = Param::getOpcnt() / Param::getClientcnt() / Param::getPerclientWorkercnt();
        uint32_t perclient_workercnt = Param::getPerclientWorkercnt();
        uint32_t keycnt = Param::getKeycnt();

        facebook_stressor_config_.numOps = static_cast<uint64_t>(perworker_opcnt);
        facebook_stressor_config_.numThreads = static_cast<uint64_t>(perclient_workercnt);
        facebook_stressor_config_.numKeys = static_cast<uint64_t>(keycnt);

        op_pool_dist_ptr_ = new std::discrete_distribution<>(facebook_stressor_config_.opPoolDistribution.begin(), facebook_stressor_config_.opPoolDistribution.end());
        if (op_pool_dist_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "failed to create operation pool distribution!");
            exit(1);
        }
    }

    void FacebookWorkloadWrapper::createWorkloadGenerator_(const uint32_t& global_client_idx)
    {
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate opcnt requests by generateKeyDistributions() in constructor
        workload_generator_ = makeGenerator_(facebook_stressor_config_, global_client_idx);
    }

    Request FacebookWorkloadWrapper::generateReqInternal_(std::mt19937_64& request_randgen)
    {
        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        const uint8_t tmp_poolid = static_cast<uint8_t>((*op_pool_dist_ptr_)(request_randgen));

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_->getReq(tmp_poolid, request_randgen, last_reqid_));

        // Convert facebook::cachelib::cachebench::Request to covered::Request
        const Key tmp_covered_key(tmp_facebook_req.key);
        const Value tmp_covered_value(static_cast<uint32_t>(*(tmp_facebook_req.sizeBegin)));

        last_reqid_ = tmp_facebook_req.requestId;
        return Request(tmp_covered_key, tmp_covered_value);
    }

    // The same makeGenerator as in lib/CacheLib/cachelib/cachebench/runner/Stressor.cpp
    std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> FacebookWorkloadWrapper::makeGenerator_(const StressorConfig& config, const uint32_t& global_client_idx)
    {
        if (config.generator == "piecewise-replay") {
            Util::dumpErrorMsg(kClassName, "piecewise-replay generator is not supported now!");
            exit(1);
            // TODO: copy PieceWiseReplayGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::PieceWiseReplayGenerator>(config);
        } else if (config.generator == "replay") {
            Util::dumpErrorMsg(kClassName, "replay generator is not supported now!");
            exit(1);
            // TODO: copy KVReplayGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::KVReplayGenerator>(config);
        } else if (config.generator.empty() || config.generator == "workload") {
            // TODO: Remove the empty() check once we label workload-based configs
            // properly
            return std::make_unique<covered::WorkloadGenerator>(config, global_client_idx);
        } else if (config.generator == "online") {
            Util::dumpErrorMsg(kClassName, "online generator is not supported now!");
            exit(1);
            // TODO: copy OnlineGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::OnlineGenerator>(config);
        } else {
            throw std::invalid_argument("Invalid config");
        }
    }
}

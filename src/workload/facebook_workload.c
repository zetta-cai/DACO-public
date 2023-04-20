#include <memory> // std::make_unique
#include <random> // std::mt19937_64, std::discrete_distribution

#include <cachelib/cachebench/workload/PieceWiseReplayGenerator.h>
#include <cachelib/cachebench/workload/KVReplayGenerator.h>
#include <cachelib/cachebench/workload/WorkloadGenerator.h>
#include <cachelib/cachebench/workload/OnlineGenerator.h>

#include "common/util.h"
#include "workload/facebook_workload.h"

namespace covered
{
    const std::string FacebookWorkload::kClassName = "FacebookWorkload";

    // TODO: give random seeds for key-value pair generation (0) and request generation (global_client_idx)
    FacebookWorkload::FacebookWorkload()
    {
        // Create random generators
        req_randgen_ptr_ = new std::mt19937_64(folly::Random::rand64());
        if (req_randgen_ptr_ == NULL)
        {
            dumpErrorMsg(kClassName, "failed to create random generator for requests!");
            exit(1);
        }

        last_reqid_ = std::nullopt; // Not used by WorkloadGenerator for Facebook CDN trace
    }

    FacebookWorkload::~FacebookWorkload()
    {
        if (req_randgen_ptr_ != NULL)
        {
            delete req_randgen_ptr_;
        }
        if (op_pool_dist_ptr_ != NULL)
        {
            delete op_pool_dist_ptr_;
        }
    }

    void FacebookWorkload::initWorkloadParameters()
    {
        // Load workload config file for Facebook CDN trace
        facebook::cachelib::cachebench::CacheBenchConfig facebook_config(Config::getFacebookConfigFilepath());
        //facebook_cache_config_ = facebook_config.getCacheConfig();
        facebook_stressor_config_ = facebook_config.getStressorConfig();
        return;
    }

    void FacebookWorkload::overwriteWorkloadParameters()
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
            dumpErrorMsg(kClassName, "failed to create operation pool distribution!");
            exit(1);
        }
    }

    void FacebookWorkload::createWorkloadGenerator()
    {
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate opcnt requests by generateKeyDistributions() in constructor
        workload_generator_ = makeGenerator(facebook_stressor_config_);
    }

    Request FacebookWorkload::generateReqInternal()
    {
        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        const int8_t tmp_poolid = static_cast<int8_t>(op_pool_dist_ptr_->(*req_randgen_ptr_));

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_.getReq(tmp_poolid, *req_randgen_ptr_, last_reqid_));

        // Convert facebook::cachelib::cachebench::Request to covered::Request
        const Key tmp_covered_key(tmp_facebook_req.key);
        const Value tmp_covered_value(static_cast<uint32_t>(*(req.sizeBegin)));

        last_reqid_ = tmp_facebook_req.requestId;
        return Request(tmp_covered_key, tmp_covered_value);
    }

    // The same makeGenerator as in lib/CacheLib/cachelib/cachebench/runner/Stressor.cpp
    namespace {
        std::unique_ptr<facebook::cachelib::cachebench::GeneratorBase> makeGenerator(const facebook::cachelib::cachebench::StressorConfig& config) {
            if (config.generator == "piecewise-replay") {
                return std::make_unique<facebook::cachelib::cachebench::PieceWiseReplayGenerator>(config);
            } else if (config.generator == "replay") {
                return std::make_unique<facebook::cachelib::cachebench::KVReplayGenerator>(config);
            } else if (config.generator.empty() || config.generator == "workload") {
                // TODO: Remove the empty() check once we label workload-based configs
                // properly
                return std::make_unique<facebook::cachelib::cachebench::WorkloadGenerator>(config);
            } else if (config.generator == "online") {
                return std::make_unique<facebook::cachelib::cachebench::OnlineGenerator>(config);

            } else {
                throw std::invalid_argument("Invalid config");
            }
        }
    } // Anonymous namespace (cannot be accessed outside facebook_workload.c)
}

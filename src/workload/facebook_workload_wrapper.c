#include "workload/facebook_workload_wrapper.h"

#include <memory> // std::make_unique
#include <random> // std::mt19937_64, std::discrete_distribution

#include <cachelib/cachebench/workload/KVReplayGenerator.h>
#include <cachelib/cachebench/workload/OnlineGenerator.h>
#include <cachelib/cachebench/workload/PieceWiseReplayGenerator.h>

#include "common/config.h"
#include "common/util.h"
#include "workload/cachebench/workload_generator.h"

namespace covered
{
    const std::string FacebookWorkloadWrapper::kClassName("FacebookWorkloadWrapper");

    FacebookWorkloadWrapper::FacebookWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, opcnt, perclient_workercnt)
    {
        // Differentiate facebook workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

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

    uint32_t FacebookWorkloadWrapper::getPracticalKeycnt() const
    {
        // NOTE: CacheLib CDN generator will remove redundant keys, so the number of generated key-value pairs will be slightly smaller than keycnt -> we do NOT fix CacheLib as the keycnt gap is very limited and we aim to avoid changing its workload distribution.
        return static_cast<uint32_t>(workload_generator_->getAllKeys().size());
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
        assert(clientcnt_ > 0);
        assert(perclient_workercnt_ > 0);
        uint32_t perclientworker_opcnt = opcnt_ / clientcnt_ / perclient_workercnt_;

        facebook_stressor_config_.numOps = static_cast<uint64_t>(perclientworker_opcnt);
        facebook_stressor_config_.numThreads = static_cast<uint64_t>(perclient_workercnt_);
        facebook_stressor_config_.numKeys = static_cast<uint64_t>(keycnt_);

        // NOTE: opPoolDistribution is {1.0}, which generates 0 with a probability of 1.0
        op_pool_dist_ptr_ = new std::discrete_distribution<>(facebook_stressor_config_.opPoolDistribution.begin(), facebook_stressor_config_.opPoolDistribution.end());
        if (op_pool_dist_ptr_ == NULL)
        {
            Util::dumpErrorMsg(instance_name_, "failed to create operation pool distribution!");
            exit(1);
        }
    }

    void FacebookWorkloadWrapper::createWorkloadGenerator_()
    {
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate opcnt requests by generateKeyDistributions() in constructor
        workload_generator_ = makeGenerator_(facebook_stressor_config_, client_idx_);
    }

    WorkloadItem FacebookWorkloadWrapper::generateWorkloadItemInternal_(std::mt19937_64& request_randgen)
    {
        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        const uint8_t tmp_poolid = static_cast<uint8_t>((*op_pool_dist_ptr_)(request_randgen));

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_->getReq(tmp_poolid, request_randgen, last_reqid_));

        // Convert facebook::cachelib::cachebench::Request to covered::Request
        const Key tmp_covered_key(tmp_facebook_req.key);
        const Value tmp_covered_value(static_cast<uint32_t>(*(tmp_facebook_req.sizeBegin)));
        WorkloadItemType tmp_item_type;
        facebook::cachelib::cachebench::OpType tmp_op_type = tmp_facebook_req.getOp();
        switch (tmp_op_type)
        {
            case facebook::cachelib::cachebench::OpType::kGet:
            case facebook::cachelib::cachebench::OpType::kLoneGet:
            {
                tmp_item_type = WorkloadItemType::kWorkloadItemGet;
                break;
            }
            case facebook::cachelib::cachebench::OpType::kSet:
            case facebook::cachelib::cachebench::OpType::kLoneSet:
            case facebook::cachelib::cachebench::OpType::kUpdate:
            {
                tmp_item_type = WorkloadItemType::kWorkloadItemPut;
                break;
            }
            case facebook::cachelib::cachebench::OpType::kDel:
            {
                tmp_item_type = WorkloadItemType::kWorkloadItemDel;
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "facebook::cachelib::cachebench::OpType " << static_cast<uint32_t>(tmp_op_type) << " is not supported now (please refer to lib/CacheLib/cachelib/cachebench/util/Request.h for OpType)!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        last_reqid_ = tmp_facebook_req.requestId;
        return WorkloadItem(tmp_covered_key, tmp_covered_value, tmp_item_type);
    }

    WorkloadItem FacebookWorkloadWrapper::getDatasetItemInternal_(const uint32_t itemidx)
    {
        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        assert(facebook_stressor_config_.opPoolDistribution.size() == 1 && facebook_stressor_config_.opPoolDistribution[0] == double(1.0));
        const uint8_t tmp_poolid = 0;

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_->getReq(tmp_poolid, itemidx));

        const Key tmp_covered_key(tmp_facebook_req.key);
        const Value tmp_covered_value(static_cast<uint32_t>(*(tmp_facebook_req.sizeBegin)));
        return WorkloadItem(tmp_covered_key, tmp_covered_value, WorkloadItemType::kWorkloadItemPut);
    }

    // The same makeGenerator as in lib/CacheLib/cachelib/cachebench/runner/Stressor.cpp
    std::unique_ptr<covered::GeneratorBase> FacebookWorkloadWrapper::makeGenerator_(const StressorConfig& config, const uint32_t& client_idx)
    {
        if (config.generator == "piecewise-replay") {
            Util::dumpErrorMsg(instance_name_, "piecewise-replay generator is not supported now!");
            exit(1);
            // TODO: copy PieceWiseReplayGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::PieceWiseReplayGenerator>(config);
        } else if (config.generator == "replay") {
            Util::dumpErrorMsg(instance_name_, "replay generator is not supported now!");
            exit(1);
            // TODO: copy KVReplayGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::KVReplayGenerator>(config);
        } else if (config.generator.empty() || config.generator == "workload") {
            // TODO: Remove the empty() check once we label workload-based configs
            // properly
            return std::make_unique<covered::WorkloadGenerator>(config, client_idx);
        } else if (config.generator == "online") {
            Util::dumpErrorMsg(instance_name_, "online generator is not supported now!");
            exit(1);
            // TODO: copy OnlineGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::OnlineGenerator>(config);
        } else {
            throw std::invalid_argument("Invalid config");
        }
    }
}

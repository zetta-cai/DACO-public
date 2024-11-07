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

    FacebookWorkloadWrapper::FacebookWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase)
    {
        // NOTE: Facebook CDN is not replayed trace and NO need for trace preprocessing (also NO need to dump dataset file)
        assert(!needAllTraceFiles_()); // Must NOT trace preprocessor

        // Differentiate facebook workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();
        
        // For clients, dataset loader, and cloud
        op_pool_dist_ptr_ = NULL;
        workload_generator_ = nullptr;

        // For clients
        client_worker_item_randgen_ptrs_.resize(perclient_workercnt, NULL);
        last_reqid_ = std::nullopt; // Not used by WorkloadGenerator for Facebook CDN trace
    }

    FacebookWorkloadWrapper::~FacebookWorkloadWrapper()
    {
        // For clients, dataset loader, and cloud
        if (op_pool_dist_ptr_ != NULL)
        {
            delete op_pool_dist_ptr_;
            op_pool_dist_ptr_ = NULL;
        }

        assert(workload_generator_ != nullptr);
        workload_generator_->markFinish();
        workload_generator_->markShutdown();

        if (needWorkloadItems_()) // Clients
        {
            // Release random generator for each client worker
            for (uint32_t i = 0; i < client_worker_item_randgen_ptrs_.size(); i++)
            {
                assert(client_worker_item_randgen_ptrs_[i] != NULL);
                delete client_worker_item_randgen_ptrs_[i];
                client_worker_item_randgen_ptrs_[i] = NULL;
            }
        }
    }

    uint32_t FacebookWorkloadWrapper::getPracticalKeycnt() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        // NOTE: CacheLib CDN generator will remove redundant keys, so the number of generated key-value pairs will be slightly smaller than keycnt -> we do NOT fix CacheLib as the keycnt gap is very limited and we aim to avoid changing its workload distribution.
        int64_t dataset_size = workload_generator_->getAllKeys().size();
        return Util::toUint32(dataset_size);
    }

    WorkloadItem FacebookWorkloadWrapper::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud
        assert(itemidx < getPracticalKeycnt());
        
        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        assert(facebook_stressor_config_.opPoolDistribution.size() == 1 && facebook_stressor_config_.opPoolDistribution[0] == double(1.0));
        // const uint8_t tmp_poolid = 0;

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_->getDatasetReq(itemidx));

        const Key tmp_covered_key(tmp_facebook_req.key);
        const Value tmp_covered_value(static_cast<uint32_t>(*(tmp_facebook_req.sizeBegin)));

        return WorkloadItem(tmp_covered_key, tmp_covered_value, WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double FacebookWorkloadWrapper::getAvgDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getAvgDatasetKeysize();
    }
    
    double FacebookWorkloadWrapper::getAvgDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getAvgDatasetValuesize();
    }

    uint32_t FacebookWorkloadWrapper::getMinDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getMinDatasetKeysize();
    }

    uint32_t FacebookWorkloadWrapper::getMinDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getMinDatasetValuesize();
    }

    uint32_t FacebookWorkloadWrapper::getMaxDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getMaxDatasetKeysize();
    }

    uint32_t FacebookWorkloadWrapper::getMaxDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return workload_generator_->getMaxDatasetValuesize();
    }

    // For warmup speedup

    void FacebookWorkloadWrapper::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        uint32_t value_size = 0;
        workload_generator_->quickDatasetGet(key.getKeystr(), value_size);
        value = Value(value_size);

        return;
    }

    void FacebookWorkloadWrapper::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        workload_generator_->quickDatasetPut(key.getKeystr(), value.getValuesize());

        return;
    }

    void FacebookWorkloadWrapper::quickDatasetDel(const Key& key)
    {
        quickDatasetPut(key, Value()); // Use default value with is_deleted = true and value size = 0 as delete operation

        return;
    }

    void FacebookWorkloadWrapper::initWorkloadParameters_()
    {
        // Load workload config file for Facebook CDN trace
        CacheBenchConfig facebook_config(Config::getFacebookConfigFilepath());
        //facebook_cache_config_ = facebook_config.getCacheConfig();
        facebook_stressor_config_ = facebook_config.getStressorConfig();

        if (needWorkloadItems_()) // Clients
        {
            for (uint32_t tmp_local_client_worker_idx = 0; tmp_local_client_worker_idx < getPerclientWorkercnt_(); tmp_local_client_worker_idx++)
            {
                uint32_t tmp_global_client_worker_idx = Util::getGlobalClientWorkerIdx(getClientIdx_(), tmp_local_client_worker_idx, getPerclientWorkercnt_());

                // Each per-client worker uses worker_idx as deterministic seed to create a random generator and get different requests
                // NOTE: we use global_client_worker_idx to randomly generate requests from workload items
                std::mt19937_64* tmp_client_worker_item_randgen_ptr_ = new std::mt19937_64(tmp_global_client_worker_idx + getWorkloadRandombase_());
                // (OBSOLETE: homogeneous cache access patterns is a WRONG assumption -> we should ONLY follow homogeneous workload distribution yet still with heterogeneous cache access patterns) NOTE: we use WORKLOAD_KVPAIR_GENERATION_SEED to generate requests with homogeneous cache access patterns
                //std::mt19937_64* tmp_client_worker_item_randgen_ptr_ = new std::mt19937_64(Util::WORKLOAD_KVPAIR_GENERATION_SEED + getWorkloadRandombase_());
                if (tmp_client_worker_item_randgen_ptr_ == NULL)
                {
                    Util::dumpErrorMsg(instance_name_, "failed to create a random generator for requests!");
                    exit(1);
                }

                client_worker_item_randgen_ptrs_[tmp_local_client_worker_idx] = tmp_client_worker_item_randgen_ptr_;
            }
        }

        return;
    }

    void FacebookWorkloadWrapper::overwriteWorkloadParameters_()
    {
        uint32_t tmp_perclientworker_opcnt = 0;
        uint32_t tmp_perclient_workercnt = 0;
        if (needWorkloadItems_())
        {
            tmp_perclient_workercnt = getPerclientWorkercnt_();
            tmp_perclientworker_opcnt = getPerclientOpcnt_() / tmp_perclient_workercnt;
        }

        facebook_stressor_config_.numOps = static_cast<uint64_t>(tmp_perclientworker_opcnt);
        facebook_stressor_config_.numThreads = static_cast<uint64_t>(tmp_perclient_workercnt);
        facebook_stressor_config_.numKeys = static_cast<uint64_t>(getKeycnt_());

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
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate perclient_opcnt_ requests by generateKeyDistributions() in constructor
        uint32_t tmp_client_idx = 0;
        if (needWorkloadItems_()) // Clients
        {
            tmp_client_idx = getClientIdx_(); // Use client idx as random seed to generate workload items for the current client
        }
        else // Dataset loader and cloud
        {
            tmp_client_idx = 0; // NOTE: ONLY need dataset items, yet NOT use workload items -> client idx makes no sense
        }
        workload_generator_ = makeGenerator_(facebook_stressor_config_, tmp_client_idx, getPerclientWorkercnt_());
    }

    // Access by multiple client workers (thread safe)

    WorkloadItem FacebookWorkloadWrapper::generateWorkloadItem_(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needWorkloadItems_()); // Must be clients for evaluation
        assert(local_client_worker_idx < client_worker_item_randgen_ptrs_.size());

        std::mt19937_64* request_randgen_ptr = client_worker_item_randgen_ptrs_[local_client_worker_idx];
        assert(request_randgen_ptr != NULL);

        // Must be 0 for Facebook CDN trace due to only a single operation pool (cachelib::PoolId = int8_t)
        const uint8_t tmp_poolid = static_cast<uint8_t>((*op_pool_dist_ptr_)(*request_randgen_ptr));

        const facebook::cachelib::cachebench::Request& tmp_facebook_req(workload_generator_->getReq(local_client_worker_idx, tmp_poolid, *request_randgen_ptr, last_reqid_));

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

    // Utility functions for dynamic workload patterns

    uint32_t FacebookWorkloadWrapper::getLargestRank_(const uint32_t local_client_worker_idx) const
    {
        checkDynamicPatterns_();

        const std::vector<uint32_t>& tmp_ranked_unique_key_indices_const_ref = workload_generator_->getRankedUniqueKeyIndicesConstRef(local_client_worker_idx, 0); // poolId MUST be 0
        return tmp_ranked_unique_key_indices_const_ref.size() - 1; // poolId MUST be 0
    }

    void FacebookWorkloadWrapper::getRankedKeys_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) const
    {
        checkDynamicPatterns_();

        // Get ranked indexes
        std::vector<uint32_t> tmp_ranked_idxes;
        getRankedIdxes_(local_client_worker_idx, start_rank, ranked_keycnt, tmp_ranked_idxes);

        // Get the const reference of current client worker's ranked key indices
        const std::vector<uint32_t>& tmp_ranked_unique_key_indices_const_ref = workload_generator_->getRankedUniqueKeyIndicesConstRef(local_client_worker_idx, 0); // poolId MUST be 0

        // Set ranked keys based on the ranked indexes
        ranked_keys.clear();
        for (int i = 0; i < tmp_ranked_idxes.size(); i++)
        {
            const uint32_t tmp_ranked_key_indices_idx = tmp_ranked_idxes[i];
            const uint32_t tmp_ranked_key_indice = tmp_ranked_unique_key_indices_const_ref[tmp_ranked_key_indices_idx];
            ranked_keys.push_back(workload_generator_->getDatasetReq(tmp_ranked_key_indice).key);
        }

        return;
    }

    // (1) For the role of clients, dataset loader, and cloud

    // The same makeGenerator as in lib/CacheLib/cachelib/cachebench/runner/Stressor.cpp
    std::unique_ptr<covered::GeneratorBase> FacebookWorkloadWrapper::makeGenerator_(const StressorConfig& config, const uint32_t& client_idx, const uint32_t& perclient_workercnt)
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
            return std::make_unique<covered::WorkloadGenerator>(config, client_idx, perclient_workercnt);
        } else if (config.generator == "online") {
            Util::dumpErrorMsg(instance_name_, "online generator is not supported now!");
            exit(1);
            // TODO: copy OnlineGenerator into namespace covered to support covered::StressorConfig
            //return std::make_unique<facebook::cachelib::cachebench::OnlineGenerator>(config);
        } else {
            throw std::invalid_argument("Invalid config");
        }
    }

    // (2) Common utilities

    void FacebookWorkloadWrapper::checkPointers_() const
    {
        assert(op_pool_dist_ptr_ != NULL);
        assert(workload_generator_ != nullptr);  

        if (needWorkloadItems_()) // Clients
        {
            assert(client_worker_item_randgen_ptrs_.size() > 0);
            for (uint32_t i = 0; i < client_worker_item_randgen_ptrs_.size(); i++)
            {
                assert(client_worker_item_randgen_ptrs_[i] != NULL);
            }
        }
    }
}

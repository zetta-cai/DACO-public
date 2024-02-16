#include "workload/wikipedia_workload_wrapper.h"

#include <unistd.h> // lseek
#include <unordered_set>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string WikipediaWorkloadWrapper::kClassName("WikipediaWorkloadWrapper");

    WikipediaWorkloadWrapper::WikipediaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const WikipediaWorkloadExtraParam& workload_extra_param) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt), workload_extra_param_(workload_extra_param)
    {
        // Differentiate facebook workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        dataset_kvpairs_.clear();
        workload_key_indices_.clear();
        workload_value_sizes_.clear();
    }

    WikipediaWorkloadWrapper::~WikipediaWorkloadWrapper()
    {
    }

    uint32_t WikipediaWorkloadWrapper::getPracticalKeycnt() const
    {
        return dataset_kvpairs_.size();
    }

    void WikipediaWorkloadWrapper::initWorkloadParameters_()
    {
        const std::string wiki_trace_type = workload_extra_param_.getWikiTraceType();

        uint32_t column_cnt = 0;
        uint32_t key_column_idx = 0;
        uint32_t value_column_idx = 0;
        std::vector<std::string> trace_filepaths; // NOTE: follow the trace order
        if (wiki_trace_type == WikipediaWorkloadExtraParam::WIKI_TRACE_IMAGE_TYPE)
        {
            column_cnt = 5;
            key_column_idx = 1; // 2nd column
            value_column_idx = 3; // 4th column

            // TODO: Update by Config module
        }
        else if (wiki_trace_type == WikipediaWorkloadExtraParam::WIKI_TRACE_TEXT_TYPE)
        {
            column_cnt = 4;
            key_column_idx = 1; // 2nd column
            value_column_idx = 2; // 3rd column

            // TODO: Update by Config module
        }
        else
        {
            std::ostringstream oss;
            oss << "Wikipedia trace type " << wiki_trace_type << " is not supported now!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        const uint32_t trace_filecnt = trace_filepaths.size();
        assert(trace_filecnt > 0);

        std::ostringstream oss;
        oss << "load Wikipedia trace " << wiki_trace_type << " files...";
        Util::dumpNormalMsg(instance_name_, oss.str());
        
        std::unordered_set<Key> dataset_keyset; // Check if key has been tracked by dataset_kvpairs_
        for (uint32_t tmp_fileidx = 0; tmp_fileidx < trace_filecnt; tmp_fileidx++)
        {
            const std::string tmp_filepath = trace_filepaths[tmp_fileidx];

            // TODO: Encapsulate the following code as an individual function

            // Check if file exists
            bool is_exist = Util::isFileExist(tmp_filepath, true);
            if (!is_exist)
            {
                std::ostringstream oss;
                oss << "Wikipedia trace file " << tmp_filepath << " does not exist!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Get file length
            int tmp_fd = Util::openFile(tmp_filepath, O_RDONLY);
            int tmp_filelen = lseek(tmp_fd, 0, SEEK_END);
            assert(tmp_filelen > 0);

            // TODO: Build memory mapping
        }

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
        uint32_t perclientworker_opcnt = perclient_opcnt_ / perclient_workercnt_;

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
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate perclient_opcnt_ requests by generateKeyDistributions() in constructor
        workload_generator_ = makeGenerator_(facebook_stressor_config_, client_idx_);
    }

    WorkloadItem FacebookWorkloadWrapper::generateWorkloadItemInternal_(const uint32_t& local_client_worker_idx)
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

    // Get average/min/max dataset key/value size

    double FacebookWorkloadWrapper::getAvgDatasetKeysize() const
    {
        return workload_generator_->getAvgDatasetKeysize();
    }
    
    double FacebookWorkloadWrapper::getAvgDatasetValuesize() const
    {
        return workload_generator_->getAvgDatasetValuesize();
    }

    uint32_t FacebookWorkloadWrapper::getMinDatasetKeysize() const
    {
        return workload_generator_->getMinDatasetKeysize();
    }

    uint32_t FacebookWorkloadWrapper::getMinDatasetValuesize() const
    {
        return workload_generator_->getMinDatasetValuesize();
    }

    uint32_t FacebookWorkloadWrapper::getMaxDatasetKeysize() const
    {
        return workload_generator_->getMaxDatasetKeysize();
    }

    uint32_t FacebookWorkloadWrapper::getMaxDatasetValuesize() const
    {
        return workload_generator_->getMaxDatasetValuesize();
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

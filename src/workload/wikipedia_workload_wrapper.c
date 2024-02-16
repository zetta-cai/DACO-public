#include "workload/wikipedia_workload_wrapper.h"

#include <stdlib.h> // strtoll
#include <string.h> // strchr
#include <sys/mman.h> // mmap and munmap
#include <unistd.h> // lseek
#include <unordered_map>

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
        
        std::unordered_map<Key, Value, KeyHasher> dataset_kvmap; // Check if key has been tracked by dataset_kvpairs_ and compare with the original value size
        for (uint32_t tmp_fileidx = 0; tmp_fileidx < trace_filecnt; tmp_fileidx++) // For each trace file
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
            int64_t tmp_filelen = lseek(tmp_fd, 0, SEEK_END);
            assert(tmp_filelen > 0);

            // Process the current trace file
            uint32_t mmap_block_cnt = (tmp_filelen - 1) / Util::MAX_MMAP_BLOCK_SIZE + 1;
            assert(mmap_block_cnt > 0);
            char* prev_block_taildata = NULL;
            int64_t prev_block_tailsize = 0;
            for (uint32_t tmp_mmap_block_idx = 0; tmp_mmap_block_idx < mmap_block_cnt; tmp_mmap_block_idx++) // For each mmap block in the current trace file
            {
                // Calculate current mmap block size
                int64_t inclusive_start_fileoff = tmp_mmap_block_idx * Util::MAX_MMAP_BLOCK_SIZE;
                int64_t exclusive_end_fileoff = (tmp_mmap_block_idx == mmap_block_cnt - 1) ? tmp_filelen : (tmp_mmap_block_idx + 1) * Util::MAX_MMAP_BLOCK_SIZE;
                int64_t tmp_mmap_block_size = exclusive_end_fileoff - inclusive_start_fileoff;

                // Perform memory mapping
                char* tmp_block_buffer = (char*) mmap(NULL, tmp_mmap_block_size, PROT_READ, MAP_PRIVATE, tmp_fd, inclusive_start_fileoff);
                assert(tmp_block_buffer != NULL);

                // Parse the current mmap block in TSV format
                char* tmp_line_startpos = tmp_block_buffer;
                char* tmp_block_buffer_endpos = tmp_block_buffer + tmp_mmap_block_size - 1;
                while (tmp_line_startpos <= tmp_block_buffer_endpos) // For lines in the current mmap block of the current trace file
                {
                    // Find the end of the current line
                    char* tmp_line_endpos = strchr(tmp_line_startpos, Util::LINE_SEP_CHAR);
                    if (tmp_line_endpos == NULL) // Remaining tail data is NOT a complete line
                    {
                        // Save the tail data
                        assert(tmp_line_startpos <= tmp_block_buffer_endpos);
                        prev_block_tailsize = tmp_block_buffer_endpos - tmp_line_startpos + 1;
                        prev_block_taildata = (char*) malloc(prev_block_tailsize);
                        assert(prev_block_taildata != NULL);
                        memcpy(prev_block_taildata, tmp_line_startpos, prev_block_tailsize);

                        break; // Switch to the next mmap block of the current trace file
                    }

                    // Concatenate the tail data of the previous mmap block if any
                    assert(tmp_line_startpos != NULL);
                    char* tmp_concat_line_startpos = tmp_line_startpos;
                    char* tmp_concat_line_endpos = tmp_line_endpos;
                    if (prev_block_tailsize > 0)
                    {
                        assert(prev_block_taildata != NULL);

                        uint32_t tmp_concat_line_size = prev_block_tailsize + tmp_line_endpos - tmp_line_startpos + 1;
                        tmp_concat_line_startpos = (char*) malloc(tmp_concat_line_size);
                        assert(tmp_concat_line_startpos != NULL);
                        memcpy(tmp_concat_line_startpos, prev_block_taildata, prev_block_tailsize); // Tail data of prev mmap block
                        memcpy(tmp_concat_line_startpos + prev_block_tailsize, tmp_line_startpos, tmp_line_endpos - tmp_line_startpos + 1); // Current line of current mmap block
                        tmp_concat_line_endpos = tmp_concat_line_startpos + tmp_concat_line_size - 1;
                    }

                    // Process the current line
                    char* tmp_column_startpos = tmp_concat_line_startpos;
                    Key tmp_key;
                    Value tmp_value;
                    for (uint32_t tmp_column_idx = 0; tmp_column_idx < column_cnt; tmp_column_idx++) // For each column in the current line of current mmap block of the current trace file
                    {
                        assert(tmp_column_startpos <= tmp_concat_line_endpos);

                        // Find the end of the current column
                        char* tmp_column_endpos = tmp_concat_line_endpos;
                        if (tmp_column_idx != column_cnt - 1)
                        {
                            tmp_column_endpos = strchr(tmp_column_startpos, Util::TSV_SEP_CHAR);
                            assert(tmp_column_endpos != NULL);
                        }

                        assert(tmp_column_endpos > tmp_column_startpos); // Column MUST NOT empty except the column separator
                        if (tmp_column_idx == key_column_idx)
                        {
                            char* tmp_key_endptr = NULL;
                            int64_t tmp_keyint = strtoll(tmp_column_startpos, &tmp_key_endptr, 10);
                            assert(tmp_key_endptr == tmp_column_endpos); // The first invalid char MUST be the column separator
                            tmp_key = Key(std::string((const char*)&tmp_keyint, sizeof(int64_t)));
                        }
                        else if (tmp_column_idx == value_column_idx)
                        {
                            char* tmp_value_endptr = NULL;
                            int tmp_valuesize = strtol(tmp_column_startpos, &tmp_value_endptr, 10);
                            assert(tmp_value_endptr == tmp_column_endpos); // The first invalid char MUST be the column separator
                            tmp_value = Value(tmp_valuesize);
                        }
                        else
                        {
                            // Do nothing
                        }

                        // Switch to the next column
                        tmp_column_startpos = tmp_column_endpos + 1;
                    } // End of columns in the current line of the current mmap block of the current trace file

                    // Update dataset and workload if necessary
                    std::unordered_map<Key, Value, KeyHasher>::iterator tmp_dataset_kvmap_iter = dataset_kvmap.find(tmp_key);
                    if (tmp_dataset_kvmap_iter == dataset_kvmap.end()) // The first request on the key
                    {
                        dataset_kvmap.insert(std::pair(tmp_key, tmp_value));

                        dataset_kvpairs_.push_back(std::pair(tmp_key, tmp_value));
                        workload_key_indices_.push_back(dataset_kvpairs_.size() - 1);
                        workload_value_sizes_.push_back(-1); // Treat the first request as read request, as stresstest phase is after dataset loading phase
                    }
                    else // Subsequent requests on the key
                    {
                        if (tmp_value.getValuesize() == tmp_dataset_kvmap_iter->second.getValuesize())
                        {
                            workload_value_sizes_.push_back(-1); // Read request
                        }
                        else
                        {
                            workload_value_sizes_.push_back(tmp_value.getValuesize()); // Write request (put or delete)
                        }
                    }

                    // Release concatenated line if necessary
                    if (prev_block_tailsize > 0)
                    {
                        // Release tail data allocated for prev mmap block
                        assert(prev_block_taildata != NULL);
                        free(prev_block_taildata);
                        prev_block_taildata = NULL;
                        prev_block_tailsize = 0;

                        // Release concatenated line allocated for current mmap block
                        assert(tmp_concat_line_startpos != NULL);
                        free(tmp_concat_line_startpos);
                        tmp_concat_line_startpos = NULL;
                    }

                    // Switch to the next line
                    tmp_line_startpos = tmp_line_startpos + 1;
                } // End of lines of the current mmap block in the current trace file

                // Release memory mapping
                munmap(tmp_block_buffer, tmp_mmap_block_size);

                // NOTE: the last mmap block of the current trace file MUST NOT have tail data
                if (tmp_mmap_block_idx == mmap_block_cnt - 1)
                {
                    assert(prev_block_tailsize == 0);
                    assert(prev_block_taildata == NULL);
                }
            } // End of mmap blocks of the current trace file

            // Close file
            Util::closeFile(tmp_fd);
        } // End of trace files

        return;
    }

    // TODO: END HERE
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

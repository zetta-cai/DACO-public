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

        average_dataset_keysize_ = 0;
        average_dataset_valuesize_ = 0;
        min_dataset_keysize_ = 0;
        min_dataset_valuesize_ = 0;
        max_dataset_keysize_ = 0;
        max_dataset_valuesize_ = 0;

        dataset_kvpairs_.clear();
        workload_key_indices_.clear();
        workload_value_sizes_.clear();

        per_client_worker_workload_idx_.resize(perclient_workercnt);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            per_client_worker_workload_idx_[i] = i;
        }
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

            // Process the current trace file
            parseCurrentFile_(tmp_filepath, key_column_idx, value_column_idx, column_cnt, dataset_kvmap);
        } // End of trace files

        return;
    }

    void WikipediaWorkloadWrapper::overwriteWorkloadParameters_()
    {
        // NOTE: nothing to overwrite for Wikipedia workload
        return;
    }

    void WikipediaWorkloadWrapper::createWorkloadGenerator_()
    {
        // NOTE: nothing to create for Wikipedia workload
        return;
    }

    WorkloadItem WikipediaWorkloadWrapper::generateWorkloadItemInternal_(const uint32_t& local_client_worker_idx)
    {
        uint32_t current_workload_idx = per_client_worker_workload_idx_[local_client_worker_idx];
        assert(current_workload_idx < workload_key_indices_.size());

        // Get key, value, and type
        Key tmp_key = dataset_kvpairs_[workload_key_indices_[current_workload_idx]].first;
        Value tmp_value = dataset_kvpairs_[workload_key_indices_[current_workload_idx]].second;
        WorkloadItemType tmp_type = WorkloadItemType::kWorkloadItemGet;
        int tmp_workload_valuesize = workload_value_sizes_[current_workload_idx];
        if (tmp_workload_valuesize == 0)
        {
            tmp_type = WorkloadItemType::kWorkloadItemDel;
            tmp_value = Value();
        }
        else if (tmp_workload_valuesize > 0)
        {
            tmp_type = WorkloadItemType::kWorkloadItemPut;
            tmp_value = Value(tmp_workload_valuesize);
        }

        // Update for the next workload idx
        uint32_t next_workload_idx = (current_workload_idx + perclient_workercnt_) % workload_key_indices_.size();
        per_client_worker_workload_idx_[local_client_worker_idx] = next_workload_idx;

        return WorkloadItem(tmp_key, tmp_value, tmp_type);
    }

    WorkloadItem WikipediaWorkloadWrapper::getDatasetItemInternal_(const uint32_t itemidx)
    {
        assert(itemidx < dataset_kvpairs_.size());

        const Key tmp_covered_key = dataset_kvpairs_[itemidx].first;
        const Value tmp_covered_value = dataset_kvpairs_[itemidx].second;
        return WorkloadItem(tmp_covered_key, tmp_covered_value, WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double WikipediaWorkloadWrapper::getAvgDatasetKeysize() const
    {
        return average_dataset_keysize_;
    }
    
    double WikipediaWorkloadWrapper::getAvgDatasetValuesize() const
    {
        return average_dataset_valuesize_;
    }

    uint32_t WikipediaWorkloadWrapper::getMinDatasetKeysize() const
    {
        return min_dataset_keysize_;
    }

    uint32_t WikipediaWorkloadWrapper::getMinDatasetValuesize() const
    {
        return min_dataset_valuesize_;
    }

    uint32_t WikipediaWorkloadWrapper::getMaxDatasetKeysize() const
    {
        return max_dataset_keysize_;
    }

    uint32_t WikipediaWorkloadWrapper::getMaxDatasetValuesize() const
    {
        return max_dataset_valuesize_;
    }

    // Wiki-specific helper functions

    void WikipediaWorkloadWrapper::parseCurrentFile_(const std::string& tmp_filepath, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, std::unordered_map<Key, Value, KeyHasher>& dataset_kvmap)
    {
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

        // Process mmap blocks of the current trace file
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
                bool is_achieve_trace_file_end = false;
                char* tmp_line_endpos = strchr(tmp_line_startpos, Util::LINE_SEP_CHAR);
                if (tmp_line_endpos == NULL) // Remaining tail data is NOT a complete line
                {
                    if (tmp_mmap_block_idx == mmap_block_cnt - 1) // The last mmap block
                    {
                        is_achieve_trace_file_end = true;
                    }
                    else // Intermediate mmap blocks
                    {
                        // Save the tail data
                        assert(tmp_line_startpos <= tmp_block_buffer_endpos);
                        prev_block_tailsize = tmp_block_buffer_endpos - tmp_line_startpos + 1;
                        prev_block_taildata = (char*) malloc(prev_block_tailsize);
                        assert(prev_block_taildata != NULL);
                        memcpy(prev_block_taildata, tmp_line_startpos, prev_block_tailsize);
                    }

                    break; // Switch to the next mmap block of the current trace file
                }

                // Complete the last line of the last mmap block if necessary for strtol/strtoll
                assert(tmp_line_startpos != NULL);
                char* tmp_complete_line_startpos = tmp_line_startpos;
                char* tmp_complete_line_endpos = tmp_line_endpos;
                if (is_achieve_trace_file_end)
                {
                    completeLastLine_(tmp_line_startpos, tmp_line_endpos, &tmp_complete_line_startpos, &tmp_complete_line_endpos);
                }

                // Concatenate the tail data of the previous mmap block if any
                char* tmp_concat_line_startpos = tmp_complete_line_startpos;
                char* tmp_concat_line_endpos = tmp_complete_line_endpos;
                if (prev_block_tailsize > 0)
                {
                    concatenateLastLine_(prev_block_taildata, prev_block_tailsize, tmp_complete_line_startpos, tmp_complete_line_endpos, &tmp_concat_line_startpos, &tmp_concat_line_endpos);
                }

                // Process the current line to get key and value
                Key tmp_key;
                Value tmp_value;
                parseCurrentLine_(tmp_concat_line_startpos, tmp_concat_line_endpos, key_column_idx, value_column_idx, column_cnt, tmp_key, tmp_value);

                // Update dataset and workload if necessary
                updateDatasetAndWorkload_(tmp_key, tmp_value, dataset_kvmap);

                // Release complete line if necessary
                if (is_achieve_trace_file_end)
                {
                    assert(tmp_complete_line_startpos != NULL);
                    free(tmp_complete_line_startpos);
                    tmp_complete_line_startpos = NULL;
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
                tmp_line_startpos = tmp_line_endpos + 1;
                if (is_achieve_mmap_block_end)
                {
                    assert(tmp_line_startpos > tmp_block_buffer_endpos);
                }
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

        return;
    }

    void WikipediaWorkloadWrapper::completeLastLine_(const char* tmp_line_startpos, const char* tmp_line_endpos, char** tmp_complete_line_startpos_ptr, char** tmp_complete_line_endpos_ptr) const
    {
        uint32_t tmp_complete_line_size = tmp_line_endpos - tmp_line_startpos + 1 + 1; // +1 for the last line separator
        *tmp_complete_line_startpos_ptr = (char*) malloc(tmp_complete_line_size);
        assert(*tmp_complete_line_startpos_ptr != NULL);
        *tmp_complete_line_endpos_ptr = *tmp_complete_line_startpos_ptr + tmp_complete_line_size - 1;

        memcpy(*tmp_complete_line_startpos_ptr, tmp_line_startpos, tmp_complete_line_size - 1); // The last line of the current (i.e., the last) mmap block
        (*tmp_complete_line_startpos_ptr)[tmp_complete_line_size - 1] = Util::LINE_SEP_CHAR; // The last line separator

        return;
    }

    void WikipediaWorkloadWrapper::concatenateLastLine_(const char* prev_block_taildata, const uint32_t& prev_block_tailsize, const char* tmp_complete_line_startpos, const char* tmp_complete_line_endpos, char** tmp_concat_line_startpos_ptr, char** tmp_concat_line_endpos_ptr) const
    {
        assert(prev_block_tailsize > 0);
        assert(prev_block_taildata != NULL);

        uint32_t tmp_concat_line_size = prev_block_tailsize + tmp_complete_line_endpos - tmp_complete_line_startpos + 1;
        *tmp_concat_line_startpos_ptr = (char*) malloc(tmp_concat_line_size);
        assert(*tmp_concat_line_startpos_ptr != NULL);
        *tmp_concat_line_endpos_ptr = *tmp_concat_line_startpos_ptr + tmp_concat_line_size - 1;

        memcpy(*tmp_concat_line_startpos_ptr, prev_block_taildata, prev_block_tailsize); // Tail data of prev mmap block
        memcpy(*tmp_concat_line_startpos_ptr + prev_block_tailsize, tmp_complete_line_startpos, tmp_complete_line_endpos - tmp_complete_line_startpos + 1); // Current line of current mmap block

        return;
    }

    void WikipediaWorkloadWrapper::parseCurrentLine_(const char* tmp_concat_line_startpos, const char* tmp_concat_line_endpos, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, Key& key, Value& value) const
    {
        char* tmp_column_startpos = tmp_concat_line_startpos;
        for (uint32_t tmp_column_idx = 0; tmp_column_idx < column_cnt; tmp_column_idx++) // For each column in the current line of current mmap block of the current trace file
        {
            assert(tmp_column_startpos <= tmp_concat_line_endpos);

            // Find the end of the current column
            char* tmp_column_endpos = tmp_concat_line_endpos; // \n
            if (tmp_column_idx != column_cnt - 1)
            {
                tmp_column_endpos = strchr(tmp_column_startpos, Util::TSV_SEP_CHAR); // \t
                assert(tmp_column_endpos != NULL);
            }

            assert(tmp_column_endpos > tmp_column_startpos); // Column MUST NOT empty except the column separator
            if (tmp_column_idx == key_column_idx)
            {
                char* tmp_key_endptr = NULL;
                int64_t tmp_keyint = strtoll(tmp_column_startpos, &tmp_key_endptr, 10);
                assert(tmp_key_endptr == tmp_column_endpos); // The first invalid char MUST be the column/line separator
                key = Key(std::string((const char*)&tmp_keyint, sizeof(int64_t)));
            }
            else if (tmp_column_idx == value_column_idx)
            {
                char* tmp_value_endptr = NULL;
                int tmp_valuesize = strtol(tmp_column_startpos, &tmp_value_endptr, 10);
                assert(tmp_value_endptr == tmp_column_endpos); // The first invalid char MUST be the column/line separator
                value = Value(tmp_valuesize);
            }
            else
            {
                // Do nothing
            }

            // Switch to the next column
            tmp_column_startpos = tmp_column_endpos + 1;
        } // End of columns in the current line of the current mmap block of the current trace file

        return;
    }

    void WikipediaWorkloadWrapper::updateDatasetAndWorkload_(const Key& key, const Value& value, std::unordered_map<Key, Value, KeyHasher>& dataset_kvmap)
    {
        std::unordered_map<Key, Value, KeyHasher>::iterator tmp_dataset_kvmap_iter = dataset_kvmap.find(key);
        if (tmp_dataset_kvmap_iter == dataset_kvmap.end()) // The first request on the key
        {
            dataset_kvmap.insert(std::pair(key, value));

            // Update dataset statistics
            average_dataset_keysize_ = (average_dataset_keysize_ * dataset_kvpairs_.size() + key.getKeysize()) / (dataset_kvpairs_.size() + 1);
            average_dataset_valuesize_ = (average_dataset_valuesize_ * dataset_kvpairs_.size() + value.getValuesize()) / (dataset_kvpairs_.size() + 1);
            if (dataset_kvpairs_.size() == 0) // The first kvpair
            {
                min_dataset_keysize_ = key.getKeysize();
                min_dataset_valuesize_ = value.getValuesize();
                max_dataset_keysize_ = key.getKeysize();
                max_dataset_valuesize_ = value.getValuesize();
            }
            else // Subsequent kvpairs
            {
                min_dataset_keysize_ = std::min(min_dataset_keysize_, key.getKeysize());
                min_dataset_valuesize_ = std::min(min_dataset_valuesize_, value.getValuesize());
                max_dataset_keysize_ = std::max(max_dataset_keysize_, key.getKeysize());
                max_dataset_valuesize_ = std::max(max_dataset_valuesize_, value.getValuesize());
            }

            dataset_kvpairs_.push_back(std::pair(key, value));
            workload_key_indices_.push_back(dataset_kvpairs_.size() - 1);
            workload_value_sizes_.push_back(-1); // Treat the first request as read request, as stresstest phase is after dataset loading phase
        }
        else // Subsequent requests on the key
        {
            if (value.getValuesize() == tmp_dataset_kvmap_iter->second.getValuesize())
            {
                workload_value_sizes_.push_back(-1); // Read request
            }
            else
            {
                workload_value_sizes_.push_back(value.getValuesize()); // Write request (put or delete)
            }
        }

        return;
    }
}

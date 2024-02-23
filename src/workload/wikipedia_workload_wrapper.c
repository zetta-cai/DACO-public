#include "workload/wikipedia_workload_wrapper.h"

#include <fcntl.h> // O_RDONLY
#include <stdlib.h> // strtoll
#include <string.h> // strchr
#include <sys/mman.h> // mmap and munmap
#include <unistd.h> // lseek

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string WikipediaWorkloadWrapper::kClassName("WikipediaWorkloadWrapper");

    WikipediaWorkloadWrapper::WikipediaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, max_eval_workload_loadcnt)
    {
        // Differentiate facebook workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // For preprocessor
        total_workload_opcnt_ = 0;

        // For preprocessor, dataset loader, and cloud
        // See WorkloadWrapperBase

        // For clients
        curclient_partial_dataset_kvmap_.clear();
        curclient_workload_keys_.clear();
        curclient_workload_value_sizes_.clear();
        eval_workload_opcnt_ = 0;

        per_client_worker_workload_idx_.resize(perclient_workercnt);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            per_client_worker_workload_idx_[i] = i;
        }
    }

    WikipediaWorkloadWrapper::~WikipediaWorkloadWrapper()
    {
    }

    WorkloadItem WikipediaWorkloadWrapper::generateWorkloadItem(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();

        assert(needWorkloadItems_()); // Must be clients for evaluation
        
        uint32_t curclient_workload_idx = per_client_worker_workload_idx_[local_client_worker_idx];
        assert(curclient_workload_idx < curclient_workload_keys_.size());

        // Get key, value, and type
        Key tmp_key = curclient_workload_keys_[curclient_workload_idx];
        Value tmp_value;
        WorkloadItemType tmp_type = WorkloadItemType::kWorkloadItemGet;
        int tmp_workload_valuesize = curclient_workload_value_sizes_[curclient_workload_idx];
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
        uint32_t next_curclient_workload_idx = (curclient_workload_idx + getPerclientWorkercnt_()) % curclient_workload_keys_.size();
        per_client_worker_workload_idx_[local_client_worker_idx] = next_curclient_workload_idx;

        return WorkloadItem(tmp_key, tmp_value, tmp_type);
    }

    uint32_t WikipediaWorkloadWrapper::getPracticalKeycnt() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getDatasetKvpairsConstRef_().size();
    }

    uint32_t WikipediaWorkloadWrapper::getTotalOpcnt() const
    {
        checkIsValid_();

        assert(needAllTraceFiles_()); // Must be trace preprocessor to load all trace files

        return total_workload_opcnt_;
    }

    WorkloadItem WikipediaWorkloadWrapper::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();

        assert(needDatasetItems_());
        assert(itemidx < getPracticalKeycnt());

        const Key tmp_covered_key = getDatasetKvpairsRef_()[itemidx].first;
        const Value tmp_covered_value = getDatasetKvpairsRef_()[itemidx].second;
        return WorkloadItem(tmp_covered_key, tmp_covered_value, WorkloadItemType::kWorkloadItemPut);
    }

    void WikipediaWorkloadWrapper::initWorkloadParameters_()
    {
        if (needAllTraceFiles_() || needWorkloadItems_()) // Need all trace files for preprocessing (preprocessor), or need partial trace files for workload items (clients)
        {
            if (needAllTraceFiles_()) // Check trace dirpath and dataset filepath for trace preprocessor
            {
                verifyDatasetFileForPreprocessor_();
            }

            parseTraceFiles_(); // Update dataset for trace preprocessor, or workloads for clients

            if (needAllTraceFiles_()) // Dump dataset file by trace preprocessor for dataset loader and cloud
            {
                const uint32_t dataset_filesize = dumpDatasetFile_();

                std::ostringstream oss;
                oss << "dump dataset file (" << dataset_filesize << " bytes) for workload " << getWorkloadName_();
                Util::dumpNormalMsg(instance_name_, oss.str());
            }
        }
        else if (needDatasetItems_()) // Need dataset items for dataset loader and cloud for warmup speedup
        {
            const uint32_t dataset_filesize = loadDatasetFile_(); // Load dataset for dataset loader and cloud (will update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics)

            std::ostringstream oss;
            oss << "load dataset file (" << dataset_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(instance_name_, oss.str());
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid workload usage role " << getWorkloadUsageRole_();
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

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

    // Get average/min/max dataset key/value size

    double WikipediaWorkloadWrapper::getAvgDatasetKeysize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getAverageDatasetKeysize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }
    
    double WikipediaWorkloadWrapper::getAvgDatasetValuesize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getAverageDatasetValuesize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t WikipediaWorkloadWrapper::getMinDatasetKeysize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getMinDatasetKeysize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t WikipediaWorkloadWrapper::getMinDatasetValuesize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getMinDatasetValuesize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t WikipediaWorkloadWrapper::getMaxDatasetKeysize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getMaxDatasetKeysize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t WikipediaWorkloadWrapper::getMaxDatasetValuesize() const
    {
        checkIsValid_();

        assert(needDatasetItems_());

        return getMaxDatasetValuesize_(); // NOT tracked by evaluation phase which may NOT load all trace files
    }

    // For warmup speedup

    void WikipediaWorkloadWrapper::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetGet_(key, value);
        return;
    }

    void WikipediaWorkloadWrapper::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetPut_(key, value);
        return;
    }

    void WikipediaWorkloadWrapper::quickDatasetDel(const Key& key)
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetDel_(key);
        return;
    }

    // Wiki-specific helper functions

    // (1) For role of preprocessor (all trace files) and clients (partial trace files)

    void WikipediaWorkloadWrapper::parseTraceFiles_()
    {
        uint32_t column_cnt = 0;
        uint32_t key_column_idx = 0;
        uint32_t value_column_idx = 0;
        std::vector<std::string> trace_filepaths; // NOTE: follow the trace order
        if (getWorkloadName_() == Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME)
        {
            column_cnt = 5;
            key_column_idx = 1; // 2nd column
            value_column_idx = 3; // 4th column
            trace_filepaths = Config::getWikiimageTraceFilepaths();
        }
        else if (getWorkloadName_() == Util::WIKIPEDIA_TEXT_WORKLOAD_NAME)
        {
            column_cnt = 4;
            key_column_idx = 1; // 2nd column
            value_column_idx = 2; // 3rd column
            trace_filepaths = Config::getWikitextTraceFilepaths();
        }
        else
        {
            std::ostringstream oss;
            oss << "Wikipedia workload name " << getWorkloadName_() << " is not supported now!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        const uint32_t trace_filecnt = trace_filepaths.size();
        assert(trace_filecnt > 0);

        std::ostringstream oss;
        oss << "load " << getWorkloadName_() << " trace files...";
        Util::dumpNormalMsg(instance_name_, oss.str());
        
        bool is_achieve_max_eval_workload_loadcnt = false;
        for (uint32_t tmp_fileidx = 0; tmp_fileidx < trace_filecnt; tmp_fileidx++) // For each trace file
        {
            const std::string tmp_filepath = trace_filepaths[tmp_fileidx];

            oss.clear();
            oss.str("");
            oss << "load " << tmp_filepath << " (" << (tmp_fileidx + 1) << "/" << trace_filecnt << ")...";
            Util::dumpNormalMsg(instance_name_, oss.str());

            // Process the current trace file
            uint32_t tmp_opcnt = parseCurrentFile_(tmp_filepath, key_column_idx, value_column_idx, column_cnt, is_achieve_max_eval_workload_loadcnt);
            if (needAllTraceFiles_()) // Trace preprocessor (all trace files)
            {
                total_workload_opcnt_ += tmp_opcnt; // NOTE: update total opcnt for trace preprocessing
            }
            else if (needWorkloadItems_()) // Clients (partial trace files)
            {
                eval_workload_opcnt_ += tmp_opcnt; // NOTE: update eval opcnt for clients during evaluation
            }

            if (is_achieve_max_eval_workload_loadcnt)
            {
                assert(needWorkloadItems_()); // Must be clients for evaluation
                assert(total_workload_opcnt_ == 0);

                oss.clear();
                oss.str("");
                oss << "achieve max workload load cnt for evaluation: " << getMaxEvalWorkloadLoadcnt_() << "; # of loaded operations: " << eval_workload_opcnt_ << "; current client workload size: " << curclient_workload_keys_.size();
                Util::dumpNormalMsg(instance_name_, oss.str());
                break;
            }
        } // End of trace files

        // TMPDEBUG24
        std::ostringstream tmposs;
        tmposs << "getWorkloadUsageRole_: " << getWorkloadUsageRole_() << "; current client workload size: " << curclient_workload_keys_.size() << "; total_workload_opcnt_: " << total_workload_opcnt_;
        Util::dumpNormalMsg(instance_name_, tmposs.str());

        return;
    }

    uint32_t WikipediaWorkloadWrapper::parseCurrentFile_(const std::string& tmp_filepath, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, bool& is_achieve_max_eval_workload_loadcnt)
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
        bool is_first_line = true; // We should skip the first title/metadata line
        uint32_t global_workload_idx = 0; // i.e., global data line index of the current trace file
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
            while (!is_achieve_max_eval_workload_loadcnt && tmp_line_startpos <= tmp_block_buffer_endpos) // For lines in the current mmap block of the current trace file
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

                if (is_first_line) // Skip the first line of the current trace file, which is a title/metadata line instead of a data line
                {
                    is_first_line = false;
                }
                else // Non-first line of the current trace file (i.e., data line)
                {
                    bool need_curline = false;
                    if (needAllTraceFiles_()) // Trace preprocessor (consider all data lines)
                    {
                        need_curline = true;
                    }
                    else if (needWorkloadItems_() && (global_workload_idx % getClientcnt_() == getClientIdx_())) // Clients (ONLY consider corresponding data lines)
                    {
                        need_curline = true;
                    }

                    if (need_curline) // The current line is partitioned to the current client
                    {
                        // Process the current line to get key and value
                        Key tmp_key;
                        Value tmp_value;
                        parseCurrentLine_(tmp_concat_line_startpos, tmp_concat_line_endpos, key_column_idx, value_column_idx, column_cnt, tmp_key, tmp_value);

                        // Update dataset and workload if necessary
                        updateDatasetOrWorkload_(tmp_key, tmp_value);
                    }
                    global_workload_idx++;

                    if (needWorkloadItems_() && eval_workload_opcnt_ + global_workload_idx >= getMaxEvalWorkloadLoadcnt_()) // Clients for evaluation
                    {
                        is_achieve_max_eval_workload_loadcnt = true; // Stop parsing trace files
                    }
                }

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
                if (is_achieve_trace_file_end)
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

            if (is_achieve_max_eval_workload_loadcnt) // Stop parsing trace files
            {
                break;
            }
        } // End of mmap blocks of the current trace file

        // Close file
        Util::closeFile(tmp_fd);

        return global_workload_idx;
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
        const char* tmp_column_startpos = tmp_concat_line_startpos;
        for (uint32_t tmp_column_idx = 0; tmp_column_idx < column_cnt; tmp_column_idx++) // For each column in the current line of current mmap block of the current trace file
        {
            assert(tmp_column_startpos <= tmp_concat_line_endpos);

            // Find the end of the current column
            const char* tmp_column_endpos = tmp_concat_line_endpos; // \n
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

    // (2) Common utilities

    void WikipediaWorkloadWrapper::updateDatasetOrWorkload_(const Key& key, const Value& value)
    {
        if (needAllTraceFiles_()) // Preprocessor (all trace files)
        {
            std::unordered_map<Key, uint32_t, KeyHasher>& tmp_dataset_lookup_table_ref = getDatasetLookupTableRef_();
            std::vector<std::pair<Key, Value>>& tmp_dataset_kvpairs_ref = getDatasetKvpairsRef_();

            std::unordered_map<Key, uint32_t, KeyHasher>::iterator tmp_dataset_lookup_table_iter = tmp_dataset_lookup_table_ref.find(key);
            if (tmp_dataset_lookup_table_iter == tmp_dataset_lookup_table_ref.end()) // The first request on the key
            {
                const uint32_t original_dataset_size = tmp_dataset_kvpairs_ref.size();
                tmp_dataset_kvpairs_ref.push_back(std::pair(key, value));
                tmp_dataset_lookup_table_ref.insert(std::pair(key, original_dataset_size));

                // Update dataset statistics
                updateDatasetStatistics_(key, value, original_dataset_size);
            }
        }
        else if (needWorkloadItems_()) // Clients (partial trace files) for evaluation phase
        {
            std::unordered_map<Key, Value, KeyHasher>::iterator tmp_partial_dataset_kvmap_iter = curclient_partial_dataset_kvmap_.find(key);
            if (tmp_partial_dataset_kvmap_iter == curclient_partial_dataset_kvmap_.end()) // The first request on the key
            {
                curclient_partial_dataset_kvmap_.insert(std::pair(key, value));

                curclient_workload_keys_.push_back(key);
                curclient_workload_value_sizes_.push_back(-1); // Treat the first request as read request, as stresstest phase is after dataset loading phase
            }
            else // Subsequent requests on the key
            {
                const Value& original_value = tmp_partial_dataset_kvmap_iter->second;

                curclient_workload_keys_.push_back(key);
                if (value.getValuesize() == original_value.getValuesize())
                {
                    curclient_workload_value_sizes_.push_back(-1); // Read request
                }
                else
                {
                    curclient_workload_value_sizes_.push_back(value.getValuesize()); // Write request (put or delete)
                }
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid workload usage role " << getWorkloadUsageRole_() << ", which does not need both dataset and workload items";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }
}

/*
 * WikipediaWorkloadWrapper: encapsulate TSV file readers for Wiki CDN trace (https://analytics.wikimedia.org/published/datasets/caching/2019/) (thread safe).
 *
 * NOTE: Wiki TSV (Tag-Separated-Value) files have the following formats:
 * -> Image TSV: relative_unix hashed_path_query image_type response_size time_firstbyte
 * -> Text TSV: relative_unix hashed_path_query response_size time_firstbyte
 * 
 * NOTE: (i) as Wiki image and text CDN workloads have large I/O overhead for loading trace files, we MUST distinguish loading and evaluation phases -> (ii) In loading phase, we ONLY load dataset instead of tracking workload items; while in evaluation phase, we ONLY track workload items instead of dataset -> (iii) Although we cannot use dataset key indices for workload items, the space cost is acceptable due to not-many workload items under geo-distributed tiered storage (large propagation latency limits throughput and hence # of workload items).
 * 
 * By Siyuan Sheng (2024.02.15).
 */

#ifndef WIKIPEDIA_WORKLOAD_WRAPPER_H
#define WIKIPEDIA_WORKLOAD_WRAPPER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class WikipediaWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        WikipediaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt);
        virtual ~WikipediaWorkloadWrapper();

        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) override; // NOTE: follow the real-world trace to select an item without modifying any variable -> thread safe
        virtual uint32_t getPracticalKeycnt() const override;
        virtual uint32_t getTotalOpcnt() const override;
        virtual WorkloadItem getDatasetItem(const uint32_t itemidx) override; // Get a dataset key-value pair item with the index of itemidx

        // Get average/min/max dataset key/value size
        virtual double getAvgDatasetKeysize() const override;
        virtual double getAvgDatasetValuesize() const override;
        virtual uint32_t getMinDatasetKeysize() const override;
        virtual uint32_t getMinDatasetValuesize() const override;
        virtual uint32_t getMaxDatasetKeysize() const override;
        virtual uint32_t getMaxDatasetValuesize() const override;
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // Wiki-specific helper functions

        // (1) For role of preprocessor (all trace files) and clients (partial trace files)

        void parseTraceFiles_();
        uint32_t parseCurrentFile_(const std::string& tmp_filepath, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, bool& is_achieve_max_eval_workload_loadcnt); // Process the current trace file (return # of lines in the current trace file)
        void completeLastLine_(const char* tmp_line_startpos, const char* tmp_line_endpos, char** tmp_complete_line_startpos_ptr, char** tmp_complete_line_endpos_ptr) const; // Complete the last line of a trace file by an extra line separator
        void concatenateLastLine_(const char* prev_block_taildata, const uint32_t& prev_block_tailsize, const char* tmp_complete_line_startpos, const char* tmp_complete_line_endpos, char** tmp_concat_line_startpos_ptr, char** tmp_concat_line_endpos_ptr) const; // Concatenate tail data of the previous mmap block with the first complete line of the current mmap block
        void parseCurrentLine_(const char* tmp_concat_line_startpos, const char* tmp_concat_line_endpos, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, Key& key, Value& value) const; // Parse a line to get key and value
        void updateDatasetOrWorkload_(const Key& key, const Value& value); // Update dataset or workload with the key-value pair from all/partial trace files

        // Const shared variables
        std::string instance_name_;

        // Const shared variables
        // (1) For role of preprocessor
        uint32_t total_workload_opcnt_; // Total opcnt of workloads in all clients
        // (2) For role of preprocessor, dataset loader, and cloud
        // See WorkloadWrapperBase
        // (3) For role of clients during evaluation
        std::unordered_map<Key, Value, KeyHasher> curclient_partial_dataset_kvmap_; // Key-value map of partial dataset accessed by workload in current client (compare with original value sizes for workload item types)
        std::vector<Key> curclient_workload_keys_; // Key indices of workload in the current client
        std::vector<int> curclient_workload_value_sizes_; // Value sizes of workload in the current client (< 0: read; = 0: delete; > 0: write)
        uint32_t eval_workload_opcnt_; // Evaluation opcnt of workloads in all clients

        // Non-const individual variables
        std::vector<uint32_t> per_client_worker_workload_idx_; // Track per-clientworker workload index
    };
}

#endif
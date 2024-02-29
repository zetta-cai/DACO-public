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
#include "workload/replayed_workload_wrapper_base.h"

namespace covered
{
    class WikipediaWorkloadWrapper : public ReplayedWorkloadWrapperBase
    {
    public:
        WikipediaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt);
        virtual ~WikipediaWorkloadWrapper();
    private:
        static const std::string kClassName;

        // Wiki-specific helper functions

        // (1) For role of preprocessor (all trace files) and clients (partial trace files)

        virtual void parseTraceFiles_() override;
        void parseCurrentFile_(const std::string& tmp_filepath, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, bool& is_achieve_max_eval_workload_loadcnt); // Process the current trace file
        void completeLastLine_(const char* tmp_line_startpos, const char* tmp_line_endpos, char** tmp_complete_line_startpos_ptr, char** tmp_complete_line_endpos_ptr) const; // Complete the last line of a trace file by an extra line separator
        void concatenateLastLine_(const char* prev_block_taildata, const uint32_t& prev_block_tailsize, const char* tmp_complete_line_startpos, const char* tmp_complete_line_endpos, char** tmp_concat_line_startpos_ptr, char** tmp_concat_line_endpos_ptr) const; // Concatenate tail data of the previous mmap block with the first complete line of the current mmap block
        void parseCurrentLine_(const char* tmp_concat_line_startpos, const char* tmp_concat_line_endpos, const uint32_t& key_column_idx, const uint32_t& value_column_idx, const uint32_t& column_cnt, Key& key, Value& value) const; // Parse a line to get key and value

        // Const shared variables
        std::string instance_name_;

        // NOTE: see more const and non-const variables in ReplayedWorkloadWrapperBase
    };
}

#endif
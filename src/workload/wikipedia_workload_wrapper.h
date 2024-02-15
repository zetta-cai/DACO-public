/*
 * WikipediaWorkloadWrapper: encapsulate TSV file readers for Wiki CDN trace (https://analytics.wikimedia.org/published/datasets/caching/2019/) (thread safe).
 *
 * NOTE: Wiki TSV (Tag-Separated-Value) files have the following formats:
 * -> Image TSV: relative_unix hashed_path_query image_type response_size time_firstbyte
 * -> Text TSV: relative_unix hashed_path_query response_size time_firstbyte
 * 
 * By Siyuan Sheng (2024.02.15).
 */

#ifndef WIKIPEDIA_WORKLOAD_WRAPPER_H
#define WIKIPEDIA_WORKLOAD_WRAPPER_H

#include <string>
#include <vector>

#include "workload/wikipedia_workload_extra_param.h"
#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class WikipediaWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        WikipediaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const WikipediaWorkloadExtraParam& workload_extra_param);
        virtual ~WikipediaWorkloadWrapper();

        virtual uint32_t getPracticalKeycnt() const override;

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

        // NOTE: follow the real-world trace to select an item without modifying any variable -> thread safe
        virtual WorkloadItem generateWorkloadItemInternal_(const uint32_t& local_client_worker_idx) override;

        // Get a dataset key-value pair item with the index of itemidx
        virtual WorkloadItem getDatasetItemInternal_(const uint32_t itemidx) override;

        // Const shared variables
        std::string instance_name_;
        const WikipediaWorkloadExtraParam workload_extra_param_;

        // Const shared variables
        std::vector<Key, Value> dataset_kvpairs_; // Key-value pairs of dataset
        std::vector<uint32_t> workload_key_indices_; // Key indices of workload
        std::vector<int> workload_value_sizes_; // Value sizes of workload (< 0: read; = 0: delete; > 0: write)
    };
}

#endif
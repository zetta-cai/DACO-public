/*
 * (OBSOLETE) ReplayedWorkloadWrapperBase: the base class ONLY for replayed workload wrappers (replay single-node traces by sampling and partitioning).
 *
 * OBSOLETE reason: unreasonable results due to incorrect trace partitioning without geographical information.
 *
 * NOTE: basically follow WorkloadWrapperBase, yet with some variables and functions specific for replayed workloads.
 * 
 * NOTE: we randomly sample workload items due to limitation of memory capacity in our testbed, yet still keep the original workload distribution.
 * --> Approach 1: use std::bernoulli_distribution with probability of workload_sample_ratio_ to make a decision for each workload item -> after n independent trials (time complexity is O(n)), will finally get trace_sample_opcnt_ items.
 * --> Approach 2: use std::shuffle to randomly rearrange the order of total workload items, and then choose the first trace_sample_opcnt_ items -> time complexity of std::shuffle is O(n), which needs to generate n random numbers to swap items from end to begin of total workload items.
 * --> Here we use approach 1, which can perform sampling for each workload item individually, while std::shuffle in approach 2 needs all workload items before sampling (may exceed total memory capacity) and require 3 memory copies due to in-place swapping.
 * 
 * By Siyuan Sheng (2024.02.26).
 */

#ifndef REPLAYED_WORKLOAD_WRAPPER_BASE_H
#define REPLAYED_WORKLOAD_WRAPPER_BASE_H

#include <random> // std::mt19937 and std::bernoulli_distribution

#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ReplayedWorkloadWrapperBase : public WorkloadWrapperBase
    {
    public:
        ReplayedWorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& workload_randombase);
        virtual ~ReplayedWorkloadWrapperBase();

        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) override; // NOTE: follow the real-world trace to select an item without modifying any variable -> thread safe
        virtual uint32_t getPracticalKeycnt() const override;
        virtual WorkloadItem getDatasetItem(const uint32_t itemidx) override; // Get a dataset key-value pair item with the index of itemidx

        // Get average/min/max dataset key/value size
        virtual double getAvgDatasetKeysize() const override;
        virtual double getAvgDatasetValuesize() const override;
        virtual uint32_t getMinDatasetKeysize() const override;
        virtual uint32_t getMinDatasetValuesize() const override;
        virtual uint32_t getMaxDatasetKeysize() const override;
        virtual uint32_t getMaxDatasetValuesize() const override;

        // For warmup speedup
        virtual void quickDatasetGet(const Key& key, Value& value) const override;
        virtual void quickDatasetPut(const Key& key, const Value& value) override;
        virtual void quickDatasetDel(const Key& key) override;
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // Helper functions (ONLY for replayed traces)

        // (1) For role of trace preprocessor

        void verifyDatasetAndWorkloadAbsenceForPreprocessor_(); // Dataset and workload file should NOT exist
        virtual void parseTraceFiles_(const uint32_t& preprocess_phase) = 0;
        void calculateWorkloadSampleRatio_();
        uint32_t dumpDatasetFile_() const; // Dump dataset key-value pairs into dataset file; return dataset file size (in units of bytes)
        uint32_t dumpWorkloadFile_() const; // Dump total workload key-value pairs into workload file; return workload file size (in units of bytes)

        // (2) For role of dataset loader and cloud

        uint32_t loadDatasetFile_(); // Load dataset key-value pairs to update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics; return dataset file size (in units of bytes)

        // (3) For role of cloud for warmup speedup

        void quickDatasetGet_(const Key& key, Value& value) const;
        void quickDatasetPut_(const Key& key, const Value& value);
        void quickDatasetDel_(const Key& key);

        // (4) For role of clients during evaluation

        uint32_t loadWorkloadFile_(); // Load total workload key-value pairs to update curclient_workload_keys_, curclient_workload_value_sizes_, and eval_workload_opcnt_; return partial workload file size (in units of bytes)

        // Const shared variables
        std::string base_instance_name_;

        // Const shared variables (ONLY for replayed traces)
        // (A) For role of preprocessor
        uint32_t total_workload_opcnt_; // Total opcnt of workloads in all clients before sampling (used to calculate workload sample ratio)
        double workload_sample_ratio_; // Sample ratio for workload items (calculated by trace sample opcnt / toatl workload opcnt)
        std::vector<Key> sample_workload_keys_; // Keys of total workload in all clients after sampling
        std::vector<int> sample_workload_value_sizes_; // Value sizes of workload in all clients after sampling (< 0: read; = 0: delete; > 0: write)
        std::mt19937* workload_sample_randgen_ptr_; // Random number generator for sampling workload items
        std::bernoulli_distribution* workload_sample_dist_ptr_; // Bernoulli distribution for sampling workload items
        // (B) For role of preprocessor, dataset loader, and cloud
        // NOTE: non-replayed traces can generate all information (dataset items, workload items, dataset statistics) by workload generator
        double average_dataset_keysize_; // Average dataset key size
        double average_dataset_valuesize_; // Average dataset value size
        uint32_t min_dataset_keysize_; // Minimum dataset key size
        uint32_t min_dataset_valuesize_; // Minimum dataset value size
        uint32_t max_dataset_keysize_; // Maximum dataset key size
        uint32_t max_dataset_valuesize_; // Maximum dataset value size
        std::unordered_map<Key, uint32_t, KeyHasher> dataset_lookup_table_; // Fast indexing for dataset key-value pairs after sampling
        std::vector<std::pair<Key, Value>> dataset_kvpairs_; // Key-value pairs of dataset after sampling
        // (C) For role of clients during evaluation
        std::vector<Key> curclient_workload_keys_; // Keys of workload in the current client
        std::vector<int> curclient_workload_value_sizes_; // Value sizes of workload in the current client (< 0: read; = 0: delete; > 0: write)

        // Non-const individual variables
        std::vector<uint32_t> per_client_worker_workload_idx_; // Track per-clientworker workload index
    protected:
        static const uint32_t PHASE_FOR_WORKLOAD_SAMPLE_RATIO; // Phase 0 of trace preprocessing
        static const uint32_t PHASE_FOR_WORKLOAD_SAMPLE; // Phase 1 of trace preprocessing

        // (5) For role of trace preprocessor, dataset loader, and cloud

        bool updateDatasetOrSampleWorkload_(const Key& key, const Value& value, const uint32_t& preprocess_phase); // Update dataset items from all trace files (also update and sample workload items) or dataset file with the key-value pair (return true if clients achieve trace sample opcnt)
        void updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size); // Update dataset statistics (e.g., average/min/max dataset key/value size)
    };
}

#endif
/*
 * ReplayedWorkloadWrapperBase: the base class ONLY for replayed workload wrappers.
 *
 * NOTE: basically follow WorkloadWrapperBase, yet with some variables and functions specific for replayed workloads.
 * 
 * By Siyuan Sheng (2024.02.26).
 */

#ifndef REPLAYED_WORKLOAD_WRAPPER_BASE_H
#define REPLAYED_WORKLOAD_WRAPPER_BASE_H

#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ReplayedWorkloadWrapperBase : public WorkloadWrapperBase
    {
    public:
        ReplayedWorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt);
        virtual ~ReplayedWorkloadWrapperBase();

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

        // For warmup speedup
        virtual void quickDatasetGet(const Key& key, Value& value) const override;
        virtual void quickDatasetPut(const Key& key, const Value& value) override;
        virtual void quickDatasetDel(const Key& key) override;
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        virtual void parseTraceFiles_() = 0;

        // Const shared variables
        std::string base_instance_name_;
        double dataset_sample_ratio_; // Sample ratio for dataset (1.0: all dataset items; < 1.0: sample ratio of dataset items)

        // Const shared variables (ONLY for replayed traces)
        // (1) For role of preprocessor
        uint32_t total_workload_opcnt_; // Total opcnt of workloads in all clients
        std::vector<Key> total_workload_keys_; // Keys of total workload in all clients (updated ONLY if sample ratio < 1.0 for sampling)
        std::vector<int> total_workload_value_sizes_; // Value sizes of workload in all clients (< 0: read; = 0: delete; > 0: write; updated ONLY if sample ratio < 1.0 for sampling)
        // (2) For role of preprocessor, dataset loader, and cloud
        // NOTE: non-replayed traces can generate all information (dataset items, workload items, dataset statistics) by workload generator
        double average_dataset_keysize_; // Average dataset key size
        double average_dataset_valuesize_; // Average dataset value size
        uint32_t min_dataset_keysize_; // Minimum dataset key size
        uint32_t min_dataset_valuesize_; // Minimum dataset value size
        uint32_t max_dataset_keysize_; // Maximum dataset key size
        uint32_t max_dataset_valuesize_; // Maximum dataset value size
        std::unordered_map<Key, uint32_t, KeyHasher> dataset_lookup_table_; // Fast indexing for dataset key-value pairs (will be sampled if sample ratio < 1.0)
        std::vector<std::pair<Key, Value>> dataset_kvpairs_; // Key-value pairs of dataset (will be sampled if sample ratio < 1.0)
        // (3) For role of clients during evaluation
        std::unordered_map<Key, Value, KeyHasher> curclient_partial_dataset_kvmap_; // Key-value map of partial dataset accessed by workload in current client (compare with original value sizes for workload item types; ONLY used by clients to calculate coded value sizes, if load partial trace files under sample ratio of 1.0)
        std::vector<Key> curclient_workload_keys_; // Keys of workload in the current client
        std::vector<int> curclient_workload_value_sizes_; // Value sizes of workload in the current client (< 0: read; = 0: delete; > 0: write)
        uint32_t eval_workload_opcnt_; // Evaluation opcnt of workloads in all clients

        // Non-const individual variables
        std::vector<uint32_t> per_client_worker_workload_idx_; // Track per-clientworker workload index
    protected:
        // Helper functions (ONLY for replayed traces)

        // (1) For role of trace preprocessor

        void verifyDatasetAndWorkloadAbsenceForPreprocessor_(); // Dataset and workload file (if sample ratio < 1) should NOT exist
        void sampleDatasetAndWorkload_(); // Sample dataset and total workload items (if sample ratio < 1.0; will update dataset_kvpairs_, dataset_lookup_table_, dataset statistics, total_workload_keys_, total_workload_value_sizes_, and total_workload_opcnt_)
        void sampleDatasetInternal_(); // Sample dataset items under sample ratio < 1.0 (update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics)
        void sampleWorkloadInternal_(); // Sample total workload items under sample ratio < 1.0 (update total_workload_keys_, total_workload_value_sizes_, and total_workload_opcnt_)
        uint32_t dumpDatasetFile_() const; // Dump dataset key-value pairs into dataset file; return dataset file size (in units of bytes)
        uint32_t dumpWorkloadFile_() const; // Dump total workload key-value pairs into workload file; return workload file size (in units of bytes)

        // (2) For role of dataset loader and cloud

        uint32_t loadDatasetFile_(); // Load dataset key-value pairs to update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics; return dataset file size (in units of bytes)

        // (3) For role of cloud for warmup speedup

        void quickDatasetGet_(const Key& key, Value& value) const;
        void quickDatasetPut_(const Key& key, const Value& value);
        void quickDatasetDel_(const Key& key);

        // (4) For role of clients during evaluation

        uint32_t loadWorkloadFile_(); // Load total workload key-value pairs to update curclient_workload_keys_, curclient_workload_value_sizes_, and eval_workload_opcnt_ (NO need curclient_partial_dataset_kvmap_); return partial workload file size (in units of bytes)
        void dumpInfoIfAchieveMaxLoadCnt_() const; // Dump info if clients achieve max evaluation workload loadcnt

        // (5) Common utilities

        bool updateDatasetOrWorkload_(const Key& key, const Value& value); // Update dataset (from all trace files or dataset file) or workload (from partial trace files or partial workload file) with the key-value pair (return true if clients achieve max evaluation workload loadcnt)
        void updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size); // Update dataset statistics (e.g., average/min/max dataset key/value size)
        void resetDatasetStatistics_(); // Reset dataset statistics (e.g., average/min/max dataset key/value size) for sampling
    };
}

#endif
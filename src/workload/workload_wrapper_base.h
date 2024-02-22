/*
 * WorkloadWrapperBase: the base class for workload generator to generate keycnt dataset key-value pairs with Util::DATASET_KVPAIR_GENERATION_SEED as the seed and opcnt/clientcnt workload requests/items with client_idx as the seed (thread safe).
 *
 * Access global covered::Config and covered::Param for static and dynamic configurations.
 * Overwrite some workload parameters (e.g., numOps and numKeys) based on static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef WORKLOAD_WRAPPER_BASE_H
#define WORKLOAD_WRAPPER_BASE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "workload/workload_item.h"

namespace covered
{
    class WorkloadWrapperBase
    {
    public:
        static const std::string WORKLOAD_USAGE_ROLE_PREPROCESSOR;
        static const std::string WORKLOAD_USAGE_ROLE_LOADER;
        static const std::string WORKLOAD_USAGE_ROLE_CLIENT;
        static const std::string WORKLOAD_USAGE_ROLE_CLOUD;

        static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt = 0);
        // static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint64_t& capacity_bytes, const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt = 0); // (OBSOLETE due to already checking objsize in LocalCacheBase)

        WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt);
        virtual ~WorkloadWrapperBase();

        // Access by the single thread of client wrapper (NO need to be thread safe)
        void validate(); // Provide individual validate() as contructor cannot invoke virtual functions

        // Access by multiple client workers (thread safe)
        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) = 0;
        virtual uint32_t getPracticalKeycnt() const = 0;
        virtual uint32_t getTotalOpcnt() const = 0;
        virtual WorkloadItem getDatasetItem(const uint32_t itemidx) = 0; // Get a dataset key-value pair item with the index of itemidx

        // Get average/min/max dataset key/value size
        virtual double getAvgDatasetKeysize() const = 0;
        virtual double getAvgDatasetValuesize() const = 0;
        virtual uint32_t getMinDatasetKeysize() const = 0;
        virtual uint32_t getMinDatasetValuesize() const = 0;
        virtual uint32_t getMaxDatasetKeysize() const = 0;
        virtual uint32_t getMaxDatasetValuesize() const = 0;

        // For warmup speedup
        virtual void quickDatasetGet(const Key& key, Value& value) const = 0;
        virtual void quickDatasetPut(const Key& key, const Value& value) = 0;
        virtual void quickDatasetDel(const Key& key) = 0;
    private:
        static const std::string kClassName;

        // Access by the single thread of client wrapper (NO need to be thread safe)
        virtual void initWorkloadParameters_() = 0; // initialize workload parameters (e.g., by default or by loading config file)
        virtual void overwriteWorkloadParameters_() = 0; // overwrite some workload patermers based on covered::Config and covered::Param
        virtual void createWorkloadGenerator_() = 0; // create workload generator based on overwritten workload parameters

        // Const shared variables
        std::string base_instance_name_;
        bool is_valid_;

        // Const shared variables coming from Param
        // ONLY for clients
        const uint32_t clientcnt_;
        const uint32_t client_idx_;
        const uint32_t perclient_opcnt_;
        const uint32_t perclient_workercnt_;
        const uint32_t max_eval_workload_loadcnt_; // ONLY used in evaluation phase
        // For all roles
        const uint32_t keycnt_;
        const std::string workload_name_;
        const std::string workload_usage_role_; // Distinguish different roles to avoid file I/O overhead of loading all trace files (for dataset loader and clients during loading/evaluation), and avoid disk I/O overhead of accessing rocksdb (for cloud during warmup)

        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        // NOTE: non-replayed traces can generate all information (dataset items, workload items, dataset statistics) by workload generator
        double average_dataset_keysize_; // Average dataset key size
        double average_dataset_valuesize_; // Average dataset value size
        uint32_t min_dataset_keysize_; // Minimum dataset key size
        uint32_t min_dataset_valuesize_; // Minimum dataset value size
        uint32_t max_dataset_keysize_; // Maximum dataset key size
        uint32_t max_dataset_valuesize_; // Maximum dataset value size
        std::unordered_map<Key, uint32_t, KeyHasher> dataset_lookup_table_; // Fast indexing for dataset key-value pairs
        std::vector<std::pair<Key, Value>> dataset_kvpairs_; // Key-value pairs of dataset
    protected:
        // Getters for const shared variables coming from Param
        // ONLY for clients
        const uint32_t getClientcnt_() const;
        const uint32_t getClientIdx_() const;
        const uint32_t getPerclientOpcnt_() const;
        const uint32_t getPerclientWorkercnt_() const;
        const uint32_t getMaxEvalWorkloadLoadcnt_() const;
        // For all roles
        const uint32_t getKeycnt_() const;
        const std::string getWorkloadName_() const;
        const std::string getWorkloadUsageRole_() const;

        // (1) ONLY for replayed traces, which have dataset file dumped by trace preprocessor

        // (1.1) For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        const double getAverageDatasetKeysize_() const;
        const double getAverageDatasetValuesize_() const;
        const uint32_t getMinDatasetKeysize_() const;
        const uint32_t getMinDatasetValuesize_() const;
        const uint32_t getMaxDatasetKeysize_() const;
        const uint32_t getMaxDatasetValuesize_() const;
        std::unordered_map<Key, uint32_t, KeyHasher>& getDatasetLookupTableRef_();
        std::vector<std::pair<Key, Value>>& getDatasetKvpairsRef_();

        // (1.2) For role of trace preprocessor (ONLY for replayed traces)

        void verifyDatasetFileForPreprocessor_();
        uint32_t dumpDatasetFile_() const; // Dump dataset key-value pairs into dataset file; return dataset file size (in units of bytes)

        // (1.3) For role of dataset loader and cloud (ONLY for replayed traces)

        uint32_t loadDatasetFile_(); // Load dataset key-value pairs to update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics; return dataset file size (in units of bytes)

        // (1.4) For role of cloud for warmup speedup (ONLY for replayed traces)

        void quickDatasetGet_(const Key& key, Value& value) const;
        void quickDatasetPut_(const Key& key, const Value& value);
        void quickDatasetDel_(const Key& key);

        // (1.5) Common utilities (ONLY for replayed traces)

        void updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size); // Update dataset statistics (e.g., average/min/max dataset key/value size)

        // (2) Other common utilities

        bool needAllTraceFiles_();
        bool needDatasetItems_();
        bool needWorkloadItems_();

        void checkIsValid_() const;
    };
}

#endif
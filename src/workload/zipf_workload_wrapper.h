/*
 * ZipfWorkloadWrapper: use power-law Zipf distribution to reproduce replayed (single-node) traces with geographical information based on Zipfian constant and key/value size distributions (thread safe).
 * 
 * By Siyuan Sheng (2024.08.05).
 */

#ifndef ZIPF_WORKLOAD_WRAPPER_H
#define ZIPF_WORKLOAD_WRAPPER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <random> // std::mt19937_64, std::discrete_distribution

#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ZipfWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        ZipfWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha);
        virtual ~ZipfWorkloadWrapper();

        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) override; // NOTE: randomly select an item without modifying any variable -> thread safe
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

        void loadZipfCharacteristicsFile_(); // Load Zipfian constant, key size histogram, and value size histogram (required by all roles including clients, dataset loader, and cloud)
        static std::string getKeystrFromKeyrank_(const int64_t& keyrank, const uint32_t& keysize); // Generate a key string based on the key rank with key size bytes
        static int64_t getKeyrankFromKeystr_(const std::string& keystr); // Generate a key rank based on the key string

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // (1) Zipf-specific helper functions

        // Common utilities
        void checkPointers_() const;

        // (2) Statistics variables
        
        // For role of clients, dataset loader, and cloud
        double average_dataset_keysize_; // Average dataset key size
        double average_dataset_valuesize_; // Average dataset value size
        uint32_t min_dataset_keysize_; // Minimum dataset key size
        uint32_t min_dataset_valuesize_; // Minimum dataset value size
        uint32_t max_dataset_keysize_; // Maximum dataset key size
        uint32_t max_dataset_valuesize_; // Maximum dataset value size

        // (3) Const shared variables
        std::string instance_name_;
        // const float zipf_alpha_; // NOTE: we use the Zipfian constant loaded from the characteristics file, while that from CLI is ONLY used for Zipfian Facebook CDN workload to tune workload skewness

        // (4) Other shared variables
        // For clients, dataset loader, and cloud
        std::vector<std::string> dataset_keys_; // Client needs dataset_keys_ to sample for workload items
        std::vector<double> dataset_probs_;
        std::vector<uint32_t> dataset_valsizes_; // Client needs dataset_valsizes_ to generate workload items (although NOT used by client workers due to GET requests)
        // For clients
        std::vector<std::mt19937_64*> client_worker_item_randgen_ptrs_;
        std::vector<std::discrete_distribution<uint32_t>*> client_worker_reqdist_ptrs_; // randomly select request index from workload indices of each client
        // std::vector<uint32_t> workload_key_indices_; // workload indices for each client (NOTE: NO need due to directly generating workload items by power-law Zipf distribution)

        // (5) Optype ratios for workloads requiring them
        double read_ratio_;
        double update_ratio_;
        double delete_ratio_;
        double insert_ratio_;
        // For clients under such workloads
        std::vector<std::mt19937_64*> client_worker_optype_randgen_ptrs_;
        std::vector<std::uniform_real_distribution<double>*> client_worker_optype_dist_ptrs_;
    };
}

#endif
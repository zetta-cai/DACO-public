/*
 * FbphotoWorkloadWrapper: reproduce the workload of Facebook photo caching based on distributions mentioned in the paper (https://dl.acm.org/doi/pdf/10.1145/2517349.2522722) (thread safe).
 * 
 * NOTE: you should run scripts/workload/facebook_photocache.py first to generate the dataset of Facebook photo caching.
 * 
 * NOTE: FacebookWorkloadWrapper is for OSDI'20 Facebook CDN trace, while FbphotoWorkloadWrapper is for SOSP'13 Facebook photo caching trace
 * 
 * By Siyuan Sheng (2024.04.01).
 */

#ifndef FBPHOTO_WORKLOAD_WRAPPER_H
#define FBPHOTO_WORKLOAD_WRAPPER_H

#include <random> // std::mt19937_64, std::discrete_distribution
#include <string>
#include <vector>

#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class FbphotoWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        static const uint32_t FBPHOTO_WORKLOAD_DATASET_KEYCNT; // 1.3M dataset items

        FbphotoWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role);
        virtual ~FbphotoWorkloadWrapper();

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

        void loadFbphotoDatasetFile_();

        virtual void initWorkloadParameters_() override;
        virtual void overwriteWorkloadParameters_() override;
        virtual void createWorkloadGenerator_() override;

        // Facebook-specific helper functions

        // (1) For role of clients, dataset loader, and cloud
        double average_dataset_keysize_; // Average dataset key size
        double average_dataset_valuesize_; // Average dataset value size
        uint32_t min_dataset_keysize_; // Minimum dataset key size
        uint32_t min_dataset_valuesize_; // Minimum dataset value size
        uint32_t max_dataset_keysize_; // Maximum dataset key size
        uint32_t max_dataset_valuesize_; // Maximum dataset value size

        // (2) Common utilities

        void checkPointers_() const;

        // Const shared variables
        std::string instance_name_;

        // Const shared variables
        // (1) For clients, dataset loader, and cloud
        std::vector<uint32_t> dataset_keys_;
        std::vector<double> dataset_probs_;
        std::vector<uint32_t> dataset_valsizes_;
        // (2) For clients
        std::vector<std::mt19937_64*> client_worker_item_randgen_ptrs_;
        std::vector<std::uniform_int_distribution<uint32_t>*> client_worker_reqdist_ptrs_; // randomly select request index from workload indices of each client
        std::vector<uint32_t> workload_key_indices_; // workload indices for each client (follow dataset probs)
    };
}

#endif
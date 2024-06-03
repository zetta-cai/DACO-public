/*
 * ZetaWorkloadWrapper: use Zeta distribution to reproduce replayed traces with geographical information based on Zipfian constant and key/value size distributions (thread safe).
 * 
 * By Siyuan Sheng (2024.05.30).
 */

#ifndef ZETA_WORKLOAD_WRAPPER_H
#define ZETA_WORKLOAD_WRAPPER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <random> // std::mt19937_64, std::discrete_distribution

#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class ZetaWorkloadWrapper : public WorkloadWrapperBase
    {
    public:
        static const uint32_t RIEMANN_ZETA_PRECISION; // Precision for Riemann Zeta function calculation

        ZetaWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha);
        virtual ~ZetaWorkloadWrapper();

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

        void loadZetaCharacteristicsFile_(); // Load Zipfian constant, key size histogram, and value size histogram
        static std::string generateKeystr_(const uint32_t& keyrank, const uint32_t& keysize); // Generate a key string based on the key rank with key size bytes
        static double calcRiemannZeta_(const double& zipf_constant); // Calculate the Riemann Zeta function for the Zipfian constant

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
        // const float zipf_alpha_; // NOTE: we use the Zipfian constant loaded from the characteristics file, while that from CLI is ONLY used for Zipfian Facebook CDN workload to tune workload skewness

        // Const shared variables
        // (1) For clients, dataset loader, and cloud
        std::vector<std::string> dataset_keys_;
        std::vector<double> dataset_probs_;
        std::vector<uint32_t> dataset_valsizes_;
        // (2) For clients
        std::vector<std::mt19937_64*> client_worker_item_randgen_ptrs_;
        std::vector<std::discrete_distribution<uint32_t>*> client_worker_reqdist_ptrs_; // randomly select request index from workload indices of each client
        // std::vector<uint32_t> workload_key_indices_; // workload indices for each client (NOTE: NO need due to directly generating workload items by Zeta distribution)
    };
}

#endif
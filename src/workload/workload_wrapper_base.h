/*
 * WorkloadWrapperBase: the base class for replayed and non-replayed workload wrappers (thread safe).
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

        static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha);
        // static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint64_t& capacity_bytes, const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role); // (OBSOLETE due to already checking objsize in LocalCacheBase)

        WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role);
        virtual ~WorkloadWrapperBase();

        // Access by the single thread of client wrapper (NO need to be thread safe)
        void validate(); // Provide individual validate() as contructor cannot invoke virtual functions

        // Access by multiple client workers (thread safe)
        virtual WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx) = 0;
        virtual uint32_t getPracticalKeycnt() const = 0;
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
        // For all roles
        const uint32_t keycnt_;
        const std::string workload_name_;
        const std::string workload_usage_role_; // Distinguish different roles to avoid file I/O overhead of loading all trace files (for dataset loader and clients during loading/evaluation), and avoid disk I/O overhead of accessing rocksdb (for cloud during warmup)
    protected:
        // Getters for const shared variables coming from Param
        // ONLY for clients
        const uint32_t getClientcnt_() const;
        const uint32_t getClientIdx_() const;
        const uint32_t getPerclientOpcnt_() const;
        const uint32_t getPerclientWorkercnt_() const;
        // For all roles
        const uint32_t getKeycnt_() const;
        const std::string getWorkloadName_() const;
        const std::string getWorkloadUsageRole_() const;

        // (2) Other common utilities

        bool needAllTraceFiles_() const;
        bool needDatasetItems_() const;
        bool needWorkloadItems_() const;

        void checkIsValid_() const;
    };
}

#endif
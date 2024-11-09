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

#include <deque>
#include <random> // std::mt19937_64, std::discrete_distribution
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

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
        
        // NOTE: add workload_randombase to introduce variances for single-node simulator -> yet NOT affect previous evaluation results of other experiments due to default randomness = 0!!!

        static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase = 0);
        // static WorkloadWrapperBase* getWorkloadGeneratorByWorkloadName(const uint64_t& capacity_bytes, const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role); // (OBSOLETE due to already checking objsize in LocalCacheBase)

        WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase);
        virtual ~WorkloadWrapperBase();

        // Access by the single thread of client wrapper (NO need to be thread safe)
        void validate(); // Provide individual validate() as contructor cannot invoke virtual functions

        // Access by multiple client workers (thread safe)
        WorkloadItem generateWorkloadItem(const uint32_t& local_client_worker_idx, bool* is_dynamic_mapped_ptr = nullptr);
        virtual uint32_t getPracticalKeycnt() const = 0;
        virtual WorkloadItem getDatasetItem(const uint32_t itemidx) = 0; // Get a dataset key-value pair item with the index of itemidx

        // Access by single thread of client wrapper, yet contend with multiple client workers for dynamic rules (thread safe)
        void updateDynamicRules(); // Update dynamic rules for dynamic workload patterns

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
        void prepareForDynamicPatterns_(); // Prepare for dynamic workload patterns (mainly prepare randgen and dist for random patterns)
        void initDynamicRules_(); // Initialize dynamic rules for dynamic workload patterns

        // Access by multiple client workers (thread safe)
        virtual WorkloadItem generateWorkloadItem_(const uint32_t& local_client_worker_idx) = 0;
        void applyDynamicRules_(const uint32_t& local_client_worker_idx, WorkloadItem& workload_item, bool* is_dynamic_mapped_ptr); // Try to apply dynamic rules for dynamic workload patterns in generateWorkloadItem() after generateWorkloadItem_()

        // Access by single thread of client wrapper, yet contend with multiple client workers for dynamic rules (thread safe)
        void updateDynamicRulesForHotin_();
        void updateDynamicRulesForHotout_();
        void updateDynamicRulesForRandom_();

        // Utility functions for dynamic workload patterns
        virtual uint32_t getLargestRank_(const uint32_t local_client_worker_idx) const = 0;
        virtual void getRankedKeys_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) const = 0; // Get keys ranked in [start_rank, start_rank + ranked_keycnt - 1] within [0, largest_rank] for dynamic workload patterns

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
        const uint32_t workload_randombase_;
        // For dynamic workload patterns
        const std::string workload_pattern_name_;
        const uint32_t dynamic_change_period_;
        const uint32_t dynamic_change_keycnt_;

        // To generate random indexes in [0, dynamic_rulecnt - 1] and hence corresponding original keys for dynamic workload patterns
        std::vector<std::mt19937_64*> curclient_perworker_dynamic_randgen_ptrs_; // Random generators to get random original keys ranked in [0, dynamic_rulecnt - 1] (used for dynamic workload patterns)
        std::vector<std::uniform_int_distribution<uint32_t>*> curclient_perworker_dynamic_dist_ptrs_; // Uniform distributions to get random original keys ranked in [0, dynamic_rulecnt - 1] (used for dynamic workload patterns)

        // Dynamic rules for dynamic workload patterns
        typedef std::deque<std::string> dynamic_rules_mapped_keys_t;
        typedef std::unordered_map<std::string, dynamic_rules_mapped_keys_t::iterator> dynamic_rules_lookup_map_t;
        boost::shared_mutex dynamic_rwlock_; // Ensure thread safety for dynamic rules (read by client workers of current client; written by client wrapper of current client)
        uint32_t dynamic_period_idx_;
        std::vector<std::vector<dynamic_rules_lookup_map_t::iterator>> curclient_perworker_dynamic_rules_rankptrs_; // Rank pointers (rank -> original keys' pointers in lookup map) of dynamic rules for each client worker in current client (used for dynamic workload patterns) (NOTE: NEVER changed after initialization)
        std::vector<dynamic_rules_lookup_map_t> curclient_perworker_dynamic_rules_lookup_map_; // Lookup maps (original keys -> mapped keys' pointers) of dynamic rules for each client worker in current client (used for dynamic workload patterns) (NOTE: keys are NEVER changed after initialization, yet values will be changed when updating dynamic rules)
        std::vector<dynamic_rules_mapped_keys_t> curclient_perworker_dynamic_rules_mapped_keys_; // Mapped keys of dynamic rules for each client worker in current client (used for dynamic workload patterns) (NOTE: partially changed when updating dynamic rules)
    protected:
        // Utility functions for dynamic workload patterns
        void checkDynamicPatterns_() const;
        void getRankedIdxes_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<uint32_t>& ranked_idxes) const; // Get indexes of [start_rank, start_rank + ranked_keycnt - 1] within [0, largest_rank] for dynamic workload patterns
        void getRandomIdxes_(const uint32_t local_client_worker_idx, const uint32_t random_keycnt, std::vector<uint32_t>& random_idxes) const; // Get random_keycnt random indexes within [0, dynamic_rulecnt - 1] for dynamic workload patterns

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
        const uint32_t getWorkloadRandombase_() const;
        // For dynamic workload patterns
        const std::string getWorkloadPatternName_() const;
        const uint32_t getDynamicChangePeriod_() const;
        const uint32_t getDynamicChangeKeycnt_() const;

        // (2) Other common utilities

        bool needAllTraceFiles_() const; // Whether need to preprocess all single-node trace files to sample workload itesm for partition-based geo-distributed accesses and dataset items for dataset loader & cloud
        bool needDatasetItems_() const;
        bool needWorkloadItems_() const;

        void checkIsValid_() const;
    };
}

#endif
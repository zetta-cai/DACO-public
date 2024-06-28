#include "workload/akamai_workload_wrapper.h"

#include <assert.h>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string AkamaiWorkloadWrapper::kClassName("AkamaiWorkloadWrapper");

    AkamaiWorkloadWrapper::AkamaiWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role)
    {
        UNUSED(perclient_opcnt); // NO need to pre-generate workload items for approximate workload distribution, as Akamai traces have workload files

        // NOTE: Akamai workloads are not replayed single-node traces and NO need to preprocess all single-node trace files (also NO such files) to sample workload and dataset items
        assert(!needAllTraceFiles_()); // Must NOT trace preprocessor

        // Differentiate Akamai workload wrapper in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        instance_name_ = oss.str();

        // For clients, dataset loader, and cloud
        average_dataset_keysize_ = 0;
        min_dataset_keysize_ = 0;
        max_dataset_keysize_ = 0;
        average_dataset_valuesize_ = 0;
        min_dataset_valuesize_ = 0;
        max_dataset_valuesize_ = 0;
        dataset_valsizes_.clear();
        // TODO: END HERE
        loadDatasetFile_(); // Load dataset file to update dataset_valsizes_ and dataset statistics (required by all roles including clients, dataset loader, and cloud); clients use value sizes to generate workload items (yet not used due to GET requests)

        // For clients
        curclient_perworker_workload_objids_.resize(perclient_workercnt, std::vector<int64_t>());
        curclient_perworker_workloadidx_.resize(perclient_workercnt, 0);
    }

    AkamaiWorkloadWrapper::~AkamaiWorkloadWrapper()
    {
        // For clients, dataset loader, and cloud

        // Nothing to release
        return;
    }

    WorkloadItem AkamaiWorkloadWrapper::generateWorkloadItem(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needWorkloadItems_()); // Must be clients for evaluation

        // Get the workload index
        assert(local_client_worker_idx < curclient_perworker_workloadidx_.size());
        const uint32_t tmp_workload_idx = curclient_perworker_workloadidx_[local_client_worker_idx];

        // Get the workload sequence
        assert(local_client_worker_idx < curclient_perworker_workload_objids_.size());
        const std::vector<int64_t>& tmp_workload_objids_const_ref = curclient_perworker_workload_objids_[local_client_worker_idx];
        const uint32_t tmp_workload_objids_cnt = tmp_workload_objids_const_ref.size();

        // Get object ID and hence key
        assert(tmp_workload_idx >= 0 && tmp_workload_idx < tmp_workload_objids_cnt);
        const int64_t tmp_objid = tmp_workload_objids_const_ref[tmp_workload_idx];
        Key tmp_key = getKeyFromObjid_(tmp_objid);

        // Get value indexed by object ID
        assert(tmp_objid >= 0 && tmp_objid <= dataset_valsizes_.size());
        const uint32_t tmp_valsize = dataset_valsizes_[tmp_objid];
        Value tmp_value(tmp_valsize);

        // Update the workload index
        const uint32_t tmp_next_workload_idx = (tmp_workload_idx + 1) % tmp_workload_objids_cnt;
        curclient_perworker_workloadidx_[local_client_worker_idx] = tmp_next_workload_idx;
        
        return WorkloadItem(tmp_key, tmp_value, WorkloadItemType::kWorkloadItemGet); // TRAGEN-generated Akamai workload is read-only
    }

    uint32_t AkamaiWorkloadWrapper::getPracticalKeycnt() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return dataset_valsizes_.size();
    }

    WorkloadItem AkamaiWorkloadWrapper::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud
        assert(itemidx < getPracticalKeycnt());

        // NOTE: itemidx equals with the object ID
        Key tmp_key = getKeyFromObjid_(itemidx);
        const uint32_t tmp_valsize = dataset_valsizes_[itemidx];
        Value tmp_value(tmp_valsize);

        return WorkloadItem(tmp_key, tmp_value, WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double AkamaiWorkloadWrapper::getAvgDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_keysize_;
    }
    
    double AkamaiWorkloadWrapper::getAvgDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_valuesize_;
    }

    uint32_t AkamaiWorkloadWrapper::getMinDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_keysize_;
    }

    uint32_t AkamaiWorkloadWrapper::getMinDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_valuesize_;
    }

    uint32_t AkamaiWorkloadWrapper::getMaxDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_keysize_;
    }

    uint32_t AkamaiWorkloadWrapper::getMaxDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_valuesize_;
    }

    // For warmup speedup

    void AkamaiWorkloadWrapper::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get object ID (i.e., the index of dataset values)
        const int64_t tmp_objid = getObjidFromKey_(key);

        // Get value indexed by the object ID
        assert(tmp_objid < dataset_valsizes_.size()); // Should not access an unexisting key during warmup
        const uint32_t value_size = dataset_valsizes_[tmp_objid];
        value = Value(value_size);

        return;
    }

    void AkamaiWorkloadWrapper::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get object ID (i.e., the index of dataset values)
        const int64_t tmp_objid = getObjidFromKey_(key);

        // Update value indexed by the object ID
        assert(tmp_objid < dataset_valsizes_.size()); // Should not access an unexisting key during warmup
        dataset_valsizes_[tmp_objid] = value.getValuesize();

        return;
    }

    void AkamaiWorkloadWrapper::quickDatasetDel(const Key& key)
    {
        quickDatasetPut(key, Value()); // Use default value with is_deleted = true and value size = 0 as delete operation

        return;
    }

    Key AkamaiWorkloadWrapper::getKeyFromObjid_(const int64_t& objid)
    {
        assert(objid >= 0 && objid < dataset_valsizes_.size());
        const uint32_t objid_bytecnt = sizeof(int64_t); // 8B

        // Get the keystr (8B)
        std::string tmp_keystr((const char*)&objid, objid_bytecnt); // 8B

        return tmp_keystr;
    }

    int64_t AkamaiWorkloadWrapper::getObjidFromKey_(const Key& key)
    {
        const uint32_t objid_bytecnt = sizeof(int64_t); // 8B
        assert(key.getKeyLength() == objid_bytecnt);

        // Get the object ID (8B)
        int64_t tmp_objid = 0;
        memcpy((char *)&tmp_objid, key.getKeystr().c_str(), objid_bytecnt);
        assert(tmp_objid >= 0);

        return tmp_objid;
    }

    void AkamaiWorkloadWrapper::initWorkloadParameters_()
    {
        if (needWorkloadItems_()) // Clients
        {
            const uint32_t tmp_keycnt = getKeycnt_();
            const std::string tmp_workload_name = getWorkloadName_();
            const std::string tmp_trace_dirpath = Config::getTraceDirpath();
            const uint32_t tmp_perclient_workercnt = getPerclientWorkercnt_();
            const uint32_t tmp_clietcnt = getClientcnt_();
            const uint32_t tmp_total_workercnt = tmp_clietcnt * tmp_perclient_workercnt;

            // Load workload file for each client
            for (uint32_t tmp_local_client_worker_idx = 0; tmp_local_client_worker_idx < tmp_perclient_workercnt; tmp_local_client_worker_idx++)
            {
                // Get global client worker index
                uint32_t tmp_global_client_worker_idx = Util::getGlobalClientWorkerIdx(getClientIdx_(), tmp_local_client_worker_idx, tmp_perclient_workercnt);

                // Get workload filepath based on global client worker index
                std::ostringstream oss;
                oss << tmp_trace_dirpath << "/" << tmp_workload_name << "/dataset" << tmp_keycnt << "_workercnt" << tmp_total_workercnt << "/"; // E.g., data/akamaiweb/dataset1000000_workercnt4/
                oss << "worker" << tmp_global_client_worker_idx << "_sequence.txt"; // E.g., worker0_sequence.txt
                const std::string tmp_workload_filepath = oss.str();

                // Load workload items into curclient_perworker_workload_objids_[tmp_local_client_worker_idx]
                loadWorkloadFile_(tmp_workload_filepath, tmp_local_client_worker_idx);

                // Start from the first workload item
                curclient_perworker_workloadidx_[tmp_local_client_worker_idx] = 0;
            }
        }

        return;
    }

    void AkamaiWorkloadWrapper::overwriteWorkloadParameters_()
    {
        // Do nothing

        return;
    }

    void AkamaiWorkloadWrapper::createWorkloadGenerator_()
    {
        // NOT need pre-generated workload items for approximate workload distribution due to directly loadings workload items from workload files

        return;
    }

    // (1) Akamai-specific helper functions

    // For role of clients, dataset loader, and cloud
    void AkamaiWorkloadWrapper::loadDatasetFile_(const std::string& dataset_filepath)
    {}

    // For role of clients
    void AkamaiWorkloadWrapper::loadWorkloadFile_(const std::string& workload_filepath, const uint32_t& local_client_worker_index)
    {}

    // (2) Common utilities

    void AkamaiWorkloadWrapper::checkPointers_() const
    {
        // Nothing to check
        return;
    }
}

#include "workload/akamai_workload_wrapper.h"

#include <assert.h>
#include <unordered_set>

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string AkamaiWorkloadWrapper::kClassName("AkamaiWorkloadWrapper");
    const uint32_t AkamaiWorkloadWrapper::TRAGEN_VALSIZE_UNIT = 1024; // 1KiB

    AkamaiWorkloadWrapper::AkamaiWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase)
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
        loadDatasetFile_(); // Load dataset file to update dataset_valsizes_ and dataset statistics (required by all roles including clients, dataset loader, and cloud); clients use value sizes to generate workload items (yet not used due to GET requests))

        // For clients
        curclient_perworker_workload_objids_.resize(perclient_workercnt, std::vector<int64_t>());
        curclient_perworker_workloadidx_.resize(perclient_workercnt, 0);
        curclient_perworker_ranked_unique_objids_.resize(perclient_workercnt, std::vector<int64_t>());
    }

    AkamaiWorkloadWrapper::~AkamaiWorkloadWrapper()
    {
        // For clients, dataset loader, and cloud

        // Do nothing
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

    void AkamaiWorkloadWrapper::initWorkloadParameters_()
    {
        if (needWorkloadItems_()) // Clients
        {
            const uint32_t tmp_perclient_workercnt = getPerclientWorkercnt_();

            // Load workload file for each client
            for (uint32_t tmp_local_client_worker_idx = 0; tmp_local_client_worker_idx < tmp_perclient_workercnt; tmp_local_client_worker_idx++)
            {
                // Load workload items into curclient_perworker_workload_objids_[tmp_local_client_worker_idx]
                // Also update ranked objids into curclient_perworker_ranked_unique_objids_[tmp_local_client_worker_idx] (used for dynamic workload patterns)
                loadWorkloadFile_(tmp_local_client_worker_idx);

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

    // Access by multiple client workers (thread safe)

    WorkloadItem AkamaiWorkloadWrapper::generateWorkloadItem_(const uint32_t& local_client_worker_idx)
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

    // Utility functions for dynamic workload patterns

    uint32_t AkamaiWorkloadWrapper::getLargestRank_(const uint32_t local_client_worker_idx) const
    {
        checkDynamicPatterns_();

        assert(local_client_worker_idx < curclient_perworker_ranked_unique_objids_.size());
        const std::vector<int64_t>& tmp_ranked_unique_objids_const_ref = curclient_perworker_ranked_unique_objids_[local_client_worker_idx];
        return tmp_ranked_unique_objids_const_ref.size() - 1;
    }
    
    void AkamaiWorkloadWrapper::getRankedKeys_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) const
    {
        checkDynamicPatterns_();

        // Get ranked indexes
        std::vector<uint32_t> tmp_ranked_idxes;
        getRankedIdxes_(local_client_worker_idx, start_rank, ranked_keycnt, tmp_ranked_idxes);

        // Get the const reference of current client worker's ranked objids
        assert(local_client_worker_idx < curclient_perworker_ranked_unique_objids_.size());
        const std::vector<int64_t>& tmp_ranked_unique_objids_const_ref = curclient_perworker_ranked_unique_objids_[local_client_worker_idx];

        // Set ranked keys based on the ranked indexes
        ranked_keys.clear();
        for (int i = 0; i < tmp_ranked_idxes.size(); i++)
        {
            const uint32_t tmp_ranked_objid_idx = tmp_ranked_idxes[i];
            const int64_t tmp_ranked_objid = tmp_ranked_unique_objids_const_ref[tmp_ranked_objid_idx];
            Key tmp_key = getKeyFromObjid_(tmp_ranked_objid);
            ranked_keys.push_back(tmp_key.getKeystr());
        }

        return;
    }

    // (1) Akamai-specific helper functions

    // For role of clients, dataset loader, and cloud
    void AkamaiWorkloadWrapper::loadDatasetFile_()
    {
        // Get dataset filepath
        const std::string tmp_dataset_filepath = getDatasetFilepath_();

        // Check existance of dataset file
        if (!Util::isFileExist(tmp_dataset_filepath, true))
        {
            std::ostringstream oss;
            oss << "failed to find the dataset file " << tmp_dataset_filepath << "!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Read dataset file line by line
        std::fstream* fs_ptr = Util::openFile(tmp_dataset_filepath, std::ios_base::in);
        assert(fs_ptr != NULL);
        std::string cur_line = "";
        while (true)
        {
            // Read the current line
            // NOTE: NO need to use mmap and traditional read/write or even direct I/O is sufficient, as we ONLY need to enumerate the dataset file once!
            std::getline(*fs_ptr, cur_line);

            size_t delim_pos = cur_line.find(",");
            if (delim_pos != std::string::npos) // NOTE: avoid empty line
            {
                // Load object ID and value size (format: objid,valsize)
                const std::string objid_str = cur_line.substr(0, delim_pos);
                int64_t objid = std::stoll(objid_str);
                assert(objid >= 0);
                const std::string valsize_str = cur_line.substr(delim_pos + 1, cur_line.size() - delim_pos - 1);
                uint32_t valsize = static_cast<uint32_t>(std::stoul(valsize_str)) * TRAGEN_VALSIZE_UNIT; // NOTE: the unit of object sizes in Akamai's traces is KiB instead of bytes

                // Update dataset_valsizes_
                // NOTE: NO need to track object IDs, which are the indexes of dataset_valsizes_
                dataset_valsizes_.push_back(valsize);
            }

            // Check if achieving the end of the file
            if (fs_ptr->eof())
            {
                break;
            }
        }

        // Close file and release ifstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        // Update dataset statistics
        average_dataset_keysize_ = sizeof(int64_t); // 8B
        min_dataset_keysize_ = sizeof(int64_t); // 8B
        max_dataset_keysize_ = sizeof(int64_t); // 8B
        for (uint32_t i = 0; i < dataset_valsizes_.size(); i++)
        {
            uint32_t tmp_valsize = dataset_valsizes_[i];
            average_dataset_valuesize_ += tmp_valsize;

            if (i == 0 || tmp_valsize < min_dataset_valuesize_)
            {
                min_dataset_valuesize_ = tmp_valsize;
            }

            if (i == 0 || tmp_valsize > max_dataset_valuesize_)
            {
                max_dataset_valuesize_ = tmp_valsize;
            }
        }
        average_dataset_valuesize_ /= dataset_valsizes_.size();

        // Dump information
        std::ostringstream oss;
        oss << "load dataset file " << tmp_dataset_filepath << " with " << dataset_valsizes_.size() << " unique objects";
        // oss << " (dataset_valsizes_[0]/[1]: " << dataset_valsizes_[0] << "/" << dataset_valsizes_[1] << ")"; // Debug information
        Util::dumpNormalMsg(instance_name_, oss.str());
        
        return;
    }
    
    std::string AkamaiWorkloadWrapper::getDatasetFilepath_() const
    {
        const uint32_t tmp_keycnt = getKeycnt_();
        const std::string tmp_workload_name = getWorkloadName_();
        const std::string tmp_trace_dirpath = Config::getTraceDirpath();

        // Get dataset filepath
        std::ostringstream oss;
        oss << tmp_trace_dirpath << "/" << tmp_workload_name << "/dataset" << tmp_keycnt << ".txt"; // E.g., data/akamaiweb/dataset1000000.txt
        const std::string tmp_dataset_filepath = oss.str();

        return tmp_dataset_filepath;
    }

    // For role of clients
    void AkamaiWorkloadWrapper::loadWorkloadFile_(const uint32_t& local_client_worker_index)
    {
        // Get workload filepath based on global client worker index
        const std::string tmp_workload_filepath = getWorkloadFilepath_(local_client_worker_index);

        // Check existance of workload file
        if (!Util::isFileExist(tmp_workload_filepath, true))
        {
            std::ostringstream oss;
            oss << "failed to find the workload file " << tmp_workload_filepath << "!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        std::vector<int64_t>& curclient_curworker_workload_objids_ref = curclient_perworker_workload_objids_[local_client_worker_index];

        // Count objid-freq map (used for dynamic workload patterns)
        std::unordered_map<int64_t, uint32_t> tmp_freqmap;

        // Read workload file line by line
        std::fstream* fs_ptr = Util::openFile(tmp_workload_filepath, std::ios_base::in);
        assert(fs_ptr != NULL);
        std::string cur_line = "";
        while (true)
        {
            // Read the current line
            // NOTE: NO need to use mmap and traditional read/write or even direct I/O is sufficient, as we ONLY need to enumerate the workload file once!
            std::getline(*fs_ptr, cur_line);

            size_t first_delim_pos = cur_line.find(",");
            if (first_delim_pos != std::string::npos) // NOTE: avoid empty line
            {
                // Load object ID of the current request (format: timestamp,objid,valsize)
                size_t second_delim_pos = cur_line.find(",", first_delim_pos + 1);
                const std::string objid_str = cur_line.substr(first_delim_pos + 1, second_delim_pos - first_delim_pos - 1);
                int64_t objid = std::stoll(objid_str);
                assert(objid >= 0);

                // Update workload objid of the local client worker in current client
                curclient_curworker_workload_objids_ref.push_back(objid);  

                // Update obj-freq map
                std::unordered_map<int64_t, uint32_t>::iterator tmp_freqmap_iter = tmp_freqmap.find(objid);
                if (tmp_freqmap_iter == tmp_freqmap.end())
                {
                    tmp_freqmap[objid] = 1;
                }
                else
                {
                    tmp_freqmap[objid]++;
                }
            }

            // Check if achieving the end of the file
            if (fs_ptr->eof())
            {
                break;
            }
        }

        // Get freq-objid sorted map (sorted by freq in a descending order)
        std::multimap<uint32_t, int64_t, std::greater<uint32_t>> tmp_freqmap_sorted;
        for (std::unordered_map<int64_t, uint32_t>::iterator tmp_freqmap_iter = tmp_freqmap.begin(); tmp_freqmap_iter != tmp_freqmap.end(); tmp_freqmap_iter++)
        {
            tmp_freqmap_sorted.insert(std::pair<uint32_t, int64_t>(tmp_freqmap_iter->second, tmp_freqmap_iter->first));
        }

        // Update ranked objids of the local client worker in current client (used for dynamic workload patterns)
        std::vector<int64_t>& curclient_curworker_ranked_objids_ref = curclient_perworker_ranked_unique_objids_[local_client_worker_index];
        curclient_curworker_ranked_objids_ref.clear(); // Clear for safety
        for (std::multimap<uint32_t, int64_t>::iterator tmp_freqmap_sorted_iter = tmp_freqmap_sorted.begin(); tmp_freqmap_sorted_iter != tmp_freqmap_sorted.end(); tmp_freqmap_sorted_iter++)
        {
            curclient_curworker_ranked_objids_ref.push_back(tmp_freqmap_sorted_iter->second);
        }

        // Close file and release ifstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        // Dump information
        std::ostringstream oss;
        oss << "load workload file " << tmp_workload_filepath << "for local client worker " << local_client_worker_index << " with " << curclient_curworker_workload_objids_ref.size() << " requests";
        // oss << " (curclient_curworker_workload_objids_ref[0]/[1]: " << curclient_curworker_workload_objids_ref[0] << "/" << curclient_curworker_workload_objids_ref[1] << ")"; // Debug information
        Util::dumpNormalMsg(instance_name_, oss.str());
        
        return;
    }

    std::string AkamaiWorkloadWrapper::getWorkloadFilepath_(const uint32_t& local_client_worker_index) const
    {
        const uint32_t tmp_perclient_workercnt = getPerclientWorkercnt_();
        const uint32_t tmp_keycnt = getKeycnt_();
        const std::string tmp_workload_name = getWorkloadName_();
        const std::string tmp_trace_dirpath = Config::getTraceDirpath();
        const uint32_t tmp_clietcnt = getClientcnt_();
        const uint32_t tmp_total_workercnt = tmp_clietcnt * tmp_perclient_workercnt;

        // Get global client worker index
        uint32_t tmp_global_client_worker_idx = Util::getGlobalClientWorkerIdx(getClientIdx_(), local_client_worker_index, tmp_perclient_workercnt);

        // Get workload filepath based on global client worker index
        std::ostringstream oss;
        oss << tmp_trace_dirpath << "/" << tmp_workload_name << "/dataset" << tmp_keycnt << "_workercnt" << tmp_total_workercnt << "/"; // E.g., data/akamaiweb/dataset1000000_workercnt4/
        oss << "worker" << tmp_global_client_worker_idx << "_sequence.txt"; // E.g., worker0_sequence.txt
        const std::string tmp_workload_filepath = oss.str();

        return tmp_workload_filepath;
    }

    // (2) Common utilities

    void AkamaiWorkloadWrapper::checkPointers_() const
    {
        // Nothing to check
        return;
    }

    Key AkamaiWorkloadWrapper::getKeyFromObjid_(const int64_t& objid) const
    {
        assert(objid >= 0 && objid < dataset_valsizes_.size());
        const uint32_t objid_bytecnt = sizeof(int64_t); // 8B

        // Get the keystr (8B)
        std::string tmp_keystr((const char*)&objid, objid_bytecnt); // 8B

        return Key(tmp_keystr);
    }

    int64_t AkamaiWorkloadWrapper::getObjidFromKey_(const Key& key) const
    {
        const uint32_t objid_bytecnt = sizeof(int64_t); // 8B
        assert(key.getKeyLength() == objid_bytecnt);

        // Get the object ID (8B)
        int64_t tmp_objid = 0;
        memcpy((char *)&tmp_objid, key.getKeystr().c_str(), objid_bytecnt);
        assert(tmp_objid >= 0 && tmp_objid < dataset_valsizes_.size());

        return tmp_objid;
    }
}

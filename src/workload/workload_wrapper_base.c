#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"
#include "workload/wikipedia_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_PREPROCESSOR("preprocessor");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_LOADER("loader");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLIENT("client");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD("cloud");

    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == Util::FACEBOOK_WORKLOAD_NAME) // Facebook/Meta CDN
        {
            workload_ptr = new FacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, max_eval_workload_loadcnt);
        }
        else if (workload_name == Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::WIKIPEDIA_TEXT_WORKLOAD_NAME) // Wiki image/text CDN
        {
            workload_ptr = new WikipediaWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, max_eval_workload_loadcnt);
        }
        else
        {
            std::ostringstream oss;
            oss << "workload " << workload_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(workload_ptr != NULL);
        workload_ptr->validate(); // validate workload before generating each request

        return workload_ptr;
    }

    // (OBSOLETE due to already checking objsize in LocalCacheBase)
    // WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint64_t& capacity_bytes, const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt)
    // {
    //     WorkloadWrapperBase* workload_ptr = getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, max_eval_workload_loadcnt);
    //     assert(workload_ptr != NULL);

    //     // NOTE: cache capacity MUST be larger than the maximum object size in the workload
    //     const uint32_t max_obj_size = workload_ptr->getMaxDatasetKeysize() + workload_ptr->getMaxDatasetValuesize();
    //     if (capacity_bytes <= max_obj_size)
    //     {
    //         std::ostringstream oss;
    //         oss << "cache capacity (" << capacity_bytes << " bytes) should > the maximum object size (" << max_obj_size << " bytes) in workload " << workload_name << "!";
    //         Util::dumpErrorMsg(kClassName, oss.str());
    //         exit(1);
    //     }

    //     return workload_ptr;
    // }

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt) : clientcnt_(clientcnt), client_idx_(client_idx), perclient_opcnt_(perclient_opcnt), perclient_workercnt_(perclient_workercnt), max_eval_workload_loadcnt_(max_eval_workload_loadcnt), keycnt_(keycnt), workload_name_(workload_name), workload_usage_role_(workload_usage_role)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        is_valid_ = false;

        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        average_dataset_keysize_ = 0;
        average_dataset_valuesize_ = 0;
        min_dataset_keysize_ = 0;
        min_dataset_valuesize_ = 0;
        max_dataset_keysize_ = 0;
        max_dataset_valuesize_ = 0;
        dataset_lookup_table_.clear();
        dataset_kvpairs_.clear();

        // Verify workload usage role
        if (workload_usage_role == WORKLOAD_USAGE_ROLE_PREPROCESSOR)
        {
            assert(Util::isReplayedWorkload(workload_name)); // ONLY replayed traces need preprocessing
        }
        else if (workload_usage_role == WORKLOAD_USAGE_ROLE_LOADER || workload_usage_role == WORKLOAD_USAGE_ROLE_CLIENT || workload_usage_role == WORKLOAD_USAGE_ROLE_CLOUD)
        {
            // Do nothing
        }
        else
        {
            oss.clear();
            oss.str("");
            oss << "invalid workload usage role: " << workload_usage_role;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
    }

    WorkloadWrapperBase::~WorkloadWrapperBase() {}

    void WorkloadWrapperBase::validate()
    {
        if (!is_valid_)
        {
            std::ostringstream oss;
            oss << "validate workload wrapper...";
            Util::dumpNormalMsg(base_instance_name_, oss.str());

            initWorkloadParameters_();
            overwriteWorkloadParameters_();
            createWorkloadGenerator_();

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(base_instance_name_, "duplicate invoke of validate()!");
        }
        return;
    }

    // Getters for const shared variables coming from Param

    const uint32_t WorkloadWrapperBase::getClientcnt_() const
    {
        // ONLY for clients
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT);

        assert(clientcnt_ > 0);
        
        return clientcnt_;
    }

    const uint32_t WorkloadWrapperBase::getClientIdx_() const
    {
        // ONLY for clients
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT);

        assert(client_idx_ >= 0);
        assert(client_idx_ < clientcnt_);
        
        return client_idx_;
    }

    const uint32_t WorkloadWrapperBase::getPerclientOpcnt_() const
    {
        // ONLY for clients
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT);

        // NOTE: perclient_opcnt MUST > 0, as loading/evaluation/warmup phase occurs after trace preprocessing
        assert(perclient_opcnt_ > 0);
        
        return perclient_opcnt_;
    }

    const uint32_t WorkloadWrapperBase::getPerclientWorkercnt_() const
    {
        // ONLY for clients
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT);

        assert(perclient_workercnt_ > 0);
        
        return perclient_workercnt_;
    }

    const uint32_t WorkloadWrapperBase::getMaxEvalWorkloadLoadcnt_() const
    {
        // ONLY for clients
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT);

        assert(max_eval_workload_loadcnt_ > 0);
        
        return max_eval_workload_loadcnt_;
    }

    const uint32_t WorkloadWrapperBase::getKeycnt_() const
    {
        // For all roles
        if (needAllTraceFiles_()) // Trace preprocessor
        {
            assert(keycnt_ == 0);
        }
        else // Dataset loader, clients, and cloud
        {
            // NOTE: keycnt MUST > 0, as loading/evaluation/warmup phase occurs after trace preprocessing
            assert(keycnt_ > 0);
        }

        return keycnt_;
    }

    const std::string WorkloadWrapperBase::getWorkloadName_() const
    {
        // For all roles
        return workload_name_;
    }

    const std::string WorkloadWrapperBase::getWorkloadUsageRole_() const
    {
        // For all roles
        return workload_usage_role_;
    }

    // (1) ONLY for replayed traces, which have dataset file dumped by trace preprocessor

    // (1.1) For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
    
    const double WorkloadWrapperBase::getAverageDatasetKeysize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(average_dataset_keysize_ >= 0);

        return average_dataset_keysize_;
    }

    const double WorkloadWrapperBase::getAverageDatasetValuesize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(average_dataset_valuesize_ >= 0);

        return average_dataset_valuesize_;
    }

    const uint32_t WorkloadWrapperBase::getMinDatasetKeysize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(min_dataset_keysize_ >= 0);

        return min_dataset_keysize_;
    }

    const uint32_t WorkloadWrapperBase::getMinDatasetValuesize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(min_dataset_valuesize_ >= 0);

        return min_dataset_valuesize_;
    }

    const uint32_t WorkloadWrapperBase::getMaxDatasetKeysize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(max_dataset_keysize_ >= 0);

        return max_dataset_keysize_;
    }

    const uint32_t WorkloadWrapperBase::getMaxDatasetValuesize_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(max_dataset_valuesize_ >= 0);

        return max_dataset_valuesize_;
    }

    std::unordered_map<Key, uint32_t, KeyHasher>& WorkloadWrapperBase::getDatasetLookupTableRef_()
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(dataset_lookup_table_.size() > 0);

        return dataset_lookup_table_;
    }

    std::vector<std::pair<Key, Value>>& WorkloadWrapperBase::getDatasetKvpairsRef_()
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(dataset_kvpairs_.size() > 0);

        return dataset_kvpairs_;
    }

    const std::vector<std::pair<Key, Value>>& WorkloadWrapperBase::getDatasetKvpairsConstRef_() const
    {
        // For role of preprocessor, dataset loader, and cloud (ONLY for replayed traces)
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needDatasetItems_());

        assert(dataset_kvpairs_.size() > 0);

        return dataset_kvpairs_;
    }

    // (1.2) For role of preprocessor (ONLY for replayed traces)

    void WorkloadWrapperBase::verifyDatasetFileForPreprocessor_()
    {
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needAllTraceFiles_()); // Trace preprocessor

        // Check if trace dirpath exists
        const std::string tmp_dirpath = Config::getTraceDirpath();
        bool is_exist = Util::isDirectoryExist(tmp_dirpath, true);
        if (!is_exist)
        {
            std::ostringstream oss;
            oss << "trace directory " << tmp_dirpath << " does not exist!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Check if dataset filepath exists
        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);
        is_exist = Util::isFileExist(tmp_dataset_filepath, true);
        if (is_exist)
        {
            std::ostringstream oss;
            oss << "dataset file " << tmp_dataset_filepath << " already exists -> please delete it before trace preprocessing!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    uint32_t WorkloadWrapperBase::dumpDatasetFile_() const
    {
        assert(Util::isReplayedWorkload(workload_name_));
        assert(needAllTraceFiles_()); // Trace preprocessor

        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);
        assert(!Util::isFileExist(tmp_dataset_filepath, true)); // Must NOT exist (already verified by verifyDatasetFile_() before)

        // Create and open a binary file for dumping dataset by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one dataset file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_dataset_filepath << " for dumping dataset of " << workload_name_;
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_dataset_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump key-value pairs of dataset
        // Format: dataset size, key, value, key, value, ...
        uint32_t size = 0;
        // (0) dataset size
        const uint32_t dataset_size = dataset_kvpairs_.size();
        assert(dataset_size > 0);
        fs_ptr->write((const char*)&dataset_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-value pairs
        const bool is_value_space_efficient = true; // NOT serialize value content
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            // Key
            const Key& tmp_key = dataset_kvpairs_[i].first;
            DynamicArray tmp_dynamic_array_for_key(tmp_key.getKeyPayloadSize());
            const uint32_t key_serialize_size = tmp_key.serialize(tmp_dynamic_array_for_key, 0);
            tmp_dynamic_array_for_key.writeBinaryFile(0, fs_ptr, key_serialize_size);
            size += key_serialize_size;

            // Value
            const Value& tmp_value = dataset_kvpairs_[i].second;
            DynamicArray tmp_dynamic_array_for_value(tmp_value.getValuePayloadSize(is_value_space_efficient));
            const uint32_t value_serialize_size = tmp_value.serialize(tmp_dynamic_array_for_value, 0, is_value_space_efficient);
            tmp_dynamic_array_for_value.writeBinaryFile(0, fs_ptr, value_serialize_size);
            size += value_serialize_size;
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (1.3) For role of dataset loader and cloud (ONLY for replayed traces)

    uint32_t WorkloadWrapperBase::loadDatasetFile_()
    {
        assert(Util::isReplayedWorkload(workload_name_));
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_LOADER || workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLOUD); // dataset loader and cloud

        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);

        bool is_exist = Util::isFileExist(tmp_dataset_filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "dataset file " << tmp_dataset_filepath << " does not exist -> please run trace_preprocessor before dataset loader and evaluation!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Open the existing binary file for total aggregated statistics
        std::fstream* fs_ptr = Util::openFile(tmp_dataset_filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Load dataset key-value pairs from dataset file
        // Format: dataset size, key, value, key, value, ...
        uint32_t size = 0;
        // (0) dataset size
        uint64_t dataset_size = 0;
        fs_ptr->read((char *)&dataset_size, sizeof(uint64_t));
        size += sizeof(uint64_t);
        dataset_kvpairs_.resize(dataset_size);
        // (1) key-value pairs
        const bool is_value_space_efficient = true; // NOT deserialize value content
        for (uint64_t i = 0; i < dataset_size; i++)
        {
            // Key
            Key tmp_key;
            uint32_t key_deserialize_size = tmp_key.deserialize(fs_ptr);
            size += key_deserialize_size;

            // Value
            Value tmp_value;
            uint32_t value_deserialize_size = tmp_value.deserialize(fs_ptr, is_value_space_efficient);
            size += value_deserialize_size;

            // Update dataset
            const uint32_t original_dataset_size = dataset_kvpairs_.size();
            dataset_kvpairs_[i] = std::pair(tmp_key, tmp_value);
            dataset_lookup_table_.insert(std::pair(tmp_key, i));

            // Update dataset statistics
            updateDatasetStatistics_(tmp_key, tmp_value, original_dataset_size);
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (1.4) For role of cloud for warmup speedup (ONLY for replayed traces)

    void WorkloadWrapperBase::quickDatasetGet_(const Key& key, Value& value) const
    {
        assert(Util::isReplayedWorkload(workload_name_));
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLOUD); // cloud

        // Check dataset lookup table
        std::unordered_map<Key, uint32_t, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
        assert(tmp_iter != dataset_lookup_table_.end()); // Key must exist

        // Get value
        const uint32_t dataset_kvpairs_index = tmp_iter->second;
        assert(dataset_kvpairs_index < dataset_kvpairs_.size());
        assert(dataset_kvpairs_[dataset_kvpairs_index].first == key);
        value = dataset_kvpairs_[dataset_kvpairs_index].second;

        return;
    }

    void WorkloadWrapperBase::quickDatasetPut_(const Key& key, const Value& value)
    {
        assert(Util::isReplayedWorkload(workload_name_));
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLOUD); // cloud

        // Check dataset lookup table
        std::unordered_map<Key, uint32_t, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
        assert(tmp_iter != dataset_lookup_table_.end()); // Key must exist

        // Put value
        const uint32_t dataset_kvpairs_index = tmp_iter->second;
        assert(dataset_kvpairs_index < dataset_kvpairs_.size());
        assert(dataset_kvpairs_[dataset_kvpairs_index].first == key);
        dataset_kvpairs_[dataset_kvpairs_index].second = value;

        return;
    }

    void WorkloadWrapperBase::quickDatasetDel_(const Key& key)
    {
        quickDatasetPut_(key, Value()); // Put a default value with is_deleted = true

        return;
    }

    // (1.5) Common utilities (ONLY for replayed traces)

    void WorkloadWrapperBase::updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size)
    {
        assert(Util::isReplayedWorkload(workload_name_));

        average_dataset_keysize_ = (average_dataset_keysize_ * original_dataset_size + key.getKeyLength()) / (original_dataset_size + 1);
        average_dataset_valuesize_ = (average_dataset_valuesize_ * original_dataset_size + value.getValuesize()) / (original_dataset_size + 1);
        if (original_dataset_size == 0) // The first kvpair
        {
            min_dataset_keysize_ = key.getKeyLength();
            min_dataset_valuesize_ = value.getValuesize();
            max_dataset_keysize_ = key.getKeyLength();
            max_dataset_valuesize_ = value.getValuesize();
        }
        else // Subsequent kvpairs
        {
            min_dataset_keysize_ = std::min(min_dataset_keysize_, key.getKeyLength());
            min_dataset_valuesize_ = std::min(min_dataset_valuesize_, value.getValuesize());
            max_dataset_keysize_ = std::max(max_dataset_keysize_, key.getKeyLength());
            max_dataset_valuesize_ = std::max(max_dataset_valuesize_, value.getValuesize());
        }

        return;
    }

    // (2) Other common utilities

    bool WorkloadWrapperBase::needAllTraceFiles_() const
    {
        // Trace preprocessor loads all trace files for dataset items and total opcnt
        if (workload_usage_role_ == WORKLOAD_USAGE_ROLE_PREPROCESSOR)
        {
            return true;
        }
        return false;
    }

    bool WorkloadWrapperBase::needDatasetItems_() const
    {
        // Trace preprocessor, dataset loader, and cloud need dataset items for preprocessing, loading, and warmup speedup
        // Trace preprocessor is from all trace files, while dataset loader and cloud are from dataset file dumped by trace preprocessor
        if (workload_usage_role_ == WORKLOAD_USAGE_ROLE_PREPROCESSOR || workload_usage_role_ == WORKLOAD_USAGE_ROLE_LOADER || workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLOUD) // ONLY need dataset items for preprocessing, loading, and warmup speedup
        {
            return true;
        }
        return false;
    }

    bool WorkloadWrapperBase::needWorkloadItems_() const
    {
        // Client needs workload items for evaluation
        if (workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT) // ONLY need workload items for evaluation
        {
            return true;
        }
        return false;
    }

    void WorkloadWrapperBase::checkIsValid_() const
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(base_instance_name_, "not invoke validate() yet!");
            exit(1);
        }
        return;
    }
}
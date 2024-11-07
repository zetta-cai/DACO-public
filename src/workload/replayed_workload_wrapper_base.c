#include "workload/replayed_workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string ReplayedWorkloadWrapperBase::kClassName("ReplayedWorkloadWrapperBase");

    const uint32_t ReplayedWorkloadWrapperBase::PHASE_FOR_WORKLOAD_SAMPLE_RATIO = 0;
    const uint32_t ReplayedWorkloadWrapperBase::PHASE_FOR_WORKLOAD_SAMPLE = 1;

    ReplayedWorkloadWrapperBase::ReplayedWorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        // For preprocessor
        total_workload_opcnt_ = 0;
        workload_sample_ratio_ = 0.0;
        sample_workload_keys_.clear();
        sample_workload_value_sizes_.clear();
        workload_sample_randgen_ptr_ = new std::mt19937(Util::DATASET_KVPAIR_SAMPLE_SEED);
        assert(workload_sample_randgen_ptr_ != NULL);
        workload_sample_dist_ptr_ = NULL;

        // For role of preprocessor, dataset loader, and cloud
        average_dataset_keysize_ = 0;
        average_dataset_valuesize_ = 0;
        min_dataset_keysize_ = 0;
        min_dataset_valuesize_ = 0;
        max_dataset_keysize_ = 0;
        max_dataset_valuesize_ = 0;
        dataset_lookup_table_.clear();
        dataset_kvpairs_.clear();

        // For clients
        curclient_workload_keys_.clear();
        curclient_workload_value_sizes_.clear();
        curclient_ranked_unique_keys_.clear();

        per_client_worker_workload_idx_.resize(perclient_workercnt);
        for (uint32_t i = 0; i < perclient_workercnt; i++)
        {
            per_client_worker_workload_idx_[i] = i;
        }

        assert(Util::isReplayedWorkload(workload_name)); // ONLY replayed traces can inherit from ReplayedWorkloadWrapperBase
    }

    ReplayedWorkloadWrapperBase::~ReplayedWorkloadWrapperBase()
    {
        // For trace preprocessor
        assert(workload_sample_randgen_ptr_ != NULL);
        delete workload_sample_randgen_ptr_;
        workload_sample_randgen_ptr_ = NULL;
        if (workload_sample_dist_ptr_ != NULL)
        {
            delete workload_sample_dist_ptr_;
            workload_sample_dist_ptr_ = NULL;
        }
    }

    uint32_t ReplayedWorkloadWrapperBase::getPracticalKeycnt() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        int64_t dataset_size = dataset_kvpairs_.size();
        return Util::toUint32(dataset_size);
    }

    WorkloadItem ReplayedWorkloadWrapperBase::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();

        assert(needDatasetItems_());
        assert(itemidx < getPracticalKeycnt());

        const Key tmp_covered_key = dataset_kvpairs_[itemidx].first;
        const Value tmp_covered_value = dataset_kvpairs_[itemidx].second;
        return WorkloadItem(tmp_covered_key, tmp_covered_value, WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double ReplayedWorkloadWrapperBase::getAvgDatasetKeysize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(average_dataset_keysize_ >= 0);

        return average_dataset_keysize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }
    
    double ReplayedWorkloadWrapperBase::getAvgDatasetValuesize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(average_dataset_valuesize_ >= 0);

        return average_dataset_valuesize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t ReplayedWorkloadWrapperBase::getMinDatasetKeysize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(min_dataset_keysize_ >= 0);

        return min_dataset_keysize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t ReplayedWorkloadWrapperBase::getMinDatasetValuesize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(min_dataset_valuesize_ >= 0);

        return min_dataset_valuesize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t ReplayedWorkloadWrapperBase::getMaxDatasetKeysize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(max_dataset_keysize_ >= 0);

        return max_dataset_keysize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }

    uint32_t ReplayedWorkloadWrapperBase::getMaxDatasetValuesize() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        assert(max_dataset_valuesize_ >= 0);

        return max_dataset_valuesize_; // NOT tracked by evaluation phase which may NOT load all trace files
    }

    // For warmup speedup

    void ReplayedWorkloadWrapperBase::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetGet_(key, value);
        return;
    }

    void ReplayedWorkloadWrapperBase::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetPut_(key, value);
        return;
    }

    void ReplayedWorkloadWrapperBase::quickDatasetDel(const Key& key)
    {
        checkIsValid_();

        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // Must be cloud for warmup speedup

        quickDatasetDel_(key);
        return;
    }

    void ReplayedWorkloadWrapperBase::initWorkloadParameters_()
    {
        if (needAllTraceFiles_()) // Need all trace files for preprocessing (preprocessor)
        {
            verifyDatasetAndWorkloadAbsenceForPreprocessor_(); // Sampled dataset and workload files should NOT exist before preprocessing

            // Phase 0 of trace preprocessing to get total opcnt of all trace files
            parseTraceFiles_(PHASE_FOR_WORKLOAD_SAMPLE_RATIO); // Will invoke updateDatasetOrSampleWorkload_() to update total_workload_opcnt_

            // Calculate workload sample ratio
            calculateWorkloadSampleRatio_(); // Will update workload_sample_ratio_ based on total_workload_opcnt_ and thus workload_sample_dist_ptr_

            parseTraceFiles_(PHASE_FOR_WORKLOAD_SAMPLE); // Will invoke updateDatasetOrSampleWorkload_() to update and sample dataset and workload items for trace preprocessor

            // Dump dataset file by trace preprocessor for dataset loader and cloud
            const uint32_t dataset_filesize = dumpDatasetFile_();
            std::ostringstream oss;
            oss << "dump dataset file (" << dataset_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(base_instance_name_, oss.str());

            // Dump sampled workload file
            const uint32_t workload_filesize = dumpWorkloadFile_();
            oss.clear();
            oss.str("");
            oss << "dump workload file (" << workload_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(base_instance_name_, oss.str());
        }
        else if (needWorkloadItems_()) // Need workload file for workload items (clients)
        {
            const uint32_t workload_filesize = loadWorkloadFile_(); // Load workload file for clients (will update curclient_workload_keys_ and curclient_workload_value_sizes_)
                
            std::ostringstream oss;
            oss << "load workload file (" << workload_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(base_instance_name_, oss.str());
        }
        else if (needDatasetItems_()) // Need dataset items for dataset loader and cloud for warmup speedup
        {
            const uint32_t dataset_filesize = loadDatasetFile_(); // Load dataset for dataset loader and cloud (will update dataset_kvpairs_, dataset_lookup_table_, and dataset statistics)

            std::ostringstream oss;
            oss << "load dataset file (" << dataset_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(base_instance_name_, oss.str());
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid workload usage role " << getWorkloadUsageRole_();
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void ReplayedWorkloadWrapperBase::overwriteWorkloadParameters_()
    {
        // NOTE: nothing to overwrite for Wikipedia workload
        return;
    }

    void ReplayedWorkloadWrapperBase::createWorkloadGenerator_()
    {
        // NOTE: nothing to create for Wikipedia workload
        return;
    }

    // Access by multiple client workers (thread safe)

    WorkloadItem ReplayedWorkloadWrapperBase::generateWorkloadItem_(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();

        assert(needWorkloadItems_()); // Must be clients for evaluation
        
        uint32_t curclient_workload_idx = per_client_worker_workload_idx_[local_client_worker_idx];
        assert(curclient_workload_idx < curclient_workload_keys_.size());

        // Get key, value, and type
        Key tmp_key = curclient_workload_keys_[curclient_workload_idx];
        Value tmp_value;
        WorkloadItemType tmp_type = WorkloadItemType::kWorkloadItemGet;
        int tmp_workload_valuesize = curclient_workload_value_sizes_[curclient_workload_idx];
        if (tmp_workload_valuesize == 0)
        {
            tmp_type = WorkloadItemType::kWorkloadItemDel;
            tmp_value = Value();
        }
        else if (tmp_workload_valuesize > 0)
        {
            tmp_type = WorkloadItemType::kWorkloadItemPut;
            tmp_value = Value(tmp_workload_valuesize);
        }

        // Update for the next workload idx
        uint32_t next_curclient_workload_idx = (curclient_workload_idx + getPerclientWorkercnt_()) % curclient_workload_keys_.size();
        per_client_worker_workload_idx_[local_client_worker_idx] = next_curclient_workload_idx;

        return WorkloadItem(tmp_key, tmp_value, tmp_type);
    }

    // Utility functions for dynamic workload patterns

    uint32_t ReplayedWorkloadWrapperBase::getLargestRank_(const uint32_t local_client_worker_idx) const
    {
        checkDynamicPatterns_();

        UNUSED(local_client_worker_idx);

        return curclient_ranked_unique_keys_.size() - 1;
    }
    
    void ReplayedWorkloadWrapperBase::getRankedKeys_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) const
    {
        checkDynamicPatterns_();

        // Get ranked indexes
        std::vector<uint32_t> tmp_ranked_idxes;
        getRankedIdxes_(local_client_worker_idx, start_rank, ranked_keycnt, tmp_ranked_idxes);

        // Set ranked keys based on the ranked indexes
        ranked_keys.clear();
        for (int i = 0; i < tmp_ranked_idxes.size(); i++)
        {
            const uint32_t tmp_ranked_keys_idx = tmp_ranked_idxes[i];
            const Key& tmp_key = curclient_ranked_unique_keys_[tmp_ranked_keys_idx];
            ranked_keys.push_back(tmp_key.getKeystr());
        }

        return;
    }

    // (1) For role of preprocessor

    void ReplayedWorkloadWrapperBase::verifyDatasetAndWorkloadAbsenceForPreprocessor_()
    {
        const std::string workload_name = getWorkloadName_();
        assert(Util::isReplayedWorkload(workload_name));
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

        // Check if sampled dataset filepath exists
        const std::string tmp_sampled_dataset_filepath = Util::getSampledDatasetFilepath(workload_name);
        is_exist = Util::isFileExist(tmp_sampled_dataset_filepath, true);
        if (is_exist)
        {
            std::ostringstream oss;
            oss << "sampled dataset file " << tmp_sampled_dataset_filepath << " already exists -> please delete sampled dataset and workload files before trace preprocessing!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Check if sampled workload filepath exists
        const std::string tmp_sampled_workload_filepath = Util::getSampledWorkloadFilepath(workload_name);
        is_exist = Util::isFileExist(tmp_sampled_workload_filepath, true);
        if (is_exist)
        {
            std::ostringstream oss;
            oss << "sampled workload file " << tmp_sampled_workload_filepath << " already exists -> please delete sampled dataset and workload files before trace preprocessing!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void ReplayedWorkloadWrapperBase::calculateWorkloadSampleRatio_()
    {
        assert(total_workload_opcnt_ > 0); // Must be updated by parseTraceFile_() in phase 0 of trace preprocessing

        const uint32_t trace_sample_opcnt = Config::getTraceSampleOpcnt(getWorkloadName_());
        if (trace_sample_opcnt >= total_workload_opcnt_)
        {
            workload_sample_ratio_ = 1.0;
        }
        else
        {
            workload_sample_ratio_ = static_cast<double>(trace_sample_opcnt) / static_cast<double>(total_workload_opcnt_);
        }

        std::ostringstream oss;
        oss << "workload sample ratio " << workload_sample_ratio_ << " for workload " << getWorkloadName_() << " with trace sample opcnt " << trace_sample_opcnt << " and total workload opcnt " << total_workload_opcnt_;
        Util::dumpNormalMsg(base_instance_name_, oss.str());

        assert(workload_sample_ratio_ > 0.0);
        assert(workload_sample_ratio_ <= 1.0);

        workload_sample_dist_ptr_ = new std::bernoulli_distribution(workload_sample_ratio_); // Bernoulli distribution with p of workload_sample_ratio_
        assert(workload_sample_dist_ptr_ != NULL);

        return;
    }

    uint32_t ReplayedWorkloadWrapperBase::dumpDatasetFile_() const
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needAllTraceFiles_()); // Trace preprocessor

        const std::string tmp_dataset_filepath = Util::getSampledDatasetFilepath(getWorkloadName_());
        assert(!Util::isFileExist(tmp_dataset_filepath, true)); // Must NOT exist (already verified by verifyDatasetAndWorkloadFile_() before)

        // Create and open a binary file for dumping dataset by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one dataset file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_dataset_filepath << " for dumping " << dataset_kvpairs_.size() << " sampled dataset of " << getWorkloadName_() << " with trace sample opcnt " << Config::getTraceSampleOpcnt(getWorkloadName_()) << " and total workload opcnt " << total_workload_opcnt_ << " (workload sample ratio " << workload_sample_ratio_ << ")";
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
            const uint32_t key_serialize_size = tmp_key.serialize(fs_ptr);
            size += key_serialize_size;

            // Must be sampled key
            std::unordered_map<Key, uint32_t, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(tmp_key);
            assert(tmp_iter != dataset_lookup_table_.end());

            // Value
            const Value& tmp_value = dataset_kvpairs_[i].second;
            const uint32_t value_serialize_size = tmp_value.serialize(fs_ptr, is_value_space_efficient);
            assert(value_serialize_size == tmp_value.getValuePayloadSize(is_value_space_efficient));
            size += value_serialize_size;
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    uint32_t ReplayedWorkloadWrapperBase::dumpWorkloadFile_() const
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needAllTraceFiles_()); // Trace preprocessor

        const std::string tmp_workload_filepath = Util::getSampledWorkloadFilepath(getWorkloadName_());
        assert(!Util::isFileExist(tmp_workload_filepath, true)); // Must NOT exist (already verified by verifyDatasetAndWorkloadFile_() before)

        // Create and open a binary file for dumping sampled workload items by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one workload file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_workload_filepath << " for dumping " << sample_workload_keys_.size() << " sampled workload items of " << getWorkloadName_() << " with trace sample opcnt " << Config::getTraceSampleOpcnt(getWorkloadName_()) << " and total workload opcnt " << total_workload_opcnt_ << " (workload sample ratio " << workload_sample_ratio_ << ")";
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_workload_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump key-value pairs of dataset
        // Format: sampled workload size, key, coded value size, key coded value size, ...
        uint32_t size = 0;
        // (0) sampled workload size
        const uint32_t sampled_workload_size = sample_workload_keys_.size();
        assert(sampled_workload_size > 0);
        fs_ptr->write((const char*)&sampled_workload_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-valuesize pairs
        for (uint32_t i = 0; i < sampled_workload_size; i++)
        {
            // Key
            const Key& tmp_key = sample_workload_keys_[i];
            const uint32_t key_serialize_size = tmp_key.serialize(fs_ptr);
            size += key_serialize_size;

            // Coded value size (< 0: read; = 0: delete; > 0: write)
            const int tmp_coded_value_size = sample_workload_value_sizes_[i];
            fs_ptr->write((const char*)&tmp_coded_value_size, sizeof(int));
            size += sizeof(int);
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (2) For role of dataset loader and cloud (ONLY for replayed traces)

    uint32_t ReplayedWorkloadWrapperBase::loadDatasetFile_()
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_LOADER || getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // dataset loader and cloud

        const std::string tmp_sampled_dataset_filepath = Util::getSampledDatasetFilepath(getWorkloadName_());

        bool is_exist = Util::isFileExist(tmp_sampled_dataset_filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "dataset file " << tmp_sampled_dataset_filepath << " does not exist -> please run trace_preprocessor before dataset loader and evaluation!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Open the existing binary file for sampled dataset items
        std::ostringstream oss;
        oss << "open file " << tmp_sampled_dataset_filepath << " for loading dataset of " << getWorkloadName_();
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_sampled_dataset_filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Load dataset key-value pairs from dataset file
        // Format: dataset size, key, value, key, value, ...
        uint32_t size = 0;
        // (0) dataset size
        uint32_t dataset_size = 0;
        fs_ptr->read((char *)&dataset_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-value pairs
        const bool is_value_space_efficient = true; // NOT deserialize value content
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            // Key
            Key tmp_key;
            uint32_t key_deserialize_size = tmp_key.deserialize(fs_ptr);
            size += key_deserialize_size;

            // Value
            Value tmp_value;
            uint32_t value_deserialize_size = tmp_value.deserialize(fs_ptr, is_value_space_efficient);
            size += value_deserialize_size;

            // Update dataset and dataset statistics
            const uint32_t unused_preprocess_phase = 0; // NOTE: preprocess phase is NOT used for dataset loader and cloud
            bool tmp_is_achieve_trace_sample_opcnt = updateDatasetOrSampleWorkload_(tmp_key, tmp_value, unused_preprocess_phase);
            assert(!tmp_is_achieve_trace_sample_opcnt); // NOTE: ONLY trace preprocessor can achieve trace sample opcnt
        }

        // Close file and release ifstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (3) For role of cloud for warmup speedup (ONLY for replayed traces)

    void ReplayedWorkloadWrapperBase::quickDatasetGet_(const Key& key, Value& value) const
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // cloud

        // Check dataset lookup table
        std::unordered_map<Key, uint32_t, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
        if (tmp_iter == dataset_lookup_table_.end())
        {
            // Key must exist
            std::ostringstream oss;
            oss << "key " << key.getKeyDebugstr() << " does not exist in dataset (lookup table size: " << dataset_lookup_table_.size() << "; dataset size: " << dataset_kvpairs_.size() << ") for quick get!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Get value
        const uint32_t dataset_kvpairs_index = tmp_iter->second;
        assert(dataset_kvpairs_index < dataset_kvpairs_.size());
        assert(dataset_kvpairs_[dataset_kvpairs_index].first == key);
        value = dataset_kvpairs_[dataset_kvpairs_index].second;

        return;
    }

    void ReplayedWorkloadWrapperBase::quickDatasetPut_(const Key& key, const Value& value)
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(getWorkloadUsageRole_() == WORKLOAD_USAGE_ROLE_CLOUD); // cloud

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

    void ReplayedWorkloadWrapperBase::quickDatasetDel_(const Key& key)
    {
        quickDatasetPut_(key, Value()); // Put a default value with is_deleted = true

        return;
    }

    // (4) For role of clients during evaluation

    uint32_t ReplayedWorkloadWrapperBase::loadWorkloadFile_()
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needWorkloadItems_()); // clients for evaluation

        const std::string tmp_sampled_workload_filepath = Util::getSampledWorkloadFilepath(getWorkloadName_());

        bool is_exist = Util::isFileExist(tmp_sampled_workload_filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "sampled workload file " << tmp_sampled_workload_filepath << " does not exist -> please run trace_preprocessor before dataset loader and evaluation!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Open the existing binary file for sampled workload items
        std::ostringstream oss;
        oss << "open file " << tmp_sampled_workload_filepath << " for loading sampled workload items of " << getWorkloadName_();
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_sampled_workload_filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Count per-key freq for dynamic workload patterns
        std::unordered_map<Key, uint32_t, KeyHasher> tmp_key_freq_map;

        // Load sampled workload key-valuesize pairs from dataset file
        // Format: sampled workload size, key, coded value size, key, coded value size, ...
        uint32_t size = 0;
        // (0) sampled workload size
        uint32_t sampled_workload_size = 0;
        fs_ptr->read((char *)&sampled_workload_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-valuesize pairs
        for (uint32_t i = 0; i < sampled_workload_size; i++)
        {
            // Key
            Key tmp_key;
            uint32_t key_deserialize_size = tmp_key.deserialize(fs_ptr);
            size += key_deserialize_size;

            // Coded value size
            int tmp_coded_value_size = -1;
            fs_ptr->read((char *)&tmp_coded_value_size, sizeof(int));
            size += sizeof(int);

            // Update current client workload key-value pairs
            curclient_workload_keys_.push_back(tmp_key);
            curclient_workload_value_sizes_.push_back(tmp_coded_value_size);

            // Update per-key freq map for dynamic workload patterns
            if (tmp_key_freq_map.find(tmp_key) == tmp_key_freq_map.end())
            {
                tmp_key_freq_map.insert(std::pair<Key, uint32_t>(tmp_key, 1));
            }
            else
            {
                tmp_key_freq_map[tmp_key] += 1;
            }
        }

        // Sort per-key freq map by freq in descending order
        std::multimap<uint32_t, Key, std::greater<uint32_t>> tmp_sorted_key_freq_map;
        for (std::unordered_map<Key, uint32_t, KeyHasher>::const_iterator iter = tmp_key_freq_map.begin(); iter != tmp_key_freq_map.end(); iter++)
        {
            tmp_sorted_key_freq_map.insert(std::pair<uint32_t, Key>(iter->second, iter->first));
        }

        // Update rank information for dynamic workload patterns
        curclient_ranked_unique_keys_.clear(); // Clear for safety
        for (std::multimap<uint32_t, Key, std::greater<uint32_t>>::const_iterator iter = tmp_sorted_key_freq_map.begin(); iter != tmp_sorted_key_freq_map.end(); iter++)
        {
            curclient_ranked_unique_keys_.push_back(iter->second);
        }

        // Close file and release ifstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (5) Common utilities (ONLY for replayed traces)

    bool ReplayedWorkloadWrapperBase::updateDatasetOrSampleWorkload_(const Key& key, const Value& value, const uint32_t& preprocess_phase)
    {
        if (!needDatasetItems_()) // Preprocessor, dataset loader, or cloud
        {
            std::ostringstream oss;
            oss << "invalid workload usage role " << getWorkloadUsageRole_() << " for updateDatasetOrSampleWorkload_()";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        bool is_achieve_trace_sample_opcnt = false;

        if (needAllTraceFiles_()) // Preprocessor (all trace files)
        {
            if (preprocess_phase == PHASE_FOR_WORKLOAD_SAMPLE_RATIO) // For workload sample ratio calculation
            {
                total_workload_opcnt_ += 1; // NOTE: update total opcnt for phase 0 of trace preprocessing
            }
            else if (preprocess_phase == PHASE_FOR_WORKLOAD_SAMPLE) // To sample dataset and workload items
            {
                bool is_sampled = false;
                if (workload_sample_ratio_ == 1.0) // No sampling
                {
                    is_sampled = true;
                }
                else // > 0.0 && < 1.0
                {
                    assert(workload_sample_dist_ptr_ != NULL);
                    assert(workload_sample_randgen_ptr_ != NULL);
                    is_sampled = (*workload_sample_dist_ptr_)(*workload_sample_randgen_ptr_); // Return true with probability p of workload_sample_ratio_
                }

                if (is_sampled) // If the workload item is sampled
                {
                    // Update sample dataset for the first workload item of the key
                    std::unordered_map<Key, uint32_t, KeyHasher>::iterator tmp_dataset_lookup_table_iter = dataset_lookup_table_.find(key);
                    if (tmp_dataset_lookup_table_iter == dataset_lookup_table_.end()) // The first request on the key
                    {
                        const uint32_t original_dataset_size = dataset_kvpairs_.size();
                        dataset_kvpairs_.push_back(std::pair(key, value));
                        tmp_dataset_lookup_table_iter = dataset_lookup_table_.insert(std::pair(key, original_dataset_size)).first;
                        
                        // Update dataset statistics
                        updateDatasetStatistics_(key, value, original_dataset_size);
                    }
                    assert(tmp_dataset_lookup_table_iter != dataset_lookup_table_.end());

                    // Always update workload keys and coded value sizes if the workload item is sampled
                    const uint32_t original_kvidx = tmp_dataset_lookup_table_iter->second;
                    assert(original_kvidx < dataset_kvpairs_.size());
                    const Value& original_value = dataset_kvpairs_[original_kvidx].second;
                    int tmp_coded_value_size = 0;
                    if (value.getValuesize() == original_value.getValuesize())
                    {
                        tmp_coded_value_size = -1; // Read request
                    }
                    else
                    {
                        tmp_coded_value_size = value.getValuesize(); // Write request (put or delete)
                    }
                    sample_workload_keys_.push_back(key);
                    sample_workload_value_sizes_.push_back(tmp_coded_value_size);
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "invalid preprocess phase " << preprocess_phase << " for updateDatasetOrSampleWorkload_()";
                Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }

            if (sample_workload_keys_.size() >= Config::getTraceSampleOpcnt(getWorkloadName_())) // Achieve trace sample opcnt
            {
                is_achieve_trace_sample_opcnt = true;
            }
        }
        else // Dataset loader and cloud (dataset file)
        {
            UNUSED(preprocess_phase);

            // NOTE: count all dataset items from dataset file
            std::unordered_map<Key, uint32_t, KeyHasher>::iterator tmp_dataset_lookup_table_iter = dataset_lookup_table_.find(key);
            if (tmp_dataset_lookup_table_iter == dataset_lookup_table_.end()) // The first request on the key
            {
                const uint32_t original_dataset_size = dataset_kvpairs_.size();
                dataset_kvpairs_.push_back(std::pair(key, value));
                tmp_dataset_lookup_table_iter = dataset_lookup_table_.insert(std::pair(key, original_dataset_size)).first; // Must be true due to NO sampling for dataset items from dataset file

                // Update dataset statistics
                updateDatasetStatistics_(key, value, original_dataset_size);
            }
            assert(tmp_dataset_lookup_table_iter != dataset_lookup_table_.end());
        }

        return is_achieve_trace_sample_opcnt;
    }

    void ReplayedWorkloadWrapperBase::updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size)
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_()); // Preprocessor, dataset loader, or cloud)

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
}
#include "workload/replayed_workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string ReplayedWorkloadWrapperBase::kClassName("ReplayedWorkloadWrapperBase");

    ReplayedWorkloadWrapperBase::ReplayedWorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, max_eval_workload_loadcnt)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        dataset_sample_ratio_ = Config::getTraceDatasetSampleRatio(workload_name);

        // For preprocessor
        total_workload_opcnt_ = 0;
        total_workload_keys_.clear();
        total_workload_value_sizes_.clear();
        dataset_sample_randgen_ptr_ = new std::mt19937(Util::DATASET_KVPAIR_SAMPLE_SEED);
        assert(dataset_sample_randgen_ptr_ != NULL);
        dataset_sample_dist_ptr_ = new std::bernoulli_distribution(dataset_sample_ratio_); // Bernoulli distribution with p of dataset_sample_ratio_
        assert(dataset_sample_dist_ptr_ != NULL);

        // For role of preprocessor, dataset loader, and cloud
        resetDatasetStatistics_();
        dataset_lookup_table_.clear();
        dataset_kvpairs_.clear();

        // For clients
        curclient_partial_dataset_kvmap_.clear();
        curclient_workload_keys_.clear();
        curclient_workload_value_sizes_.clear();
        eval_workload_opcnt_ = 0;

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
        assert(dataset_sample_randgen_ptr_ != NULL);
        delete dataset_sample_randgen_ptr_;
        dataset_sample_randgen_ptr_ = NULL;
        assert(dataset_sample_dist_ptr_ != NULL);
        delete dataset_sample_dist_ptr_;
        dataset_sample_dist_ptr_ = NULL;
    }

    WorkloadItem ReplayedWorkloadWrapperBase::generateWorkloadItem(const uint32_t& local_client_worker_idx)
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

    uint32_t ReplayedWorkloadWrapperBase::getPracticalKeycnt() const
    {
        checkIsValid_();

        // For role of preprocessor, dataset loader, and cloud
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needDatasetItems_());

        int64_t dataset_size = dataset_kvpairs_.size();
        return Util::toUint32(dataset_size);
    }

    uint32_t ReplayedWorkloadWrapperBase::getTotalOpcnt() const
    {
        checkIsValid_();

        assert(needAllTraceFiles_()); // Must be trace preprocessor to load all trace files

        return total_workload_opcnt_;
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

            // Reset total workload opcnt and total workload items before sampling
            total_workload_opcnt_ = 0;
            resetDatasetStatistics_();

            // NOTE: as we sample within a single round of parsing during trace preprocessing (i.e., NOT know keycnt in advance), we do NOT limit dataset sample cnt = keycnt *  sample ratio
            parseTraceFiles_(); // Update and sample dataset and total workload (if sample ratio < 1.0) for trace preprocessor

            // Dump dataset file by trace preprocessor for dataset loader and cloud
            const uint32_t dataset_filesize = dumpDatasetFile_();
            std::ostringstream oss;
            oss << "dump dataset file (" << dataset_filesize << " bytes) for workload " << getWorkloadName_();
            Util::dumpNormalMsg(base_instance_name_, oss.str());

            // Dump sampled workload file (if sample ratio < 1.0)
            if (dataset_sample_ratio_ < 1.0)
            {
                const uint32_t workload_filesize = dumpWorkloadFile_();
                oss.clear();
                oss.str("");
                oss << "dump workload file (" << workload_filesize << " bytes) for workload " << getWorkloadName_();
                Util::dumpNormalMsg(base_instance_name_, oss.str());
            }
            else if (dataset_sample_ratio_ == 1.0) // Sample ratio == 1.0
            {
                assert(total_workload_keys_.size() == 0);
                assert(total_workload_value_sizes_.size() == 0);
            }
            else
            {
                std::ostringstream oss;
                oss << "invalid dataset sample ratio " << dataset_sample_ratio_;
                Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }
        }
        else if (needWorkloadItems_()) // Need partial trace files for workload items (clients)
        {
            if (dataset_sample_ratio_ < 1.0)
            {
                const uint32_t workload_filesize = loadWorkloadFile_(); // Load partial workload file for clients (will update curclient_workload_keys_, curclient_workload_value_sizes_, and eval_workload_opcnt_; NO need curclient_partial_dataset_kvmap_)
                
                std::ostringstream oss;
                oss << "load partial workload file (" << workload_filesize << " bytes) for workload " << getWorkloadName_();
                Util::dumpNormalMsg(base_instance_name_, oss.str());
            }
            else
            {
                parseTraceFiles_(); // Update dataset workloads for clients
            }
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

        // Check if sampled workload filepath exists (if sample ratio < 1.0)
        if (Config::getTraceDatasetSampleRatio(workload_name) < 1.0)
        {
            const std::string tmp_sampled_workload_filepath = Util::getSampledWorkloadFilepath(workload_name);
            is_exist = Util::isFileExist(tmp_sampled_workload_filepath, true);
            if (is_exist)
            {
                std::ostringstream oss;
                oss << "sampled workload file " << tmp_sampled_workload_filepath << " already exists -> please delete sampled dataset and workload files before trace preprocessing!";
                Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }
        }

        return;
    }

    uint32_t ReplayedWorkloadWrapperBase::dumpDatasetFile_() const
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needAllTraceFiles_()); // Trace preprocessor

        assert(dataset_sample_ratio_ > 0.0 && dataset_sample_ratio_ <= 1.0); // NOTE: even if without sampling (i.e., dataset_sample_ratio_ == 1.0), we still need to dump dataset file for dataset loader and cloud (to avoid re-parsing all trace files for each client and cloud)

        const std::string tmp_dataset_filepath = Util::getSampledDatasetFilepath(getWorkloadName_());
        assert(!Util::isFileExist(tmp_dataset_filepath, true)); // Must NOT exist (already verified by verifyDatasetAndWorkloadFile_() before)

        // Create and open a binary file for dumping dataset by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one dataset file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_dataset_filepath << " for dumping sampled dataset of " << getWorkloadName_() << " with sample ratio " << dataset_sample_ratio_;
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

    uint32_t ReplayedWorkloadWrapperBase::dumpWorkloadFile_() const
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));
        assert(needAllTraceFiles_()); // Trace preprocessor

        assert(dataset_sample_ratio_ > 0.0 && dataset_sample_ratio_ < 1.0); // NOTE: we do NOT need to sample workload items and dump workload file if without sampling (i.e., dataset_sample_ratio_ == 1.0), as clients can directly load partial trace files for evaluation

        const std::string tmp_workload_filepath = Util::getSampledWorkloadFilepath(getWorkloadName_());
        assert(!Util::isFileExist(tmp_workload_filepath, true)); // Must NOT exist (already verified by verifyDatasetAndWorkloadFile_() before)

        // Create and open a binary file for dumping sampled workload items by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one workload file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_workload_filepath << " for dumping sampled workload items of " << getWorkloadName_() << " with sample ratio " << dataset_sample_ratio_;
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_workload_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump key-value pairs of dataset
        // Format: sampled workload size, key, coded value size, key coded value size, ...
        uint32_t size = 0;
        // (0) sampled workload size
        const uint32_t sampled_workload_size = total_workload_keys_.size();
        assert(sampled_workload_size > 0);
        fs_ptr->write((const char*)&sampled_workload_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-valuesize pairs
        for (uint32_t i = 0; i < sampled_workload_size; i++)
        {
            // Key
            const Key& tmp_key = total_workload_keys_[i];
            DynamicArray tmp_dynamic_array_for_key(tmp_key.getKeyPayloadSize());
            const uint32_t key_serialize_size = tmp_key.serialize(tmp_dynamic_array_for_key, 0);
            tmp_dynamic_array_for_key.writeBinaryFile(0, fs_ptr, key_serialize_size);
            size += key_serialize_size;

            // Coded value size (< 0: read; = 0: delete; > 0: write)
            const int tmp_coded_value_size = total_workload_value_sizes_[i];
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
            bool tmp_is_achieve_max_eval_workload_opcnt = updateDatasetOrWorkload_(tmp_key, tmp_value);
            assert(!tmp_is_achieve_max_eval_workload_opcnt); // NOTE: ONLY clients could achieve max eval workload opcnt
        }

        // Close file and release ofstream
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
        std::unordered_map<Key, std::pair<uint32_t, bool>, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
        if (tmp_iter == dataset_lookup_table_.end())
        {
            // Key must exist
            std::ostringstream oss;
            oss << "key " << key.getKeyDebugstr() << " does not exist in dataset (lookup table size: " << dataset_lookup_table_.size() << "; dataset size: " << dataset_kvpairs_.size() << ") for quick get!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Get value
        assert(tmp_iter->second.second); // Must be true for all dataset items from dataset file
        const uint32_t dataset_kvpairs_index = tmp_iter->second.first;
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
        std::unordered_map<Key, std::pair<uint32_t, bool>, KeyHasher>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
        assert(tmp_iter != dataset_lookup_table_.end()); // Key must exist

        // Put value
        assert(tmp_iter->second.second); // Must be true for all dataset items from dataset file
        const uint32_t dataset_kvpairs_index = tmp_iter->second.first;
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

            // Check per-client-worker max workload loadcnt limitation
            eval_workload_opcnt_ += 1; // NOTE: update eval opcnt for clients during evaluation
            if (eval_workload_opcnt_ >= getMaxEvalWorkloadLoadcnt_()) // Clients for evaluation
            {
                dumpInfoIfAchieveMaxLoadCnt_();
                break; // Stop parsing trace files
            }
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        // NOTE: NO need curclient_partial_dataset_kvmap_
        assert(curclient_partial_dataset_kvmap_.size() == 0);

        return size - 0;
    }

    void ReplayedWorkloadWrapperBase::dumpInfoIfAchieveMaxLoadCnt_() const
    {
        assert(needWorkloadItems_()); // Must be clients for evaluation
        assert(total_workload_opcnt_ == 0); // Clients will update eval workload opcnt instead of total workload opcnt (ONLY used by trace preprocessor)

        std::ostringstream oss;
        oss << "achieve max workload load cnt for evaluation: " << getMaxEvalWorkloadLoadcnt_() << "; # of loaded operations: " << eval_workload_opcnt_ << "; current client workload size: " << curclient_workload_keys_.size();
        Util::dumpNormalMsg(base_instance_name_, oss.str());
        return;
    }

    // (5) Common utilities (ONLY for replayed traces)

    bool ReplayedWorkloadWrapperBase::updateDatasetOrWorkload_(const Key& key, const Value& value)
    {
        bool is_achieve_max_eval_workload_loadcnt = false;

        if (needAllTraceFiles_()) // Preprocessor (all trace files)
        {
            bool is_sampled = false;

            // NOTE: count sampled data lines from all trace files
            std::unordered_map<Key, std::pair<uint32_t, bool>, KeyHasher>::iterator tmp_dataset_lookup_table_iter = dataset_lookup_table_.find(key);
            if (tmp_dataset_lookup_table_iter == dataset_lookup_table_.end()) // The first request on the key
            {
                // Sample the dataset item
                if (dataset_sample_ratio_ == 1.0)
                {
                    is_sampled = true;
                }
                else // > 0.0 && < 1.0
                {
                    is_sampled = (*dataset_sample_dist_ptr_)(*dataset_sample_randgen_ptr_); // Return true with probability p of dataset_sample_ratio_
                }

                const uint32_t original_dataset_size = dataset_kvpairs_.size();
                dataset_kvpairs_.push_back(std::pair(key, value));
                tmp_dataset_lookup_table_iter = dataset_lookup_table_.insert(std::pair(key, std::pair(original_dataset_size, is_sampled))).first;

                if (is_sampled)
                {
                    // Update dataset statistics
                    updateDatasetStatistics_(key, value, original_dataset_size);
                }
            }
            else
            {
                is_sampled = tmp_dataset_lookup_table_iter->second.second;
            }
            assert(tmp_dataset_lookup_table_iter != dataset_lookup_table_.end());

            if (is_sampled) // Update workload and total opcnt if the dataset item is sampled
            {
                total_workload_opcnt_ += 1; // NOTE: update total opcnt for trace preprocessing

                if (dataset_sample_ratio_ < 1.0) // Update total workload key-values ONLY if need dataset sampling
                {
                    assert(tmp_dataset_lookup_table_iter->second.second == true);

                    total_workload_keys_.push_back(key);

                    const uint32_t original_kvidx = tmp_dataset_lookup_table_iter->second.first;
                    assert(original_kvidx < dataset_kvpairs_.size());
                    const Value& original_value = dataset_kvpairs_[original_kvidx].second;
                    if (value.getValuesize() == original_value.getValuesize())
                    {
                        total_workload_value_sizes_.push_back(-1); // Read request
                    }
                    else
                    {
                        total_workload_value_sizes_.push_back(value.getValuesize()); // Write request (put or delete)
                    }
                }
            }
        }
        else if (needDatasetItems_()) // Dataset loader and cloud (dataset file)
        {
            // NOTE: count all dataset items from dataset file
            std::unordered_map<Key, std::pair<uint32_t, bool>, KeyHasher>::iterator tmp_dataset_lookup_table_iter = dataset_lookup_table_.find(key);
            if (tmp_dataset_lookup_table_iter == dataset_lookup_table_.end()) // The first request on the key
            {
                const uint32_t original_dataset_size = dataset_kvpairs_.size();
                dataset_kvpairs_.push_back(std::pair(key, value));
                tmp_dataset_lookup_table_iter = dataset_lookup_table_.insert(std::pair(key, std::pair(original_dataset_size, true))).first; // Must be true due to NO sampling for dataset items from dataset file

                // Update dataset statistics
                updateDatasetStatistics_(key, value, original_dataset_size);
            }
            assert(tmp_dataset_lookup_table_iter != dataset_lookup_table_.end());
        }
        else if (needWorkloadItems_()) // Clients (partial trace files or partial workload file) for evaluation phase
        {
            // NOTE: ONLY consider corresponding data lines from partial trace files, or corresponding workload items from partial workload file
            if (eval_workload_opcnt_ % getClientcnt_() == getClientIdx_()) // The current key-value pair is partitioned to the current client
            {
                std::unordered_map<Key, Value, KeyHasher>::iterator tmp_partial_dataset_kvmap_iter = curclient_partial_dataset_kvmap_.find(key);
                if (tmp_partial_dataset_kvmap_iter == curclient_partial_dataset_kvmap_.end()) // The first request on the key
                {
                    curclient_partial_dataset_kvmap_.insert(std::pair(key, value));

                    curclient_workload_keys_.push_back(key);
                    curclient_workload_value_sizes_.push_back(-1); // Treat the first request as read request, as stresstest phase is after dataset loading phase
                }
                else // Subsequent requests on the key
                {
                    const Value& original_value = tmp_partial_dataset_kvmap_iter->second;

                    curclient_workload_keys_.push_back(key);
                    if (value.getValuesize() == original_value.getValuesize())
                    {
                        curclient_workload_value_sizes_.push_back(-1); // Read request
                    }
                    else
                    {
                        curclient_workload_value_sizes_.push_back(value.getValuesize()); // Write request (put or delete)
                    }
                }
            }

            eval_workload_opcnt_ += 1; // NOTE: update eval opcnt for clients during evaluation

            if (eval_workload_opcnt_ >= getMaxEvalWorkloadLoadcnt_()) // Clients for evaluation
            {
                is_achieve_max_eval_workload_loadcnt = true; // Stop parsing trace files
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid workload usage role " << getWorkloadUsageRole_() << ", which does not need both dataset and workload items";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return is_achieve_max_eval_workload_loadcnt;
    }

    void ReplayedWorkloadWrapperBase::updateDatasetStatistics_(const Key& key, const Value& value, const uint32_t& original_dataset_size)
    {
        assert(Util::isReplayedWorkload(getWorkloadName_()));

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

    void ReplayedWorkloadWrapperBase::resetDatasetStatistics_()
    {
        average_dataset_keysize_ = 0;
        average_dataset_valuesize_ = 0;
        min_dataset_keysize_ = 0;
        min_dataset_valuesize_ = 0;
        max_dataset_keysize_ = 0;
        max_dataset_valuesize_ = 0;

        return;
    }
}
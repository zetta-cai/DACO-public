#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"
#include "workload/wikipedia_workload_wrapper.h"

namespace covered
{
    const std::string WORKLOAD_USAGE_ROLE_PREPROCESSOR("preprocessor");
    const std::string WORKLOAD_USAGE_ROLE_LOADER("loader");
    const std::string WORKLOAD_USAGE_ROLE_CLIENT("client");
    const std::string WORKLOAD_USAGE_ROLE_CLOUD("cloud");

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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt) : clientcnt_(clientcnt), client_idx_(client_idx), keycnt_(keycnt), perclient_opcnt_(perclient_opcnt), perclient_workercnt_(perclient_workercnt), workload_name_(workload_name), workload_usage_role_(workload_usage_role), max_eval_workload_loadcnt_(max_eval_workload_loadcnt)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        is_valid_ = false;

        // Verify workload usage role
        if (needDatasetItems_())
        {
            assert(max_eval_workload_loadcnt == 0);
        }
        else if (needWorkloadItems_())
        {
            assert(max_eval_workload_loadcnt > 0);

            // NOTE: keycnt and perclient_opcnt MUST > 0, as evaluation phase occurs after trace preprocessing
            assert(keycnt > 0);
            assert(perclient_opcnt > 0);
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

    // (1) For role of preprocessor

    void WorkloadWrapperBase::verifyDatasetFileForPreprocessor_()
    {
        // Check if trace dirpath exists
        const std::string tmp_dirpath = Config::getTraceDirpath();
        bool is_exist = Util::isDirectoryExist(tmp_dirpath, true);
        if (!is_exist)
        {
            std::ostringstream oss;
            oss << "trace directory " << tmp_dirpath << " does not exist!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Check if dataset filepath exists
        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);
        bool is_exist = Util::isFileExist(tmp_dataset_filepath, true);
        if (is_exist)
        {
            std::ostringstream oss;
            oss << "dataset file " << tmp_dataset_filepath << " already exists -> please delete it before trace preprocessing!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void WorkloadWrapperBase::dumpDatasetFile_() const
    {
        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);
        assert(!Util::isFileExist(tmp_dataset_filepath, true)); // Must NOT exist (already verified by verifyDatasetFile_() before)

        // Create and open a binary file for dumping dataset by trace preprocessor
        // NOTE: trace preprocessor is a single-thread program and hence ONLY one dataset file will be created for each given workload
        std::ostringstream oss;
        oss << "open file " << tmp_dataset_filepath << " for dumping dataset of " << workload_name_;
        Util::dumpNormalMsg(instance_name_, oss.str());
        std::fstream* fs_ptr = Util::openFile(tmp_dataset_filepath, std::ios_base::out | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Dump key-value pairs of dataset
        // Format: dataset size, key, value, key, value, ...
        uint32_t size = 0;
        // (0) dataset size
        const uint32_t dataset_size = getPracticalKeycnt();
        fs_ptr->write((const char*)&dataset_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        // (1) key-value pairs
        const bool is_value_space_efficient = true; // NOT serialize value content
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            WorkloadItem tmp_item = getDatasetItem(i);

            // Key
            const Key& tmp_key = tmp_item.getKey();
            DynamicArray tmp_dynamic_array_for_key(tmp_key.getKeyPayloadSize());
            const uint32_t key_serialize_size = tmp_key.serialize(tmp_dynamic_array_for_key, 0);
            tmp_dynamic_array_for_key.writeBinaryFile(0, fs_ptr, key_serialize_size);
            size += key_serialize_size;

            // Value
            const Value& tmp_value = tmp_item.getValue();
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

    // (2) For role of dataset loader and cloud

    void WorkloadWrapperBase::loadDatasetFile_(std::vector<std::pair<Key, Value>>& dataset_kvpairs, std::unordered_map<Key, Value, KeyHasher>& dataset_lookup_table)
    {
        const std::string tmp_dataset_filepath = Util::getDatasetFilepath(workload_name_);

        bool is_exist = Util::isFileExist(tmp_dataset_filepath, true);
        if (!is_exist)
        {
            // File does not exist
            std::ostringstream oss;
            oss << "dataset file " << tmp_dataset_filepath << " does not exist -> please run trace_preprocessor before dataset loader and evaluation!";
            Util::dumpErrorMsg(instance_name_, oss.str());
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
        dataset_kvpairs.resize(dataset_size);
        // (1) key-value pairs
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
            dataset_kvpairs[i] = std::pair(tmp_key, tmp_value);
            dataset_lookup_table.insert(std::pair(tmp_key, i));
        }

        // Close file and release ofstream
        fs_ptr->close();
        delete fs_ptr;
        fs_ptr = NULL;

        return size - 0;
    }

    // (3) Common utilities

    bool WorkloadWrapperBase::needAllTraceFiles_()
    {
        // Trace preprocessor loads all trace files for dataset items and total opcnt
        if (workload_usage_role_ == WORKLOAD_USAGE_ROLE_PREPROCESSOR)
        {
            return true;
        }
        return false;
    }

    bool WorkloadWrapperBase::needDatasetItems_()
    {
        // Trace preprocessor, dataset loader, and cloud need dataset items for preprocessing, loading, and warmup speedup
        // Trace preprocessor is from all trace files, while dataset loader and cloud are from dataset file dumped by trace preprocessor
        if (workload_usage_role == WORKLOAD_USAGE_ROLE_PREPROCESSOR || workload_usage_role == WORKLOAD_USAGE_ROLE_LOADER || workload_usage_role == WORKLOAD_USAGE_ROLE_CLOUD) // ONLY need dataset items for preprocessing, loading, and warmup speedup
        {
            return true;
        }
        return false;
    }

    bool WorkloadWrapperBase::needWorkloadItems_()
    {
        // Client needs workload items for evaluation
        if (workload_usage_role == WORKLOAD_USAGE_ROLE_CLIENT) // ONLY need workload items for evaluation
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
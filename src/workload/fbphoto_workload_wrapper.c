#include "workload/fbphoto_workload_wrapper.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const uint32_t FbphotoWorkloadWrapper::FBPHOTO_WORKLOAD_DATASET_KEYCNT = 1300000; // 1.3M dataset items

    const std::string FbphotoWorkloadWrapper::kClassName("FbphotoWorkloadWrapper");

    FbphotoWorkloadWrapper::FbphotoWorkloadWrapper(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role) : WorkloadWrapperBase(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role)
    {
        // NOTE: facebook photo caching is not replayed trace and NO need for trace preprocessing (also NO need to dump dataset file)
        assert(!needAllTraceFiles_()); // Must NOT trace preprocessor

        // Differentiate fbphoto workload wrapper in different clients
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
        loadFbphotoPropertiesFile_(); // Load properties file to update dataset keys, probs, value sizes, and dataset statistics

        // For clients
        client_worker_item_randgen_ptrs_.resize(perclient_workercnt, NULL);
        client_worker_reqdist_ptrs_.resize(perclient_workercnt, NULL);
    }

    FbphotoWorkloadWrapper::~FbphotoWorkloadWrapper()
    {
        // For clients, dataset loader, and cloud

        if (needWorkloadItems_()) // Clients
        {
            // Release random generator for each client worker
            for (uint32_t i = 0; i < client_worker_item_randgen_ptrs_.size(); i++)
            {
                assert(client_worker_item_randgen_ptrs_[i] != NULL);
                delete client_worker_item_randgen_ptrs_[i];
                client_worker_item_randgen_ptrs_[i] = NULL;
            }

            // Release request distribution for each client worker
            for (uint32_t i = 0; i < client_worker_reqdist_ptrs_.size(); i++)
            {
                assert(client_worker_reqdist_ptrs_[i] != NULL);
                delete client_worker_reqdist_ptrs_[i];
                client_worker_reqdist_ptrs_[i] = NULL;
            }
        }
    }

    WorkloadItem FbphotoWorkloadWrapper::generateWorkloadItem(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needWorkloadItems_()); // Must be clients for evaluation
        assert(local_client_worker_idx < client_worker_item_randgen_ptrs_.size());

        // Get a workload index randomly
        std::mt19937_64* request_randgen_ptr = client_worker_item_randgen_ptrs_[local_client_worker_idx];
        assert(request_randgen_ptr != NULL);
        std::uniform_int_distribution<uint32_t>* request_dist_ptr = client_worker_reqdist_ptrs_[local_client_worker_idx];
        assert(request_dist_ptr != NULL);
        const uint32_t tmp_workload_index = (*request_dist_ptr)(*request_randgen_ptr);

        // Get the key index
        const uint32_t tmp_key_index = workload_key_indices_[tmp_workload_index];
        assert(tmp_key_index < dataset_keys_.size());

        // Get key
        const uint32_t tmp_keyint = dataset_keys_[tmp_key_index];
        assert(tmp_keyint == tmp_key_index + 1);
        Key tmp_key(std::string((const char*)&tmp_keyint, sizeof(uint32_t)));
        
        return WorkloadItem(tmp_key, Value(dataset_valsizes_[tmp_key_index]), WorkloadItemType::kWorkloadItemGet); // NOT found read-write ratio in the paper -> treat as read-only for all methods with fair comparisons
    }

    uint32_t FbphotoWorkloadWrapper::getPracticalKeycnt() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return dataset_keys_.size();
    }

    WorkloadItem FbphotoWorkloadWrapper::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud
        assert(itemidx < getPracticalKeycnt());

        uint32_t tmp_keyuint = dataset_keys_[itemidx];
        Key tmp_key(std::string((const char *)&tmp_keyuint, sizeof(uint32_t)));

        return WorkloadItem(tmp_key, Value(dataset_valsizes_[itemidx]), WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double FbphotoWorkloadWrapper::getAvgDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_keysize_;
    }
    
    double FbphotoWorkloadWrapper::getAvgDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_valuesize_;
    }

    uint32_t FbphotoWorkloadWrapper::getMinDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_keysize_;
    }

    uint32_t FbphotoWorkloadWrapper::getMinDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_valuesize_;
    }

    uint32_t FbphotoWorkloadWrapper::getMaxDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_keysize_;
    }

    uint32_t FbphotoWorkloadWrapper::getMaxDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_valuesize_;
    }

    // For warmup speedup

    void FbphotoWorkloadWrapper::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get index
        uint32_t tmp_keyint = 0;
        memcpy((char *)&tmp_keyint, key.getKeystr().c_str(), sizeof(uint32_t));
        uint32_t tmp_key_index = tmp_keyint - 1;
        assert(tmp_key_index < dataset_keys_.size()); // Should not access an unexisting key during warmup
        assert(dataset_keys_[tmp_key_index] == tmp_keyint);

        uint32_t value_size = dataset_valsizes_[tmp_key_index];
        value = Value(value_size);

        return;
    }

    void FbphotoWorkloadWrapper::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get index
        uint32_t tmp_keyint = 0;
        memcpy((char *)&tmp_keyint, key.getKeystr().c_str(), sizeof(uint32_t));
        uint32_t tmp_key_index = tmp_keyint - 1;
        assert(tmp_key_index < dataset_keys_.size()); // Should not access an unexisting key during warmup
        assert(dataset_keys_[tmp_key_index] == tmp_keyint);

        dataset_valsizes_[tmp_key_index] = value.getValuesize();

        return;
    }

    void FbphotoWorkloadWrapper::quickDatasetDel(const Key& key)
    {
        quickDatasetPut(key, Value()); // Use default value with is_deleted = true and value size = 0 as delete operation

        return;
    }

    void FbphotoWorkloadWrapper::loadFbphotoPropertiesFile_()
    {
        // NOTE: MUST be the same as properties filepath in scripts/workload/facebook_photocache.py
        const std::string fbphoto_properties_filepath = Config::getTraceDirpath() + "/fbphoto.properties";

        // Check existance of properties file
        if (!Util::isFileExist(fbphoto_properties_filepath, true))
        {
            std::ostringstream oss;
            oss << "failed to find Facebook photo caching dataset file " << fbphoto_properties_filepath << "!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Load properties file including dataset size, probs, and value sizes
        std::fstream* fs_ptr = Util::openFile(fbphoto_properties_filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Load dataset size
        uint32_t dataset_size = 0;
        fs_ptr->read((char *)&dataset_size, sizeof(uint32_t));
        assert(dataset_size == FBPHOTO_WORKLOAD_DATASET_KEYCNT); // NOTE: Facebook photo caching dataset size is fixed (1.3M)

        // Load dataset probs and value sizes
        dataset_keys_.clear();
        dataset_keys_.resize(dataset_size);
        dataset_probs_.clear();
        dataset_probs_.resize(dataset_size);
        dataset_valsizes_.clear();
        dataset_valsizes_.resize(dataset_size, 0);
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            dataset_keys_[i] = i + 1; // From 1 to 1.3M

            // Load prob
            float tmp_prob = 0.0;
            fs_ptr->read((char *)&tmp_prob, sizeof(float));
            dataset_probs_[i] = static_cast<double>(tmp_prob);

            // Load value size
            uint32_t tmp_valsize = 0;
            fs_ptr->read((char *)&tmp_valsize, sizeof(uint32_t));
            dataset_valsizes_[i] = tmp_valsize;
        }
        
        // Update dataset statistics
        average_dataset_keysize_ = sizeof(uint32_t);
        min_dataset_keysize_ = sizeof(uint32_t);
        max_dataset_keysize_ = sizeof(uint32_t);
        for (uint32_t i = 0; i < dataset_size; i++)
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
        average_dataset_valuesize_ /= dataset_size;

        return;
    }

    void FbphotoWorkloadWrapper::initWorkloadParameters_()
    {
        if (needWorkloadItems_()) // Clients
        {
            // Create workload generator for each client
            for (uint32_t tmp_local_client_worker_idx = 0; tmp_local_client_worker_idx < getPerclientWorkercnt_(); tmp_local_client_worker_idx++)
            {
                uint32_t tmp_global_client_worker_idx = Util::getGlobalClientWorkerIdx(getClientIdx_(), tmp_local_client_worker_idx, getPerclientWorkercnt_());

                // Each per-client worker uses worker_idx as deterministic seed to create a random generator and get different requests
                // NOTE: we use global_client_worker_idx to randomly generate requests from workload items
                std::mt19937_64* tmp_client_worker_item_randgen_ptr_ = new std::mt19937_64(tmp_global_client_worker_idx);
                // (OBSOLETE: homogeneous cache access patterns is a WRONG assumption -> we should ONLY follow homogeneous workload distribution yet still with heterogeneous cache access patterns) NOTE: we use WORKLOAD_KVPAIR_GENERATION_SEED to generate requests with homogeneous cache access patterns
                //std::mt19937_64* tmp_client_worker_item_randgen_ptr_ = new std::mt19937_64(Util::WORKLOAD_KVPAIR_GENERATION_SEED);
                if (tmp_client_worker_item_randgen_ptr_ == NULL)
                {
                    Util::dumpErrorMsg(instance_name_, "failed to create a random generator for requests!");
                    exit(1);
                }

                client_worker_item_randgen_ptrs_[tmp_local_client_worker_idx] = tmp_client_worker_item_randgen_ptr_;

                client_worker_reqdist_ptrs_[tmp_local_client_worker_idx] = new std::uniform_int_distribution<uint32_t>(0, getPerclientOpcnt_() - 1);
                assert(client_worker_reqdist_ptrs_[tmp_local_client_worker_idx] != NULL);
            }
        }

        return;
    }

    void FbphotoWorkloadWrapper::overwriteWorkloadParameters_()
    {
        // Do nothing

        return;
    }

    void FbphotoWorkloadWrapper::createWorkloadGenerator_()
    {
        // facebook::cachelib::cachebench::WorkloadGenerator will generate keycnt key-value pairs by generateReqs() and generate perclient_opcnt_ requests by generateKeyDistributions() in constructor
        uint32_t tmp_client_idx = 0;
        if (needWorkloadItems_()) // Clients
        {
            // Prepare workloads for the current client
            tmp_client_idx = getClientIdx_();
            const uint32_t perclient_opcnt = getPerclientOpcnt_();
            workload_key_indices_.resize(perclient_opcnt);

            // Use client idx as random seed to generate workload items for the current client
            std::mt19937_64 tmp_randgen(tmp_client_idx);
            std::discrete_distribution<uint32_t> tmp_dist(dataset_probs_.begin(), dataset_probs_.end());
            for (uint32_t i = 0; i < perclient_opcnt; i++)
            {
                uint32_t tmp_key_index = tmp_dist(tmp_randgen);
                assert(tmp_key_index < dataset_keys_.size());
                assert(dataset_keys_[tmp_key_index] == tmp_key_index + 1);

                workload_key_indices_[i] = tmp_key_index;
            }
        }
        else // Dataset loader and cloud
        {
            // NOTE: ONLY need dataset items, which has been loaded in constructor

            // Do nothing
        }
    }

    // (2) Common utilities

    void FbphotoWorkloadWrapper::checkPointers_() const
    {
        if (needWorkloadItems_()) // Clients
        {
            assert(client_worker_item_randgen_ptrs_.size() > 0);
            for (uint32_t i = 0; i < client_worker_item_randgen_ptrs_.size(); i++)
            {
                assert(client_worker_item_randgen_ptrs_[i] != NULL);
            }

            assert(client_worker_reqdist_ptrs_.size() > 0);
            for (uint32_t i = 0; i < client_worker_reqdist_ptrs_.size(); i++)
            {
                assert(client_worker_reqdist_ptrs_[i] != NULL);
            }
        }
    }
}

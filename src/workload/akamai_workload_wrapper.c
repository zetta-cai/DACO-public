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

        // NOTE: Akamai workloads are not replayed single-node traces and NO need to load all single-node trace files (also NO such files) for sampling-based trace preprocessing
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
        loadDatasetFile_(); // Load dataset file to update dataset_valsizes_ and dataset statistics (required by all roles including clients, dataset loader, and cloud); clients use value sizes to generate workload items (yet not used due to GET requests)

        // For clients
        curclient_perworker_workload_objids_.clear();
    }

    // TODO: END HERE

    ZetaWorkloadWrapper::~ZetaWorkloadWrapper()
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

    WorkloadItem ZetaWorkloadWrapper::generateWorkloadItem(const uint32_t& local_client_worker_idx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needWorkloadItems_()); // Must be clients for evaluation
        assert(local_client_worker_idx < client_worker_item_randgen_ptrs_.size());

        // Get a workload index randomly
        std::mt19937_64* request_randgen_ptr = client_worker_item_randgen_ptrs_[local_client_worker_idx];
        assert(request_randgen_ptr != NULL);
        std::discrete_distribution<uint32_t>* request_dist_ptr = client_worker_reqdist_ptrs_[local_client_worker_idx];
        assert(request_dist_ptr != NULL);
        const uint32_t tmp_key_index = (*request_dist_ptr)(*request_randgen_ptr); // NOTE: here we directly use Zeta distribution to select item from dataset as workload item, instead of selecting item from pre-generated workload items (approximate workload distribution as in src/workload/fbphoto_workload_wrapper.c)
        assert(tmp_key_index < dataset_keys_.size());

        // Get key
        Key tmp_key(dataset_keys_[tmp_key_index]);
        
        return WorkloadItem(tmp_key, Value(dataset_valsizes_[tmp_key_index]), WorkloadItemType::kWorkloadItemGet); // NOT found read-write ratio in the paper -> treat as read-only for all methods with fair comparisons
    }

    uint32_t ZetaWorkloadWrapper::getPracticalKeycnt() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return dataset_keys_.size();
    }

    WorkloadItem ZetaWorkloadWrapper::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud
        assert(itemidx < getPracticalKeycnt());

        Key tmp_key(dataset_keys_[itemidx]);

        return WorkloadItem(tmp_key, Value(dataset_valsizes_[itemidx]), WorkloadItemType::kWorkloadItemPut);
    }

    // Get average/min/max dataset key/value size

    double ZetaWorkloadWrapper::getAvgDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_keysize_;
    }
    
    double ZetaWorkloadWrapper::getAvgDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return average_dataset_valuesize_;
    }

    uint32_t ZetaWorkloadWrapper::getMinDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_keysize_;
    }

    uint32_t ZetaWorkloadWrapper::getMinDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return min_dataset_valuesize_;
    }

    uint32_t ZetaWorkloadWrapper::getMaxDatasetKeysize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_keysize_;
    }

    uint32_t ZetaWorkloadWrapper::getMaxDatasetValuesize() const
    {
        checkIsValid_();
        checkPointers_();

        assert(needDatasetItems_()); // Dataset loader and cloud

        return max_dataset_valuesize_;
    }

    // For warmup speedup

    void ZetaWorkloadWrapper::quickDatasetGet(const Key& key, Value& value) const
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get index
        const std::string tmp_keystr = key.getKeystr();
        uint32_t tmp_keyrank = getKeyrankFromKeystr_(tmp_keystr);
        assert(tmp_keyrank >= 1);
        uint32_t tmp_key_index = tmp_keyrank - 1;
        assert(tmp_key_index < dataset_keys_.size()); // Should not access an unexisting key during warmup
        assert(dataset_keys_[tmp_key_index] == tmp_keystr);

        uint32_t value_size = dataset_valsizes_[tmp_key_index];
        value = Value(value_size);

        return;
    }

    void ZetaWorkloadWrapper::quickDatasetPut(const Key& key, const Value& value)
    {
        checkIsValid_();
        checkPointers_();

        assert(getWorkloadUsageRole_() == WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD);

        // Get index
        const std::string tmp_keystr = key.getKeystr();
        uint32_t tmp_keyrank = getKeyrankFromKeystr_(tmp_keystr);
        assert(tmp_keyrank >= 1);
        uint32_t tmp_key_index = tmp_keyrank - 1;
        assert(tmp_key_index < dataset_keys_.size()); // Should not access an unexisting key during warmup
        assert(dataset_keys_[tmp_key_index] == tmp_keystr);

        dataset_valsizes_[tmp_key_index] = value.getValuesize();

        return;
    }

    void ZetaWorkloadWrapper::quickDatasetDel(const Key& key)
    {
        quickDatasetPut(key, Value()); // Use default value with is_deleted = true and value size = 0 as delete operation

        return;
    }

    void ZetaWorkloadWrapper::loadZetaCharacteristicsFile_()
    {
        // NOTE: MUST be the same as characteristics filepath in scripts/workload/characterize_traces.py
        const std::string zeta_characteristics_filepath = Config::getTraceDirpath() + "/" + getWorkloadName_() + ".characteristics";

        // Check existance of characteristics file
        if (!Util::isFileExist(zeta_characteristics_filepath, true))
        {
            std::ostringstream oss;
            oss << "failed to find the characteristics file " << zeta_characteristics_filepath << "!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // (1) Load characteristics file including Zipfian constant, key size histogram, and value size histogram
        std::fstream* fs_ptr = Util::openFile(zeta_characteristics_filepath, std::ios_base::in | std::ios_base::binary);
        assert(fs_ptr != NULL);

        // Load Zipfian constant
        double zipf_constant = 0.0;
        fs_ptr->read((char *)&zipf_constant, sizeof(double));
        assert(zipf_constant > 1.0); // Zeta-based Zipfian constant must be larger than 1.0

        // Load key size histogram
        uint32_t keysize_histogram_size = 0;
        fs_ptr->read((char *)&keysize_histogram_size, sizeof(uint32_t));
        assert(keysize_histogram_size > 0);
        std::vector<uint32_t> keysize_histogram(keysize_histogram_size, 0);
        for (uint32_t tmp_keysize = 0; tmp_keysize < keysize_histogram_size; tmp_keysize++)
        {
            uint32_t tmp_keysize_freq = 0;
            fs_ptr->read((char *)&tmp_keysize_freq, sizeof(uint32_t));
            keysize_histogram[tmp_keysize] = tmp_keysize_freq;
        }

        // Load value size histogram
        uint32_t valuesize_histogram_size = 0;
        fs_ptr->read((char *)&valuesize_histogram_size, sizeof(uint32_t));
        assert(valuesize_histogram_size > 0);
        std::vector<uint32_t> valuesize_histogram(valuesize_histogram_size, 0);
        for (uint32_t tmp_valuesize = 0; tmp_valuesize < valuesize_histogram_size; tmp_valuesize++)
        {
            uint32_t tmp_valuesize_freq = 0;
            fs_ptr->read((char *)&tmp_valuesize_freq, sizeof(uint32_t));
            valuesize_histogram[tmp_valuesize] = tmp_valuesize_freq;
        }

        // (2) Convert characteristics into key/value size distribution
        
        // Get dataset key size distribution
        std::vector<uint32_t> existing_keysizes(0); // existing key sizes (i.e., with positive freqs)
        std::vector<uint32_t> positive_freqs(0); // positive freqs of existing key sizes
        uint32_t total_keysize_freq = 0;
        for (uint32_t tmp_keysize_idx = 0; tmp_keysize_idx < keysize_histogram_size; tmp_keysize_idx++)
        {
            uint32_t tmp_keysize_freq = keysize_histogram[tmp_keysize_idx];
            if (tmp_keysize_freq > 0)
            {
                // NOTE: make sure that scripts/workload/utils/trace_loader.py also treats per bucket as 1B and key size histogram ranges from 1B to 1024B
                existing_keysizes.push_back(tmp_keysize_idx + 1);
                positive_freqs.push_back(tmp_keysize_freq);
                total_keysize_freq += tmp_keysize_freq;
            }
        }

        assert(existing_keysizes.size() > 0);
        bool is_fixed_keysize = (existing_keysizes.size() == 1);
        uint32_t fixed_keysize = 0;
        std::discrete_distribution<uint32_t>* variable_keysize_dist = NULL;
        if (is_fixed_keysize)
        {
            fixed_keysize = existing_keysizes[0];
            assert(positive_freqs[0] == total_keysize_freq);
        }
        else
        {
            std::vector<double> keysize_probs(positive_freqs.size(), 0.0);
            for (uint32_t i = 0; i < positive_freqs.size(); i++)
            {
                keysize_probs[i] = static_cast<double>(positive_freqs[i]) / static_cast<double>(total_keysize_freq);
            }
            variable_keysize_dist = new std::discrete_distribution<uint32_t>(keysize_probs.begin(), keysize_probs.end());
            assert(variable_keysize_dist != NULL);
        }

        // Get dataset value size distribution
        std::vector<uint32_t> existing_valuesizes(0); // existing value sizes (i.e., with positive freqs)
        std::vector<uint32_t> positive_valuesize_freqs(0); // positive freqs of existing value sizes
        uint32_t total_valuesize_freq = 0;
        for (uint32_t tmp_valuesize_idx = 0; tmp_valuesize_idx < valuesize_histogram_size; tmp_valuesize_idx++)
        {
            uint32_t tmp_valuesize_freq = valuesize_histogram[tmp_valuesize_idx];
            if (tmp_valuesize_freq > 0)
            {
                // NOTE: make sure that scripts/workload/utils/trace_loader.py also treats per bucket as 1KiB and key size histogram ranges from 1KiB to 10240KiB
                existing_valuesizes.push_back((tmp_valuesize_idx + 1) * 1024);
                positive_valuesize_freqs.push_back(tmp_valuesize_freq);
                total_valuesize_freq += tmp_valuesize_freq;
            }
        }

        assert(existing_valuesizes.size() > 0);
        bool is_fixed_valuesize = (existing_valuesizes.size() == 1);
        uint32_t fixed_valuesize = 0;
        std::discrete_distribution<uint32_t>* variable_valuesize_dist = NULL;
        if (is_fixed_valuesize)
        {
            fixed_valuesize = existing_valuesizes[0];
            assert(positive_valuesize_freqs[0] == total_valuesize_freq);
        }
        else
        {
            std::vector<double> valuesize_probs(positive_valuesize_freqs.size(), 0.0);
            for (uint32_t i = 0; i < positive_valuesize_freqs.size(); i++)
            {
                valuesize_probs[i] = static_cast<double>(positive_valuesize_freqs[i]) / static_cast<double>(total_valuesize_freq);
            }
            variable_valuesize_dist = new std::discrete_distribution<uint32_t>(valuesize_probs.begin(), valuesize_probs.end());
            assert(variable_valuesize_dist != NULL);
        }

        // (3) Use Zipfian constant and key/value size distribution to generate keys, probs, and value sizes

        // Initialize dataset keys, probs, and value sizes
        const uint32_t dataset_size = getKeycnt_();
        dataset_keys_.clear();
        dataset_keys_.resize(dataset_size, "");
        dataset_probs_.clear();
        dataset_probs_.resize(dataset_size, 0.0);
        dataset_valsizes_.clear();
        dataset_valsizes_.resize(dataset_size, 0);

        // Generate per-rank key, prob, and value size
        std::mt19937_64 tmp_dataset_randgen(Util::DATASET_KVPAIR_GENERATION_SEED);
        const double riemann_zeta_value = calcRiemannZeta_(zipf_constant);
        double total_prob = 0.0;
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            uint32_t tmp_rank = i + 1;

            // Generate key
            uint32_t tmp_keysize = fixed_keysize;
            if (!is_fixed_keysize)
            {
                uint32_t tmp_keysize_idx = (*variable_keysize_dist)(tmp_dataset_randgen);
                assert(tmp_keysize_idx < existing_keysizes.size());
                tmp_keysize = existing_keysizes[tmp_keysize_idx];
            }
            dataset_keys_[i] = getKeystrFromKeyrank_(tmp_rank, tmp_keysize);

            // Generate prob
            double tmp_prob = 1.0 / std::pow(static_cast<double>(tmp_rank), zipf_constant) / riemann_zeta_value;
            dataset_probs_[i] = tmp_prob;
            total_prob += tmp_prob;

            // Generate value size
            uint32_t tmp_valuesize = fixed_valuesize;
            if (!is_fixed_valuesize)
            {
                uint32_t tmp_valuesize_idx = (*variable_valuesize_dist)(tmp_dataset_randgen);
                assert(tmp_valuesize_idx < existing_valuesizes.size());
                tmp_valuesize = existing_valuesizes[tmp_valuesize_idx];
            }
            dataset_valsizes_[i] = tmp_valuesize;
        }

        // Normalize dataset probs to sum to 1.0
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            dataset_probs_[i] /= total_prob;
        }

        // Update dataset statistics
        for (uint32_t i = 0; i < dataset_size; i++)
        {
            uint32_t tmp_keysize = dataset_keys_[i].length();
            average_dataset_keysize_ += tmp_keysize;

            if (i == 0 || tmp_keysize < min_dataset_keysize_)
            {
                min_dataset_keysize_ = tmp_keysize;
            }

            if (i == 0 || tmp_keysize > max_dataset_keysize_)
            {
                max_dataset_keysize_ = tmp_keysize;
            }

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
        average_dataset_keysize_ /= dataset_size;
        average_dataset_valuesize_ /= dataset_size;

        // TODO: Print debug info (to verify the same dataset across different clients and the same Zeta distribution between C++ and python)
        std::ostringstream oss;
        oss << "zipf_constant: " << zipf_constant << std::endl;
        oss << "dataset_keys_[0]: " << Key(dataset_keys_[0]).getKeyDebugstr() << "; dataset_probs_[0]: " << dataset_probs_[0] << "; dataset_valsizes_[0]: " << dataset_valsizes_[0] << std::endl;
        oss << "dataset_keys_[1]: " << Key(dataset_keys_[1]).getKeyDebugstr() << "; dataset_probs_[1]: " << dataset_probs_[1] << "; dataset_valsizes_[1]: " << dataset_valsizes_[1];
        Util::dumpNormalMsg(instance_name_, oss.str());

        if (!is_fixed_keysize) // Release key size distribution if necessary
        {
            assert(variable_keysize_dist != NULL);
            delete variable_keysize_dist;
            variable_keysize_dist = NULL;
        }

        if (!is_fixed_valuesize) // Release value size distribution if necessary
        {
            assert(variable_valuesize_dist != NULL);
            delete variable_valuesize_dist;
            variable_valuesize_dist = NULL;
        }

        return;
    }

    std::string ZetaWorkloadWrapper::getKeystrFromKeyrank_(const int64_t& keyrank, const uint32_t& keysize)
    {
        assert(keyrank >= 1);
        const uint32_t keyrank_bytecnt = sizeof(int64_t); // 8B

        // Create keystr (>= 4B)
        uint32_t tmp_keysize = keysize;
        if (keysize < keyrank_bytecnt)
        {
            tmp_keysize = keyrank_bytecnt;
        }
        std::string tmp_keystr(tmp_keysize, '0');

        // Copy keyrank str to the first 8B
        std::string keyrank_str((const char*)&keyrank, keyrank_bytecnt); // 8B
        for (uint32_t i = 0; i < keyrank_bytecnt; i++)
        {
            tmp_keystr[i] = keyrank_str[i];
        }

        return tmp_keystr;
    }

    double ZetaWorkloadWrapper::calcRiemannZeta_(const double& zipf_constant)
    {
        double riemann_zeta_value = 0.0;
        for (uint32_t i = 1; i <= RIEMANN_ZETA_PRECISION; i++)
        {
            riemann_zeta_value += 1.0 / std::pow(static_cast<double>(i), zipf_constant);
        }
        return riemann_zeta_value;
    }

    int64_t ZetaWorkloadWrapper::getKeyrankFromKeystr_(const std::string& keystr)
    {
        const uint32_t keyrank_bytecnt = sizeof(int64_t); // 8B
        assert(keystr.length() >= keyrank_bytecnt);

        // Copy the first 8B to keyrank
        int64_t tmp_keyrank = 0;
        memcpy((char *)&tmp_keyrank, keystr.c_str(), keyrank_bytecnt);
        assert(tmp_keyrank >= 1);

        return tmp_keyrank;
    }

    void ZetaWorkloadWrapper::initWorkloadParameters_()
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

                client_worker_reqdist_ptrs_[tmp_local_client_worker_idx] = new std::discrete_distribution<uint32_t>(dataset_probs_.begin(), dataset_probs_.end());
                assert(client_worker_reqdist_ptrs_[tmp_local_client_worker_idx] != NULL);
            }
        }

        return;
    }

    void ZetaWorkloadWrapper::overwriteWorkloadParameters_()
    {
        // Do nothing

        return;
    }

    void ZetaWorkloadWrapper::createWorkloadGenerator_()
    {
        // NOT need pre-generated workload items for approximate workload distribution due to directly generating workload items by Zeta distribution

        return;
    }

    // (2) Common utilities

    void ZetaWorkloadWrapper::checkPointers_() const
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

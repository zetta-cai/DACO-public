#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "workload/akamai_workload_wrapper.h"
#include "workload/facebook_workload_wrapper.h"
#include "workload/fbphoto_workload_wrapper.h"
#include "workload/wikipedia_workload_wrapper.h"
#include "workload/zeta_workload_wrapper.h"
#include "workload/zipf_workload_wrapper.h"
#include "workload/zipf_facebook_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_PREPROCESSOR("preprocessor");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_LOADER("loader");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLIENT("client");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD("cloud");

    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == Util::AKAMAI_WEB_WORKLOAD_NAME || workload_name == Util::AKAMAI_VIDEO_WORKLOAD_NAME) // Akamai's web/video trace
        {
            workload_ptr = new AkamaiWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::FACEBOOK_WORKLOAD_NAME) // Facebook/Meta CDN
        {
            workload_ptr = new FacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::FBPHOTO_WORKLOAD_NAME) // (OBSOLETE due to not open-sourced and hence NO total frequency information for probability calculation and curvefitting) Facebook photo caching
        {
            workload_ptr = new FbphotoWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::WIKIPEDIA_TEXT_WORKLOAD_NAME) // (OBSOLETE due to no geographical information) Wiki image/text CDN
        {
            workload_ptr = new WikipediaWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::ZIPF_FACEBOOK_WORKLOAD_NAME) // Zipf-based Facebook CDN (based on power law; using key/value size distribution in cachebench)
        {
            assert(zipf_alpha > 0.0);
            workload_ptr = new ZipfFacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, zipf_alpha, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::ZETA_WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::ZETA_WIKIPEDIA_TEXT_WORKLOAD_NAME || workload_name == Util::ZETA_TENCENT_PHOTO1_WORKLOAD_NAME || workload_name == Util::ZETA_TENCENT_PHOTO2_WORKLOAD_NAME) // Zipf-based workloads including Wikipedia image/text and Tencent photo caching dataset1/2 (based on zeta distribution; using key/value size distribution in characteristics files)
        {
            UNUSED(zipf_alpha);
            workload_ptr = new ZetaWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, zipf_alpha, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
        }
        else if (workload_name == Util::ZIPF_WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::ZIPF_WIKIPEDIA_TEXT_WORKLOAD_NAME || workload_name == Util::ZIPF_TENCENT_PHOTO1_WORKLOAD_NAME || workload_name == Util::ZIPF_TENCENT_PHOTO2_WORKLOAD_NAME || workload_name == Util::ZIPF_FBPHOTO_WORKLOAD_NAME || workload_name == Util::ZIPF_TWITTERKV2_WORKLOAD_NAME || workload_name == Util::ZIPF_TWITTERKV4_WORKLOAD_NAME) // Zipf-based workloads including Wikipedia image/text and Tencent photo caching dataset1/2 (based on power-law distribution; using key/value size distribution in characteristics files)
        {
            UNUSED(zipf_alpha);
            workload_ptr = new ZipfWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, zipf_alpha, workload_pattern_name, dynamic_change_period, dynamic_change_keycnt, workload_randombase);
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
    // WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint64_t& capacity_bytes, const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role)
    // {
    //     WorkloadWrapperBase* workload_ptr = getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role);
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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const std::string& workload_pattern_name, const uint32_t& dynamic_change_period, const uint32_t& dynamic_change_keycnt, const uint32_t& workload_randombase) : clientcnt_(clientcnt), client_idx_(client_idx), perclient_opcnt_(perclient_opcnt), perclient_workercnt_(perclient_workercnt), keycnt_(keycnt), workload_name_(workload_name), workload_usage_role_(workload_usage_role), workload_randombase_(workload_randombase), workload_pattern_name_(workload_pattern_name), dynamic_change_period_(dynamic_change_period), dynamic_change_keycnt_(dynamic_change_keycnt)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        is_valid_ = false;

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

        curclient_perworker_dynamic_randgen_ptrs_.resize(perclient_workercnt, NULL);
        curclient_perworker_dynamic_dist_ptrs_.resize(perclient_workercnt, NULL);

        dynamic_period_idx_ = 0;
        curclient_perworker_dynamic_rules_rankptrs_.resize(perclient_workercnt);
        curclient_perworker_dynamic_rules_lookup_map_.resize(perclient_workercnt);
        curclient_perworker_dynamic_rules_mapped_keys_.resize(perclient_workercnt);
    }

    WorkloadWrapperBase::~WorkloadWrapperBase()
    {
        if (needWorkloadItems_()) // Clients
        {
            if (Util::isDynamicWorkloadPattern(workload_pattern_name_)) // Dynamic patterns
            {
                for (uint32_t i = 0; i < curclient_perworker_dynamic_randgen_ptrs_.size(); i++)
                {
                    if (curclient_perworker_dynamic_randgen_ptrs_[i] != NULL)
                    {
                        delete curclient_perworker_dynamic_randgen_ptrs_[i];
                        curclient_perworker_dynamic_randgen_ptrs_[i] = NULL;
                    }
                }

                for (uint32_t i = 0; i < curclient_perworker_dynamic_dist_ptrs_.size(); i++)
                {
                    if (curclient_perworker_dynamic_dist_ptrs_[i] != NULL)
                    {
                        delete curclient_perworker_dynamic_dist_ptrs_[i];
                        curclient_perworker_dynamic_dist_ptrs_[i] = NULL;
                    }
                }
            }
        }
    }

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

            // NOTE: key rank information has already been set in the above steps
            prepareForDynamicPatterns_(); // Prepare variables (e.g., randgen and dist to get random keys) for dynamic workload patterns
            initDynamicRules_(); // Initialize dynamic rules for dynamic workload patterns

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(base_instance_name_, "duplicate invoke of validate()!");
        }
        return;
    }

    WorkloadItem WorkloadWrapperBase::generateWorkloadItem(const uint32_t& local_client_worker_idx, bool* is_dynamic_mapped_ptr)
    {
        checkIsValid_();

        assert(needWorkloadItems_()); // Must be clients for evaluation

        WorkloadItem workload_item = generateWorkloadItem_(local_client_worker_idx);

        if (Util::isDynamicWorkloadPattern(workload_pattern_name_)) // Dynamic patterns
        {
            applyDynamicRules_(local_client_worker_idx, workload_item, is_dynamic_mapped_ptr);
        }

        return workload_item;
    }

    // Access by single thread of client wrapper, yet contend with multiple client workers for dynamic rules (thread safe)

    void WorkloadWrapperBase::updateDynamicRules()
    {
        checkIsValid_();
        
        checkDynamicPatterns_();

        // Acquire a write lock without blocking
        while (true)
        {
            bool result = dynamic_rwlock_.try_lock();
            if (result)
            {
                break;
            }
        }

        if (workload_pattern_name_ == Util::HOTIN_WORKLOAD_PATTERN_NAME) // Hot-in dynamic pattern
        {
            updateDynamicRulesForHotin_();
        }
        else if (workload_pattern_name_ == Util::HOTOUT_WORKLOAD_PATTERN_NAME) // Hot-out dynamic pattern
        {
            updateDynamicRulesForHotout_();
        }
        else if (workload_pattern_name_ == Util::RANDOM_WORKLOAD_PATTERN_NAME) // Random dynamic pattern
        {
            updateDynamicRulesForRandom_();
        }
        else
        {
            std::ostringstream oss;
            oss << "workload pattern " << workload_pattern_name_ << " is not supported for dynamic rules update!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Release a write lock
        dynamic_rwlock_.unlock();

        return;
    }

    // Access by the single thread of client wrapper (NO need to be thread safe)

    void WorkloadWrapperBase::prepareForDynamicPatterns_()
    {
        if (needWorkloadItems_()) // Clients
        {
            if (Util::isDynamicWorkloadPattern(workload_pattern_name_)) // Dynamic patterns
            {
                for (uint32_t tmp_local_client_worker_idx = 0; tmp_local_client_worker_idx < perclient_workercnt_; tmp_local_client_worker_idx++)
                {
                    // Create random generators to get random keys for dynamic workload patterns
                    uint32_t tmp_global_client_worker_idx = Util::getGlobalClientWorkerIdx(getClientIdx_(), tmp_local_client_worker_idx, getPerclientWorkercnt_());
                    std::mt19937_64* tmp_client_worker_dynamic_randgen_ptr_ = new std::mt19937_64(tmp_global_client_worker_idx + workload_randombase_);
                    if (tmp_client_worker_dynamic_randgen_ptr_ == NULL)
                    {
                        Util::dumpErrorMsg(base_instance_name_, "failed to create a random generator for dynamic workload patterns!");
                        exit(1);
                    }
                    curclient_perworker_dynamic_randgen_ptrs_[tmp_local_client_worker_idx] = tmp_client_worker_dynamic_randgen_ptr_;

                    // // (OBSOLETE) Create uniform distribution to get random keys within [0, largest_rank] for dynamic workload patterns
                    // // NOTE: rank information has already been set in validate() before this function
                    // const uint32_t tmp_largest_rank = getLargestRank_(tmp_local_client_worker_idx);
                    // curclient_perworker_dynamic_dist_ptrs_[tmp_local_client_worker_idx] = new std::uniform_int_distribution<uint32_t>(0, tmp_largest_rank);

                    // Create uniform distribution to get random keys within [0, dynamic_rulecnt - 1] for dynamic workload patterns
                    curclient_perworker_dynamic_dist_ptrs_[tmp_local_client_worker_idx] = new std::uniform_int_distribution<uint32_t>(0, Config::getDynamicRulecnt() - 1);
                }
            }
        }
        return;
    }

    void WorkloadWrapperBase::initDynamicRules_()
    {
        if (needWorkloadItems_()) // Clients
        {
            if (Util::isDynamicWorkloadPattern(workload_pattern_name_)) // Dynamic patterns
            {
                const uint32_t dynamic_rulecnt = Config::getDynamicRulecnt();
                assert(dynamic_rulecnt > 0);

                for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
                {
                    // Get top-DYNAMIC_RULECNT hottest keys
                    std::vector<std::string> hottest_keys;
                    getRankedKeys_(local_client_worker_idx, 0, dynamic_rulecnt, hottest_keys);

                    // Initialize period index for the first time of updating dynamic rules later
                    dynamic_period_idx_ = 0;

                    // Initialize mapped keys for dynamic rules
                    dynamic_rules_mapped_keys_t& tmp_mapped_keys_ref = curclient_perworker_dynamic_rules_mapped_keys_[local_client_worker_idx];
                    for (uint32_t i = 0; i < hottest_keys.size(); i++)
                    {
                        tmp_mapped_keys_ref.push_back(hottest_keys[i]);
                    }

                    // Initialize lookup map and rank pointers for dynamic rules (each original key is mapped to itself at first)
                    dynamic_rules_mapped_keys_t::iterator tmp_mapped_keys_iter = tmp_mapped_keys_ref.begin();
                    for (uint32_t i = 0; i < hottest_keys.size(); i++)
                    {
                        dynamic_rules_lookup_map_t::iterator tmp_lookup_map_iter = curclient_perworker_dynamic_rules_lookup_map_[local_client_worker_idx].insert(std::make_pair(hottest_keys[i], tmp_mapped_keys_iter)).first;
                        curclient_perworker_dynamic_rules_rankptrs_[local_client_worker_idx].push_back(tmp_lookup_map_iter);
                        tmp_mapped_keys_iter++;
                    }
                }
            }
        }

        return;
    }

    // Access by multiple client workers (thread safe)

    void WorkloadWrapperBase::applyDynamicRules_(const uint32_t& local_client_worker_idx, WorkloadItem& workload_item, bool* is_dynamic_mapped_ptr)
    {
        checkIsValid_();

        checkDynamicPatterns_();

        Key& workload_key_ref = workload_item.getKeyRef();
        const std::string original_keystr = workload_key_ref.getKeystr();

        // Init is_dynamic_mapped_ptr if necessary
        if (is_dynamic_mapped_ptr != nullptr)
        {
            *is_dynamic_mapped_ptr = false;
        }

        // Acquire a read lock without blocking
        while (true)
        {
            bool result = dynamic_rwlock_.try_lock_shared();
            if (result)
            {
                break;
            }
        }

        // Try to convert the key in workload_item according to dynamic rules
        const dynamic_rules_lookup_map_t& tmp_lookup_map_const_ref = curclient_perworker_dynamic_rules_lookup_map_[local_client_worker_idx];
        dynamic_rules_lookup_map_t::const_iterator tmp_lookup_map_const_iter = tmp_lookup_map_const_ref.find(original_keystr);
        // NOTE: Do NOT convert the generated key if it does not exist in the dynamic rules
        if (tmp_lookup_map_const_iter != tmp_lookup_map_const_ref.end()) // Corresponding dynamic rule exists
        {
            // Get mapped key
            dynamic_rules_mapped_keys_t::iterator tmp_mapped_keys_iter = tmp_lookup_map_const_iter->second;

            // Update is_dynamic_mapped_ptr if necessary
            if (is_dynamic_mapped_ptr != nullptr && original_keystr != *tmp_mapped_keys_iter)
            {
                *is_dynamic_mapped_ptr = true;
            }

            // Replace original key with mapped key
            workload_key_ref = Key(*tmp_mapped_keys_iter); // Update the key from original one as mapped one in workload_item
        }

        // Release the read lock
        dynamic_rwlock_.unlock_shared();

        return;
    }

    // Access by single thread of client wrapper, yet contend with multiple client workers for dynamic rules (thread safe)
    
    void WorkloadWrapperBase::updateDynamicRulesForHotin_()
    {
        checkIsValid_();
        
        checkDynamicPatterns_();

        assert(workload_pattern_name_ == Util::HOTIN_WORKLOAD_PATTERN_NAME); // Hot-in dynamic pattern

        // NOTE: NO need to acquire the rwlock for dynamic rules update, which has been done in updateDynamicRules()

        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            const uint32_t largest_rank = getLargestRank_(local_client_worker_idx);

            // Calculate start rank
            int tmp_start_rank = static_cast<int>(largest_rank);
            tmp_start_rank = tmp_start_rank - static_cast<int>((dynamic_period_idx_ + 1) * dynamic_change_keycnt_) + 1;
            if (tmp_start_rank < 0)
            {
                tmp_start_rank += static_cast<int>(largest_rank + 1);
            }
            uint32_t start_rank = static_cast<uint32_t>(tmp_start_rank);

            // Get coldest keys ranked in [start_rank, start_rank + dynamic_change_keycnt_ - 1]
            std::vector<std::string> coldest_keys;
            getRankedKeys_(local_client_worker_idx, start_rank, dynamic_change_keycnt_, coldest_keys);

            // Update mapped keys of dynamic rules
            dynamic_rules_mapped_keys_t& tmp_mapped_keys_ref = curclient_perworker_dynamic_rules_mapped_keys_[local_client_worker_idx];
            assert(tmp_mapped_keys_ref.size() >= dynamic_change_keycnt_);
            for (uint32_t i = 0; i < dynamic_change_keycnt_; i++) // Pop the last dynamic_change_keycnt_ mapped keys
            {
                tmp_mapped_keys_ref.pop_back();
            }
            for (int i = coldest_keys.size() - 1; i >= 0; i--) // Push coldest keys as the first dynamic_change_keycnt_ mapped keys
            {
                tmp_mapped_keys_ref.push_front(coldest_keys[i]);
            }
            assert(tmp_mapped_keys_ref.size() == Config::getDynamicRulecnt());

            // Update lookup map of dynamic rules
            std::vector<dynamic_rules_lookup_map_t::iterator>& tmp_rankptrs_ref = curclient_perworker_dynamic_rules_rankptrs_[local_client_worker_idx];
            assert(tmp_rankptrs_ref.size() == Config::getDynamicRulecnt());
            dynamic_rules_mapped_keys_t::iterator tmp_mapped_keys_iter = tmp_mapped_keys_ref.begin();
            for (uint32_t i = 0; i < tmp_rankptrs_ref.size(); i++)
            {
                dynamic_rules_lookup_map_t::iterator tmp_lookup_map_iter = tmp_rankptrs_ref[i];
                tmp_lookup_map_iter->second = tmp_mapped_keys_iter;
                tmp_mapped_keys_iter++;
            }
        }

        // Update dynamic period index
        dynamic_period_idx_ += 1;

        return;
    }

    void WorkloadWrapperBase::updateDynamicRulesForHotout_()
    {
        checkIsValid_();
        
        checkDynamicPatterns_();

        assert(workload_pattern_name_ == Util::HOTOUT_WORKLOAD_PATTERN_NAME); // Hot-out dynamic pattern

        // NOTE: NO need to acquire the rwlock for dynamic rules update, which has been done in updateDynamicRules()

        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            const uint32_t largest_rank = getLargestRank_(local_client_worker_idx);

            // Calculate start rank
            uint32_t start_rank = Config::getDynamicRulecnt() + dynamic_period_idx_ * dynamic_change_keycnt_;
            if (start_rank > largest_rank)
            {
                start_rank -= (largest_rank + 1);
            }

            // Get lest-hottest keys ranked in [start_rank, start_rank + dynamic_change_keycnt_ - 1]
            std::vector<std::string> less_hottest_keys;
            getRankedKeys_(local_client_worker_idx, start_rank, dynamic_change_keycnt_, less_hottest_keys);

            // Update mapped keys of dynamic rules
            dynamic_rules_mapped_keys_t& tmp_mapped_keys_ref = curclient_perworker_dynamic_rules_mapped_keys_[local_client_worker_idx];
            assert(tmp_mapped_keys_ref.size() >= dynamic_change_keycnt_);
            for (uint32_t i = 0; i < dynamic_change_keycnt_; i++) // Pop the first dynamic_change_keycnt_ mapped keys
            {
                tmp_mapped_keys_ref.pop_front();
            }
            for (uint32_t i = 0; i < less_hottest_keys.size(); i++) // Push less-hottest keys as the last dynamic_change_keycnt_ mapped keys
            {
                tmp_mapped_keys_ref.push_back(less_hottest_keys[i]);
            }
            assert(tmp_mapped_keys_ref.size() == Config::getDynamicRulecnt());

            // Update lookup map of dynamic rules
            std::vector<dynamic_rules_lookup_map_t::iterator>& tmp_rankptrs_ref = curclient_perworker_dynamic_rules_rankptrs_[local_client_worker_idx];
            assert(tmp_rankptrs_ref.size() == Config::getDynamicRulecnt());
            dynamic_rules_mapped_keys_t::iterator tmp_mapped_keys_iter = tmp_mapped_keys_ref.begin();
            for (uint32_t i = 0; i < tmp_rankptrs_ref.size(); i++)
            {
                dynamic_rules_lookup_map_t::iterator tmp_lookup_map_iter = tmp_rankptrs_ref[i];
                tmp_lookup_map_iter->second = tmp_mapped_keys_iter;
                tmp_mapped_keys_iter++;
            }
        }

        // Update dynamic period index
        dynamic_period_idx_ += 1;

        return;
    }

    void WorkloadWrapperBase::updateDynamicRulesForRandom_()
    {
        checkIsValid_();
        
        checkDynamicPatterns_();

        assert(workload_pattern_name_ == Util::RANDOM_WORKLOAD_PATTERN_NAME); // Random dynamic pattern

        // NOTE: NO need to acquire the rwlock for dynamic rules update, which has been done in updateDynamicRules()

        for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
        {
            const uint32_t largest_rank = getLargestRank_(local_client_worker_idx);

            // Calculate start rank
            int tmp_start_rank = static_cast<int>(largest_rank);
            tmp_start_rank = tmp_start_rank - static_cast<int>((dynamic_period_idx_ + 1) * dynamic_change_keycnt_) + 1;
            if (tmp_start_rank < 0)
            {
                tmp_start_rank += static_cast<int>(largest_rank + 1);
            }
            uint32_t start_rank = static_cast<uint32_t>(tmp_start_rank);

            // Get coldest keys ranked in [start_rank, start_rank + dynamic_change_keycnt_ - 1]
            std::vector<std::string> coldest_keys;
            getRankedKeys_(local_client_worker_idx, start_rank, dynamic_change_keycnt_, coldest_keys);

            // Get random indexes within [0, dynamic_rulecnt - 1] to replace
            std::vector<uint32_t> random_indexes;
            getRandomIdxes_(local_client_worker_idx, dynamic_change_keycnt_, random_indexes);

            // Update mapped keys of dynamic rules (NOTE: in-place updates in mapped keys and hence NO need to update the lookup map)
            assert(coldest_keys.size() == random_indexes.size());
            std::vector<dynamic_rules_lookup_map_t::iterator>& tmp_rankptrs_ref = curclient_perworker_dynamic_rules_rankptrs_[local_client_worker_idx];
            for (uint32_t i = 0; i < dynamic_change_keycnt_; i++)
            {
                const uint32_t tmp_random_index = random_indexes[i];
                dynamic_rules_lookup_map_t::iterator tmp_lookup_map_iter = tmp_rankptrs_ref[tmp_random_index];
                dynamic_rules_mapped_keys_t::iterator tmp_mapped_keys_iter = tmp_lookup_map_iter->second;
                *tmp_mapped_keys_iter = coldest_keys[i]; // Update the mapped key of the original key ranked at tmp_random_index with the corresponding coldest key
            }
        }

        // Update dynamic period index
        dynamic_period_idx_ += 1;

        return;
    }

    // Utility functions for dynamic workload patterns

    void WorkloadWrapperBase::checkDynamicPatterns_() const
    {
        assert(needWorkloadItems_()); // Clients
        assert(Util::isDynamicWorkloadPattern(workload_pattern_name_)); // Dynamic patterns
        return;
    }

    void WorkloadWrapperBase::getRankedIdxes_(const uint32_t local_client_worker_idx, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<uint32_t>& ranked_idxes) const
    {
        checkDynamicPatterns_();

        // Check start_rank
        const uint32_t largest_rank = getLargestRank_(local_client_worker_idx);
        if (start_rank < 0 || start_rank > largest_rank)
        {
            std::ostringstream oss;
            oss << "invalid start_rank " << start_rank << " for largest_rank " << largest_rank << "!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Get indexes of [start_rank, start_rank + ranked_keycnt - 1] within the range of [0, largest_rank]
        ranked_idxes.clear();
        for (uint32_t i = 0; i < ranked_keycnt; i++)
        {
            const uint32_t tmp_ranked_idx = (start_rank + i) % (largest_rank + 1);
            ranked_idxes.push_back(tmp_ranked_idx);
        }

        return;
    }

    void WorkloadWrapperBase::getRandomIdxes_(const uint32_t local_client_worker_idx, const uint32_t random_keycnt, std::vector<uint32_t>& random_idxes) const
    {
        checkDynamicPatterns_();

        // Get the random generator
        assert(local_client_worker_idx < curclient_perworker_dynamic_randgen_ptrs_.size());
        std::mt19937_64* tmp_randgen_ptr = curclient_perworker_dynamic_randgen_ptrs_[local_client_worker_idx];
        assert(tmp_randgen_ptr != NULL);

        // Get the uniform distribution
        assert(local_client_worker_idx < curclient_perworker_dynamic_dist_ptrs_.size());
        std::uniform_int_distribution<uint32_t>* tmp_dist_ptr = curclient_perworker_dynamic_dist_ptrs_[local_client_worker_idx];
        assert(tmp_dist_ptr != NULL);

        // Get random indexes within [0, dynamic_rulecnt - 1] without duplication
        std::unordered_set<uint32_t> tmp_random_idxes_set;
        while (tmp_random_idxes_set.size() < random_keycnt)
        {
            const uint32_t tmp_rand_idx = (*tmp_dist_ptr)(*tmp_randgen_ptr);
            tmp_random_idxes_set.insert(tmp_rand_idx);
        }

        // Set random indexes
        random_idxes.clear();
        for (std::unordered_set<uint32_t>::const_iterator it = tmp_random_idxes_set.begin(); it != tmp_random_idxes_set.end(); it++)
        {
            random_idxes.push_back(*it);
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
        // For clients or dataset loaders or cloud
        assert(workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT || workload_usage_role_ == WORKLOAD_USAGE_ROLE_LOADER || workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLOUD);

        // ONLY clients use perclient_workercnt pre-generated workload item lists
        if (workload_usage_role_ == WORKLOAD_USAGE_ROLE_CLIENT)
        {
            assert(perclient_workercnt_ > 0);
        }
        
        return perclient_workercnt_;
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

    const uint32_t WorkloadWrapperBase::getWorkloadRandombase_() const
    {
        // For all roles
        return workload_randombase_;
    }

    const std::string WorkloadWrapperBase::getWorkloadPatternName_() const
    {
        // For dynamic workload patterns
        return workload_pattern_name_;
    }

    const uint32_t WorkloadWrapperBase::getDynamicChangePeriod_() const
    {
        // For dynamic workload patterns
        return dynamic_change_period_;
    }

    const uint32_t WorkloadWrapperBase::getDynamicChangeKeycnt_() const
    {
        // For dynamic workload patterns
        return dynamic_change_keycnt_;
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
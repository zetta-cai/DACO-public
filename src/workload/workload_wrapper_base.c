#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"
#include "workload/fbphoto_workload_wrapper.h"
#include "workload/wikipedia_workload_wrapper.h"
#include "workload/zeta_workload_wrapper.h"
#include "workload/zipf_facebook_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_PREPROCESSOR("preprocessor");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_LOADER("loader");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLIENT("client");
    const std::string WorkloadWrapperBase::WORKLOAD_USAGE_ROLE_CLOUD("cloud");

    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role, const float& zipf_alpha)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == Util::FACEBOOK_WORKLOAD_NAME) // Facebook/Meta CDN
        {
            workload_ptr = new FacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role);
        }
        else if (workload_name == Util::FBPHOTO_WORKLOAD_NAME) // (OBSOLETE due to not open-sourced and hence NO total frequency information for probability calculation and curvefitting) Facebook photo caching
        {
            workload_ptr = new FbphotoWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role);
        }
        else if (workload_name == Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::WIKIPEDIA_TEXT_WORKLOAD_NAME) // (OBSOLETE due to no geographical information) Wiki image/text CDN
        {
            workload_ptr = new WikipediaWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role);
        }
        else if (workload_name == Util::ZIPF_FACEBOOK_WORKLOAD_NAME) // Zipf-based Facebook CDN (based on power law; using key/value size distribution in cachebench)
        {
            assert(zipf_alpha > 0.0);
            workload_ptr = new ZipfFacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, zipf_alpha);
        }
        else if (workload_name == Util::ZETA_WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == Util::ZETA_WIKIPEDIA_TEXT_WORKLOAD_NAME || workload_name == Util::ZETA_TENCENT_PHOTO1_WORKLOAD_NAME || workload_name == Util::ZETA_TENCENT_PHOTO2_WORKLOAD_NAME) // Zipf-based workloads including Wikipedia image/text and Tencent photo caching dataset1/2 (based on zeta distribution; using key/value size distribution in characteristics files)
        {
            UNUSED(zipf_alpha);
            workload_ptr = new ZetaWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, workload_usage_role, zipf_alpha);
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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name, const std::string& workload_usage_role) : clientcnt_(clientcnt), client_idx_(client_idx), perclient_opcnt_(perclient_opcnt), perclient_workercnt_(perclient_workercnt), keycnt_(keycnt), workload_name_(workload_name), workload_usage_role_(workload_usage_role)
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
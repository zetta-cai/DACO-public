#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

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
            workload_ptr = new FacebookWorkloadWrapper(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_usage_role, max_eval_workload_loadcnt);
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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& perclient_opcnt, const uint32_t& perclient_workercnt, const std::string& workload_usage_role, const uint32_t& max_eval_workload_loadcnt) : clientcnt_(clientcnt), client_idx_(client_idx), keycnt_(keycnt), perclient_opcnt_(perclient_opcnt), perclient_workercnt_(perclient_workercnt), workload_usage_role_(workload_usage_role), max_eval_workload_loadcnt_(max_eval_workload_loadcnt)
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
#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/param/workload_param.h"
#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == WorkloadParam::FACEBOOK_WORKLOAD_NAME)
        {
            workload_ptr = new FacebookWorkloadWrapper(clientcnt, client_idx, keycnt, opcnt, perclient_workercnt);
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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& client_idx) : client_idx_(client_idx)
    {
        // Differentiate workload generator in different clients
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx;
        base_instance_name_ = oss.str();

        is_valid_ = false;
    }

    WorkloadWrapperBase::~WorkloadWrapperBase() {}

    void WorkloadWrapperBase::validate()
    {
        if (!is_valid_)
        {
            initWorkloadParameters_();
            overwriteWorkloadParameters_();
            createWorkloadGenerator_(client_idx_);

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(base_instance_name_, "duplicate invoke of validate()!");
        }
        return;
    }

    WorkloadItem WorkloadWrapperBase::generateItem(std::mt19937_64& request_randgen)
    {
        checkIsValid_();
        return generateItemInternal_(request_randgen);
    }

    void WorkloadWrapperBase::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(base_instance_name_, "not invoke validate() yet!");
            exit(1);
        }
        return;
    }
}
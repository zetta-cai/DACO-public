#include "workload/workload_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "workload/facebook_workload_wrapper.h"

namespace covered
{
    const std::string WorkloadWrapperBase::kClassName("WorkloadWrapperBase");

    WorkloadWrapperBase* WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name)
    {
        WorkloadWrapperBase* workload_ptr = NULL;
        if (workload_name == Util::FACEBOOK_WORKLOAD_NAME)
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

    WorkloadWrapperBase::WorkloadWrapperBase(const uint32_t& clientcnt, const uint32_t& client_idx, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt) : clientcnt_(clientcnt), client_idx_(client_idx), keycnt_(keycnt), opcnt_(opcnt), perclient_workercnt_(perclient_workercnt)
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
            createWorkloadGenerator_();

            is_valid_ = true;
        }
        else
        {
            Util::dumpWarnMsg(base_instance_name_, "duplicate invoke of validate()!");
        }
        return;
    }

    WorkloadItem WorkloadWrapperBase::generateWorkloadItem(std::mt19937_64& request_randgen)
    {
        checkIsValid_();
        return generateWorkloadItemInternal_(request_randgen);
    }

    uint32_t WorkloadWrapperBase::getKeycnt() const
    {
        return keycnt_;
    }

    WorkloadItem WorkloadWrapperBase::getDatasetItem(const uint32_t itemidx)
    {
        checkIsValid_();

        assert(itemidx < keycnt_);
        getDatasetItemInternal_(itemidx);
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
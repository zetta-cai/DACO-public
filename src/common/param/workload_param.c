#include "common/param/workload_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string WorkloadParam::FACEBOOK_WORKLOAD_NAME("facebook");

    const std::string WorkloadParam::kClassName("WorkloadParam");

    bool WorkloadParam::is_valid_ = false;

    uint32_t WorkloadParam::keycnt_ = 0;
    std::string WorkloadParam::workload_name_ = "";

    void WorkloadParam::setParameters(const uint32_t& keycnt, const std::string& workload_name)
    {
        // NOTE: NOT rely on any other module
        if (is_valid_)
        {
            return; // NO need to set parameters once again
        }
        else
        {
            Util::dumpNormalMsg(kClassName, "invoke setParameters()!");
        }

        keycnt_ = keycnt;
        workload_name_ = workload_name;
        checkWorkloadName_();

        is_valid_ = true;
        return;
    }

    uint32_t WorkloadParam::getKeycnt()
    {
        checkIsValid_();
        return keycnt_;
    }

    std::string WorkloadParam::getWorkloadName()
    {
        checkIsValid_();
        return workload_name_;
    }

    std::string WorkloadParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Key count (dataset size): " << keycnt_ << std::endl;
        oss << "Workload name: " << workload_name_;

        return oss.str();  
    }

    void WorkloadParam::checkWorkloadName_()
    {
        if (workload_name_ != FACEBOOK_WORKLOAD_NAME)
        {
            std::ostringstream oss;
            oss << "workload name " << workload_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void WorkloadParam::verifyIntegrity_()
    {
        assert(keycnt_ > 0);

        return;
    }

    void WorkloadParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid WorkloadParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
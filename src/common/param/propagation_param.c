#include "common/param/propagation_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string PropagationParam::kClassName("PropagationParam");

    bool PropagationParam::is_valid_ = false;

    uint32_t PropagationParam::propagation_latency_clientedge_us_ = 0;
    uint32_t PropagationParam::propagation_latency_crossedge_us_ = 0;
    uint32_t PropagationParam::propagation_latency_edgecloud_us_ = 0;

    void PropagationParam::setParameters(const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us)
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

        propagation_latency_clientedge_us_ = propagation_latency_clientedge_us;
        propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
        propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;

        is_valid_ = true;
        return;
    }

    uint32_t PropagationParam::getPropagationLatencyClientedgeUs()
    {
        checkIsValid_();
        return propagation_latency_clientedge_us_;
    }

    uint32_t PropagationParam::getPropagationLatencyCrossedgeUs()
    {
        checkIsValid_();
        return propagation_latency_crossedge_us_;
    }

    uint32_t PropagationParam::getPropagationLatencyEdgecloudUs()
    {
        checkIsValid_();
        return propagation_latency_edgecloud_us_;
    }

    std::string PropagationParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "One-way propagation latency between client and edge: " << propagation_latency_clientedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and edge: " << propagation_latency_crossedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and cloud: " << propagation_latency_edgecloud_us_ << "us";

        return oss.str();  
    }

    void PropagationParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid PropagationParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
#include "common/param/evaluator_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string EvaluatorParam::kClassName("EvaluatorParam");

    bool EvaluatorParam::is_valid_ = false;

    uint32_t EvaluatorParam::max_warmup_duration_sec_ = 0;
    uint32_t EvaluatorParam::stresstest_duration_sec_ = 0;

    void EvaluatorParam::setParameters(const uint32_t& max_warmup_duration_sec, const uint32_t& stresstest_duration_sec)
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

        max_warmup_duration_sec_ = max_warmup_duration_sec;
        stresstest_duration_sec_ = stresstest_duration_sec;

        is_valid_ = true;
        return;
    }

    uint32_t EvaluatorParam::getMaxWarmupDurationSec()
    {
        checkIsValid_();
        return max_warmup_duration_sec_;
    }

    uint32_t EvaluatorParam::getStresstestDurationSec()
    {
        checkIsValid_();
        return stresstest_duration_sec_;
    }

    std::string EvaluatorParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Max warmup duration seconds: " << max_warmup_duration_sec_ << std::endl;
        oss << "Stresstest duration seconds: " << stresstest_duration_sec_;

        return oss.str();  
    }

    void EvaluatorParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid EvaluatorParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
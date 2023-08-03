/*
 * EvaluatorParam: store CLI parameters for evaluator dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EVALUATOR_PARAM_H
#define EVALUATOR_PARAM_H

#include <string>

namespace covered
{
    class EvaluatorParam
    {
    public:
        static void setParameters(const uint32_t& max_warmup_duration_sec, const uint32_t& stresstest_duration_sec);

        static uint32_t getMaxWarmupDurationSec();
        static uint32_t getStresstestDurationSec();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid_();

        static bool is_valid_;

        static uint32_t max_warmup_duration_sec_;
        static uint32_t stresstest_duration_sec_;
    };
}

#endif
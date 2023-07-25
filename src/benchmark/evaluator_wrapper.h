/*
 * EvaluatorWrapper: control the evaluation phases during benchmark.
 * 
 * By Siyuan Sheng (2023.07.25).
 */

#ifndef EVALUATOR_WRAPPER_H
#define EVALUATOR_WRAPPER_H

#include <string>

#include "statistics/total_statistics_tracker.h"

namespace covered
{
    class EvaluatorWrapper
    {
    public:
        static void* launchEvaluatorWrapper(void* is_evaluator_initialized_ptr);

        EvaluatorWrapper();
        ~EvaluatorWrapper();

        void start();
    private:
        static const std::string kClassName;

        bool is_warmup_phase_;
        TotalStatisticsTracker* total_statistics_tracker_ptr_;
    };
}

#endif
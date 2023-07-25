#include "benchmark/evaluator_wrapper.h"

#include <assert.h>

namespace covered
{
    const std::string EvaluatorWrapper::kClassName("EvaluatorWrapper");

    void* EvaluatorWrapper::launchEvaluatorWrapper(void* is_evaluator_initialized_ptr)
    {
        bool& is_evaluator_initialized = *(static_cast<bool*>(is_evaluator_initialized_ptr));

        EvaluatorWrapper evaluator;
        is_evaluator_initialized = true; // Such that simulator or prototype will continue to launch cloud, edge, and client nodes

        evaluator.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EvaluatorWrapper::EvaluatorWrapper()
    {
        is_warmup_phase_ = true;

        // TODO
        total_statistics_tracker_ptr_ = new TotalStatisticsTracker();
        assert(total_statistics_tracker_ptr_ != NULL);
    }

    EvaluatorWrapper::~EvaluatorWrapper()
    {
        assert(total_statistics_tracker_ptr_ != NULL);
        delete total_statistics_tracker_ptr_;
        total_statistics_tracker_ptr_ = NULL;
    }

    void EvaluatorWrapper::start()
    {
        // TODO
    }
}
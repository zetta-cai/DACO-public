/*
 * SubthreadParamBase: base class for parameters to launch a subthread.
 * 
 * By Siyuan Sheng (2024.03.02).
 */

#ifndef SUBTHREAD_PARAM_BASE_H
#define SUBTHREAD_PARAM_BASE_H

#include <atomic>
#include <string>

namespace covered
{
    class SubthreadParamBase
    {
    public:
        static const uint32_t INITIALIZATION_WAIT_INTERVAL_US;

        SubthreadParamBase();
        ~SubthreadParamBase();

        const SubthreadParamBase& operator=(const SubthreadParamBase& other);

        bool isFinishInitialization() const;
        void markFinishInitialization();
    private:
        static const std::string kClassName;

        volatile std::atomic<bool> is_finish_initialization_;
    };
}

#endif
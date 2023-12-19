/*
 * Thread launcher: launch high-priority or low-priority threads w/ CPU binding.
 * 
 * By Siyuan Sheng (2023.11.08).
 */

#ifndef THREAD_LAUNCHER_H
#define THREAD_LAUNCHER_H

#define DEBUG_THREAD_LAUNCHER

#include <atomic>
#include <sched.h> // sched_param, cpu_set_t, CPU_ZERO, CPU_SET
#include <string>

#include "common/covered_common_header.h"

namespace covered
{
    class ThreadLauncher
    {
    public:
        static const int SCHEDULING_POLICY; // Task scheduling policy

        // NOTE: we do NOT use "const std::string&" here to avoid multiple threads from accessing the same std::string
        static void bindMainThreadToSharedCpuCore(const std::string main_thread_name); // Bind main thread to a shared CPU core (this will increase low_priority_threadcnt_)
        static void pthreadCreateLowPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr);
        #ifdef ENABLE_STRICT_CPU_BINDING
        static uint32_t pthreadCreateHighPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr, const uint32_t* specified_cpuidx_ptr = NULL); // Return the CPU core index that the high-priority thread is bound to
        #else
        static void pthreadCreateHighPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr);
        #endif
    private:
        static const std::string kClassName;

        static void preparePthreadAttr_(pthread_attr_t* attr_ptr);
        static void bindSharedCpuCore_(const std::string thread_name, const pthread_t& tid); // This will increase low_priority_threadcnt_

        #ifdef ENABLE_STRICT_CPU_BINDING
        static uint32_t bindDedicatedCpuCore_(const std::string thread_name, const pthread_t& tid, const uint32_t* specified_cpuidx_ptr = NULL); // This will increase high_priority_threadcnt_
        #else
        static void bindDedicatedCpuCore_(const std::string thread_name, const pthread_t& tid); // This will increase high_priority_threadcnt_
        #endif

        static std::atomic<uint32_t> low_priority_threadcnt_;
        #ifdef ENABLE_STRICT_CPU_BINDING
        static std::atomic<uint32_t> cur_high_priority_cpuidx_; // To calculate cpu idx for each high-priority thread under strick CPU binding
        #endif
        static std::atomic<uint32_t> high_priority_threadcnt_;

        static cpu_set_t cpu_shared_coreset_;
        static cpu_set_t cpu_dedicated_coreset_;
    };
}

#endif
/*
 * Thread launcher: launch high-priority or low-priority threads w/ CPU binding.
 * 
 * By Siyuan Sheng (2023.11.08).
 */

#ifndef THREAD_LAUNCHER_H
#define THREAD_LAUNCHER_H

#define DEBUG_THREAD_LAUNCHER

#include <atomic>
#include <string>

namespace covered
{
    class ThreadLauncher
    {
    public:
        static const int SCHEDULING_POLICY; // Task scheduling policy

        // NOTE: we do NOT use "const std::string&" here to avoid multiple threads from accessing the same std::string
        static void bindMainThreadToSharedCpuCore(const std::string main_thread_name); // Bind main thread to a shared CPU core (this will increase low_priority_threadcnt_)
        static void pthreadCreateLowPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr);
        static void pthreadCreateHighPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr);
    private:
        static const std::string kClassName;

        static void preparePthreadAttr_(pthread_attr_t* attr_ptr);
        static void bindSharedCpuCore_(const std::string thread_name, const pthread_t& tid); // This will increase low_priority_threadcnt_
        static void bindDedicatedCpuCore_(const std::string thread_name, const pthread_t& tid); // This will increase high_priority_threadcnt_

        static std::atomic<uint32_t> low_priority_threadcnt_;
        static std::atomic<uint32_t> high_priority_threadcnt_;
    };
}

#endif
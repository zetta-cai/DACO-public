/*
 * Thread launcher: launch high-priority or low-priority threads w/ CPU binding.
 * 
 * By Siyuan Sheng (2023.11.08).
 */

#ifndef THREAD_LAUNCHER_H
#define THREAD_LAUNCHER_H

#define DEBUG_THREAD_LAUNCHER

#include <atomic>
#include <unordered_map>
#include <sched.h> // sched_param, cpu_set_t, CPU_ZERO, CPU_SET
#include <string>

#include "common/covered_common_header.h"

namespace covered
{
    class DedicatedCoreAssignment
    {
    public:
        DedicatedCoreAssignment();
        DedicatedCoreAssignment(const uint32_t& left_inclusive_dedicated_coreidx, const uint32_t& right_inclusive_dedicated_coreidx);
        DedicatedCoreAssignment(const DedicatedCoreAssignment& other);
        ~DedicatedCoreAssignment();

        bool isAssigned() const;
        uint32_t getLeftInclusiveDedicatedCoreidx() const;
        uint32_t getRightInclusiveDedicatedCoreidx() const;

        uint32_t assignDedicatedCore(); // Return assigned CPU index

        const DedicatedCoreAssignment& operator=(const DedicatedCoreAssignment& other);
    private:
        static const std::string kClassName;

        bool is_assigned_;
        uint32_t left_inclusive_dedicated_coreidx_;
        uint32_t right_inclusive_dedicated_coreidx_;

        // To calculate cpu idx of a high-priority thread for the given thread role under strict CPU binding
        std::atomic<uint32_t> cur_high_priority_cputidx_; // Within the range of [left_inclusive_dedicated_coreidx_, right_inclusive_dedicated_coreidx_] if is_assigned_ is true
    };

    class ThreadLauncher
    {
    public:
        static const std::string CLIENT_THREAD_ROLE;
        static const std::string EDGE_THREAD_ROLE;
        static const std::string CLOUD_THREAD_ROLE;
        static const std::string EVALUATOR_THREAD_ROLE;
        static const std::string DATASET_LOADER_THREAD_ROLE;
        static const std::string TOTAL_STATISTICS_LOADER_THREAD_ROLE;
        static const std::string TRACE_PREPROCESSOR_THREAD_ROLE;

        static const int SCHEDULING_POLICY; // Task scheduling policy

        // For validation

        static void validate(const std::string main_class_name, const uint32_t clientcnt = 0, const uint32_t edgecnt = 0); // Initialize current_machine_dedicated_cores_assignment_, perrole_cur_high_priority_cpuidx_, and is_valid_

        // For low-priority threads

        // NOTE: we do NOT use "const std::string&" here to avoid multiple threads from accessing the same std::string
        static void bindMainThreadToSharedCpuCore(const std::string main_thread_name); // Bind main thread to a shared CPU core (this will increase low_priority_threadcnt_)
        static void pthreadCreateLowPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr);

        // For high-priority threads

        static uint32_t pthreadCreateHighPriority(const std::string thread_role, const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr, const uint32_t* specified_cpuidx_ptr = NULL); // Return the CPU core index that the high-priority thread is bound to
    private:
        static const std::string kClassName;

        // For validation

        static void getPerroleRequiredDedicatedCorecnt_(const std::string main_class_name, const uint32_t clientcnt, const uint32_t edgecnt, std::unordered_map<std::string, uint32_t>& perrole_required_dedicated_corecnt); // Get the required number of dedicated CPU cores for each thread role in the current physical machine

        // Common utilities

        static bool is_valid_;
        static void checkIsValid_();
        static void preparePthreadAttr_(pthread_attr_t* attr_ptr);

        // For low-priority threads

        static void bindSharedCpuCore_(const std::string thread_name, const pthread_t& tid); // This will increase low_priority_threadcnt_

        // For high-priority threads

        static uint32_t bindDedicatedCpuCore_(const std::string thread_role, const std::string thread_name, const pthread_t& tid, const uint32_t* specified_cpuidx_ptr = NULL); // Always increase high_priority_threadcnt_; will increase perrole_cur_high_priority_cpuidx_ if specified_cpuidx_ptr is NULL

        // For low-priority threads

        static std::atomic<uint32_t> low_priority_threadcnt_;
        static cpu_set_t cpu_shared_coreset_;

        // For high-priority threads
        
        // NOTE: NOT maintain such assignment of dedicated cores in Config to avoid the dependency of Config module on clientcnt/edgecnt in CLI module -> Config module should ONLY track static configuration information instead of dynamic one
        static std::unordered_map<std::string, DedicatedCoreAssignment> current_machine_dedicated_core_assignments_; // Assignment of dedicated cores for clients/edges/cloud/evaluator in the current physical machine
        static std::atomic<uint32_t> high_priority_threadcnt_;
    };
}

#endif
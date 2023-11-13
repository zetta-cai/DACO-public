#include "common/thread_launcher.h"

#include <assert.h>
#include <thread> // std::thread::hardware_concurrency

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // TODO: pass nice value into each pthread for SCHED_OTHER
    //const int ThreadLauncher::SCHEDULING_POLICY = SCHED_OTHER; // Default policy used by Linux (nice value: min 19 to max -20), which relies on kernel.sched_latency_ns and kernel.sched_min_granularity_ns
    const int ThreadLauncher::SCHEDULING_POLICY = SCHED_RR; // Round-robin (priority: min 1 to max 99), which relies on /proc/sys/kernel/sched_rr_timeslice_ms

    const std::string ThreadLauncher::kClassName = "ThreadLauncher";

    std::atomic<uint32_t> ThreadLauncher::low_priority_threadcnt_(0);
    std::atomic<uint32_t> ThreadLauncher::high_priority_threadcnt_(0);

    cpu_set_t ThreadLauncher::cpu_shared_coreset_;
    cpu_set_t ThreadLauncher::cpu_dedicated_coreset_;

    void ThreadLauncher::bindMainThreadToSharedCpuCore(const std::string main_thread_name)
    {
        pthread_t main_thread = pthread_self();
        bindSharedCpuCore_(main_thread_name, main_thread);
        return;
    }

    void ThreadLauncher::pthreadCreateLowPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr)
    {
        // Prepare thread attributes
        pthread_attr_t attr;
        preparePthreadAttr_(&attr);

        // Prepare scheduling parameters
        sched_param param;
        int ret = pthread_attr_getschedparam(&attr, &param);
        assert(ret >= 0);
        param.sched_priority = sched_get_priority_min(SCHEDULING_POLICY);
        ret = pthread_attr_setschedparam(&attr, &param);
        assert(ret >= 0);

        // Launch pthread
        int pthread_returncode = pthread_create(tid_ptr, &attr, start_routine, arg_ptr);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch low-priority thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Bind to a shared CPU core
        bindSharedCpuCore_(thread_name, *tid_ptr);

        return;
    }
    
    void ThreadLauncher::pthreadCreateHighPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr)
    {
        // Prepare thread attributes
        pthread_attr_t attr;
        preparePthreadAttr_(&attr);

        // Prepare scheduling parameters
        sched_param param;
        int ret = pthread_attr_getschedparam(&attr, &param);
        assert(ret >= 0);
        param.sched_priority = sched_get_priority_max(SCHEDULING_POLICY);
        ret = pthread_attr_setschedparam(&attr, &param);
        assert(ret >= 0);

        // Launch pthread
        int pthread_returncode = pthread_create(tid_ptr, &attr, start_routine, arg_ptr);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch high-priority thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Bind to a dedicated CPU core
        bindDedicatedCpuCore_(thread_name, *tid_ptr);

        return;
    }

    void ThreadLauncher::preparePthreadAttr_(pthread_attr_t* attr_ptr)
    {
        int ret = 0;

        // Init pthread attr by default
        ret = pthread_attr_init(attr_ptr);
        assert(ret >= 0);

        // Get and print default policy
        /*int policy = 0;
        ret = pthread_attr_getschedpolicy(attr_ptr, &policy);
        assert(ret >= 0);
        if (policy == SCHED_OTHER) // This is the default policy in Ubuntu
        {
            Util::dumpDebugMsg(kClassName, "default policy is SCHED_OTHER");
        }
        else if (policy == SCHED_RR)
        {
            Util::dumpDebugMsg(kClassName, "default policy is SCHED_RR");
        }
        else if (policy == SCHED_FIFO)
        {
            Util::dumpDebugMsg(kClassName, "default policy is SCHED_FIFO");
        }
        else
        {
            std::ostringstream oss;
            oss << "default policy is " << policy;
            Util::dumpDebugMsg(kClassName, oss.str());
        }*/

        // Set scheduling policy
        ret = pthread_attr_setschedpolicy(attr_ptr, SCHEDULING_POLICY);
        assert(ret >= 0);

        return;
    }

    void ThreadLauncher::bindSharedCpuCore_(const std::string thread_name, const pthread_t& tid)
    {
        // Update low-priority thread cnt
        uint32_t original_low_priority_threadcnt = low_priority_threadcnt_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);

        // (i) (OBSOLETE) Bind each low-priority thread to a specific CPU shared core

        /*// Calculate a shared CPU core ID for the low-priority thread
        const PhysicalMachine tmp_physical_machine = Config::getCurrentPhysicalMachine();
        const uint32_t tmp_cpu_dedicated_corecnt = tmp_physical_machine.getCpuDedicatedCorecnt();
        const uint32_t tmp_cpu_shared_corecnt = tmp_physical_machine.getCpuSharedCorecnt();
        uint32_t tmp_cpuidx = tmp_cpu_dedicated_corecnt + original_low_priority_threadcnt % tmp_cpu_shared_corecnt;
        assert(tmp_cpuidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores
        assert(tmp_cpuidx >= tmp_cpu_dedicated_corecnt && tmp_cpuidx < (tmp_cpu_dedicated_corecnt + tmp_cpu_shared_corecnt)); // MUST within the range of [cpu_dedicated_corecnt, cpu_dedicated_corecnt + cpu_shared_corecnt - 1]

        #ifdef DEBUG_THREAD_LAUNCHER
        Util::dumpVariablesForDebug(kClassName, 6, "bind low-priority thread:", thread_name.c_str(), "thread idx:", std::to_string(original_low_priority_threadcnt).c_str(), "cpu core idx:", std::to_string(tmp_cpuidx).c_str());
        #endif

        // Bind to the specific shared CPU core
        cpu_set_t tmp_cpuset;
        CPU_ZERO(&tmp_cpuset);
        CPU_SET(tmp_cpuidx, &tmp_cpuset);
        int pthread_returncode = pthread_setaffinity_np(tid, sizeof(tmp_cpuset), &tmp_cpuset);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to set affinity of thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(-1);
        }*/

        // (ii) Bind each low-priority thread to the set of CPU shared cores

        // Calculate max shared CPU core ID
        const PhysicalMachine tmp_physical_machine = Config::getCurrentPhysicalMachine();
        const uint32_t tmp_cpu_dedicated_corecnt = tmp_physical_machine.getCpuDedicatedCorecnt();
        const uint32_t tmp_cpu_shared_corecnt = tmp_physical_machine.getCpuSharedCorecnt();
        const uint32_t tmp_max_cpuidx = tmp_cpu_dedicated_corecnt + tmp_cpu_shared_corecnt - 1;
        assert(tmp_max_cpuidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores
        assert(tmp_max_cpuidx >= tmp_cpu_dedicated_corecnt && tmp_max_cpuidx < (tmp_cpu_dedicated_corecnt + tmp_cpu_shared_corecnt)); // MUST within the range of [cpu_dedicated_corecnt, cpu_dedicated_corecnt + cpu_shared_corecnt - 1]

        // Initialize cpu_shared_coreset_ if NOT
        if (original_low_priority_threadcnt == 0) // The first low-priority thread
        {
            // Prepare cpu_shared_coreset_ within the range of [cpu_dedicated_corecnt, cpu_dedicated_corecnt + cpu_shared_corecnt - 1]
            CPU_ZERO(&cpu_shared_coreset_);
            for (uint32_t tmp_cpuidx = tmp_cpu_dedicated_corecnt; tmp_cpuidx <= tmp_max_cpuidx; tmp_cpuidx++)
            {
                CPU_SET(tmp_cpuidx, &cpu_shared_coreset_);
            }
        }

        #ifdef DEBUG_THREAD_LAUNCHER
        std::ostringstream oss;
        oss << "[" << tmp_cpu_dedicated_corecnt << ", " << tmp_max_cpuidx << "]";
        Util::dumpVariablesForDebug(kClassName, 6, "bind low-priority thread:", thread_name.c_str(), "thread idx:", std::to_string(original_low_priority_threadcnt).c_str(), "cpu core set:", oss.str().c_str());
        #endif

        // Bind to the set of CPU shared cores
        int pthread_returncode = pthread_setaffinity_np(tid, sizeof(cpu_shared_coreset_), &cpu_shared_coreset_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to set affinity of thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(-1);
        }

        return;
    }

    void ThreadLauncher::bindDedicatedCpuCore_(const std::string thread_name, const pthread_t& tid)
    {
        // Update high-priority thread cnt
        uint32_t original_high_priority_threadcnt = high_priority_threadcnt_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);
        
        #ifdef ENABLE_STRICT_CPU_BINDING
        // NOTE: # of high-priority threads should NOT exceed cpu_dedicated_corecnt
        const PhysicalMachine tmp_physical_machine = Config::getCurrentPhysicalMachine();
        const uint32_t tmp_cpu_dedicated_corecnt = tmp_physical_machine.getCpuDedicatedCorecnt();
        if (original_high_priority_threadcnt >= tmp_cpu_dedicated_corecnt)
        {
            std::ostringstream oss;
            oss << "too many high-priority threads, which exceeds cpu_dedicated_corecnt " << tmp_cpu_dedicated_corecnt;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Calculate a dedicated CPU core ID for the high-priority thread
        uint32_t tmp_cpuidx = original_high_priority_threadcnt % tmp_cpu_dedicated_corecnt;
        assert(tmp_cpuidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores
        assert(tmp_cpuidx < tmp_cpu_dedicated_corecnt); // MUST within the range of [0, cpu_dedicated_corecnt - 1]

        #ifdef DEBUG_THREAD_LAUNCHER
        Util::dumpVariablesForDebug(kClassName, 6, "bind high-priority thread:", thread_name.c_str(), "thread idx:", std::to_string(original_high_priority_threadcnt).c_str(), "cpu core idx:", std::to_string(tmp_cpuidx).c_str());
        #endif

        // Bind to the specific dedicated CPU core
        cpu_set_t tmp_cpuset;
        CPU_ZERO(&tmp_cpuset);
        CPU_SET(tmp_cpuidx, &tmp_cpuset);
        int pthread_returncode = pthread_setaffinity_np(tid, sizeof(tmp_cpuset), &tmp_cpuset);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to set affinity of thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(-1);
        }
        #else
        // Calculate max dedicated CPU core ID
        const PhysicalMachine tmp_physical_machine = Config::getCurrentPhysicalMachine();
        const uint32_t tmp_cpu_dedicated_corecnt = tmp_physical_machine.getCpuDedicatedCorecnt();
        const uint32_t tmp_max_cpuidx = tmp_cpu_dedicated_corecnt - 1; // MUST within the range of [0, cpu_dedicated_corecnt - 1]
        assert(tmp_max_cpuidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores

        // Initialize cpu_dedicated_coreset_ if NOT
        if (original_high_priority_threadcnt == 0) // The first high-priority thread
        {
            // Prepare cpu_shared_coreset_ within the range of [0, cpu_dedicated_corecnt - 1]
            CPU_ZERO(&cpu_dedicated_coreset_);
            for (uint32_t tmp_cpuidx = 0; tmp_cpuidx <= tmp_cpu_dedicated_corecnt - 1; tmp_cpuidx++)
            {
                CPU_SET(tmp_cpuidx, &cpu_dedicated_coreset_);
            }
        }

        #ifdef DEBUG_THREAD_LAUNCHER
        std::ostringstream oss;
        oss << "[0, " << tmp_max_cpuidx << "]";
        Util::dumpVariablesForDebug(kClassName, 6, "bind high-priority thread:", thread_name.c_str(), "thread idx:", std::to_string(original_high_priority_threadcnt).c_str(), "cpu core set:", oss.str().c_str());
        #endif

        // Bind to the set of CPU dedicated cores
        int pthread_returncode = pthread_setaffinity_np(tid, sizeof(cpu_dedicated_coreset_), &cpu_dedicated_coreset_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to set affinity of thread " << thread_name << " (pthread_returncode: " << pthread_returncode << "; errno: " << errno << ")";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(-1);
        }
        #endif

        return;
    }
}
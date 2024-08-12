#include "common/thread_launcher.h"

#include <assert.h>
#include <thread> // std::thread::hardware_concurrency

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // DedicatedCoreAssignment

    const std::string DedicatedCoreAssignment::kClassName("DedicatedCoreAssignment");

    DedicatedCoreAssignment::DedicatedCoreAssignment() : cur_high_priority_cputidx_(0)
    {
        is_assigned_ = false;
        left_inclusive_dedicated_coreidx_ = 0;
        right_inclusive_dedicated_coreidx_ = 0;
    }

    DedicatedCoreAssignment::DedicatedCoreAssignment(const uint32_t& left_inclusive_dedicated_coreidx, const uint32_t& right_inclusive_dedicated_coreidx) : cur_high_priority_cputidx_(left_inclusive_dedicated_coreidx)
    {
        assert(right_inclusive_dedicated_coreidx >= left_inclusive_dedicated_coreidx);

        is_assigned_ = true;
        left_inclusive_dedicated_coreidx_ = left_inclusive_dedicated_coreidx;
        right_inclusive_dedicated_coreidx_ = right_inclusive_dedicated_coreidx;
    }

    DedicatedCoreAssignment::DedicatedCoreAssignment(const DedicatedCoreAssignment& other)
    {
        *this = other;
    }

    DedicatedCoreAssignment::~DedicatedCoreAssignment() {}

    bool DedicatedCoreAssignment::isAssigned() const
    {
        return is_assigned_;
    }

    uint32_t DedicatedCoreAssignment::getLeftInclusiveDedicatedCoreidx() const
    {
        return left_inclusive_dedicated_coreidx_;
    }

    uint32_t DedicatedCoreAssignment::getRightInclusiveDedicatedCoreidx() const
    {
        return right_inclusive_dedicated_coreidx_;
    }

    uint32_t DedicatedCoreAssignment::assignDedicatedCore()
    {
        assert(is_assigned_);

        uint32_t tmp_cur_high_priority_cputidx = cur_high_priority_cputidx_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);
        // assert(tmp_cur_high_priority_cputidx >= left_inclusive_dedicated_coreidx_);
        // assert(tmp_cur_high_priority_cputidx <= right_inclusive_dedicated_coreidx_);

        return tmp_cur_high_priority_cputidx;
    }

    const DedicatedCoreAssignment& DedicatedCoreAssignment::operator=(const DedicatedCoreAssignment& other)
    {
        if (this != &other)
        {
            is_assigned_ = other.is_assigned_;
            left_inclusive_dedicated_coreidx_ = other.left_inclusive_dedicated_coreidx_;
            right_inclusive_dedicated_coreidx_ = other.right_inclusive_dedicated_coreidx_;

            cur_high_priority_cputidx_.store(other.cur_high_priority_cputidx_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        }

        return *this;
    }

    // ThreadLauncher

    const std::string ThreadLauncher::CLIENT_THREAD_ROLE("client_thread");
    const std::string ThreadLauncher::EDGE_THREAD_ROLE("edge_thread");
    const std::string ThreadLauncher::CLOUD_THREAD_ROLE("cloud_thread");
    const std::string ThreadLauncher::EVALUATOR_THREAD_ROLE("evaluator_thread");
    const std::string ThreadLauncher::DATASET_LOADER_THREAD_ROLE("dataset_loader_thread");
    const std::string ThreadLauncher::TOTAL_STATISTICS_LOADER_THREAD_ROLE("total_statistics_loader_thread");
    const std::string ThreadLauncher::TRACE_PREPROCESSOR_THREAD_ROLE("trace_preprocessor_thread");
    
    // TODO: pass nice value into each pthread for SCHED_OTHER
    //const int ThreadLauncher::SCHEDULING_POLICY = SCHED_OTHER; // Default policy used by Linux (nice value: min 19 to max -20), which relies on kernel.sched_latency_ns and kernel.sched_min_granularity_ns
    const int ThreadLauncher::SCHEDULING_POLICY = SCHED_RR; // Round-robin (priority: min 1 to max 99), which relies on /proc/sys/kernel/sched_rr_timeslice_ms

    const std::string ThreadLauncher::kClassName = "ThreadLauncher";

    // Common utilities
    bool ThreadLauncher::is_valid_ = false;

    // For low-priority threads
    std::atomic<uint32_t> ThreadLauncher::low_priority_threadcnt_(0);
    cpu_set_t ThreadLauncher::cpu_shared_coreset_;

    // For high-priority threads
    std::unordered_map<std::string, DedicatedCoreAssignment> ThreadLauncher::current_machine_dedicated_core_assignments_;
    std::atomic<uint32_t> ThreadLauncher::high_priority_threadcnt_(0);

    // For validation

    void ThreadLauncher::validate(const std::string main_class_name, const uint32_t clientcnt, const uint32_t edgecnt)
    {
        if (!is_valid_)
        {
            // Get per-thread-role required dedicated corecnt
            std::unordered_map<std::string, uint32_t> perrole_required_dedicated_corecnt;
            getPerroleRequiredDedicatedCorecnt_(main_class_name, clientcnt, edgecnt, perrole_required_dedicated_corecnt);

            // NOTE: std::unordered_map does NOT guarantee the insertion order of perrole_required_dedicated_corecnt to assign dedicated CPU cores
            uint32_t tmp_dedicated_coreidx = 0;
            for (std::unordered_map<std::string, uint32_t>::const_iterator perrole_required_dedicated_corecnt_const_iter = perrole_required_dedicated_corecnt.begin(); perrole_required_dedicated_corecnt_const_iter != perrole_required_dedicated_corecnt.end(); perrole_required_dedicated_corecnt_const_iter++)
            {
                const std::string tmp_thread_role = perrole_required_dedicated_corecnt_const_iter->first;
                const uint32_t tmp_required_dedicated_corecnt = perrole_required_dedicated_corecnt_const_iter->second;

                DedicatedCoreAssignment tmp_dedicated_core_assignment;
                if (tmp_required_dedicated_corecnt > 0) // If assigned with dedicated CPU cores
                {
                    const uint32_t tmp_left_inclusive_dedicated_coreidx = tmp_dedicated_coreidx;
                    const uint32_t tmp_right_inclusive_dedicated_coreidx = tmp_left_inclusive_dedicated_coreidx + tmp_required_dedicated_corecnt - 1;
                    tmp_dedicated_core_assignment = DedicatedCoreAssignment(tmp_left_inclusive_dedicated_coreidx, tmp_right_inclusive_dedicated_coreidx);
                }
                current_machine_dedicated_core_assignments_.insert(std::pair(tmp_thread_role, tmp_dedicated_core_assignment));

                tmp_dedicated_coreidx += tmp_required_dedicated_corecnt;
            }

            // NOTE: required dedicated corecnt MUST <= total dedicated corecnt in current machine
            const uint32_t required_dedicated_corecnt = tmp_dedicated_coreidx;
            const uint32_t current_machine_dedicated_corecnt = Config::getCurrentPhysicalMachine().getCpuDedicatedCorecnt();
            if (required_dedicated_corecnt > current_machine_dedicated_corecnt)
            {
                std::ostringstream oss;
                oss << "required_dedicated_corecnt " << required_dedicated_corecnt << " exceeds current_machine_dedicated_corecnt " << current_machine_dedicated_corecnt << " in current physical machine!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }

            is_valid_ = true;
        }

        return;
    }

    void ThreadLauncher::getPerroleRequiredDedicatedCorecnt_(const std::string main_class_name, const uint32_t clientcnt, const uint32_t edgecnt, std::unordered_map<std::string, uint32_t>& perrole_required_dedicated_corecnt)
    {
        // NOTE: single-node simulator does NOT launch client/cache/cloud/evaluator threads, and hence NO need to get per-role required dedicated corecnt
        if (main_class_name == Util::SINGLE_NODE_PROTOTYPE_MAIN_NAME || main_class_name == Util::CLIENT_MAIN_NAME || main_class_name == Util::EDGE_MAIN_NAME || main_class_name == Util::CLOUD_MAIN_NAME || main_class_name == Util::EVALUATOR_MAIN_NAME)
        {
            // Different thread roles of clients/edges/cloud/evaluator will share dedicated CPU cores of current physical machine
            assert(clientcnt > 0);
            assert(edgecnt > 0);

            // NOTE: follow the order of evaluator -> cloud -> edges -> clients for insertion, yet std::unordered_map does NOT guarantee enumeration order
            uint32_t evaluator_dedicated_corecnt = 0;
            bool is_current_machine_as_evaluator = Config::getCurrentMachineEvaluatorDedicatedCorecnt(evaluator_dedicated_corecnt);
            if (is_current_machine_as_evaluator) // Current machine as evaluator
            {
                perrole_required_dedicated_corecnt.insert(std::pair(EVALUATOR_THREAD_ROLE, evaluator_dedicated_corecnt));
            }
            uint32_t cloud_dedicated_corecnt = 0;
            bool is_current_machine_as_cloud = Config::getCurrentMachineCloudDedicatedCorecnt(cloud_dedicated_corecnt);
            if (is_current_machine_as_cloud) // Current machine as cloud
            {
                perrole_required_dedicated_corecnt.insert(std::pair(CLOUD_THREAD_ROLE, cloud_dedicated_corecnt));
            }
            uint32_t edge_dedicated_corecnt = 0;
            bool is_current_machine_as_edge = Config::getCurrentMachineEdgeDedicatedCorecnt(edgecnt, edge_dedicated_corecnt);
            if (is_current_machine_as_edge) // Current machine as edge
            {
                perrole_required_dedicated_corecnt.insert(std::pair(EDGE_THREAD_ROLE, edge_dedicated_corecnt));
            }
            uint32_t client_dedicated_corecnt = 0;
            bool is_current_machine_as_client = Config::getCurrentMachineClientDedicatedCorecnt(clientcnt, client_dedicated_corecnt);
            if (is_current_machine_as_client) // Current machine as client
            {
                perrole_required_dedicated_corecnt.insert(std::pair(CLIENT_THREAD_ROLE, client_dedicated_corecnt));
            }
        }
        else if (main_class_name == Util::DATASET_LOADER_MAIN_NAME)
        {
            // The single thread role of dataset loader can occupy all dedicated CPU cores of current physical machine due to NO contention with other thread roles

            perrole_required_dedicated_corecnt.insert(std::pair(DATASET_LOADER_THREAD_ROLE, Config::getCurrentPhysicalMachine().getCpuDedicatedCorecnt()));
        }
        else if (main_class_name == Util::TOTAL_STATISTICS_LOADER_MAIN_NAME)
        {
            // The single thread role of total statistics loader can occupy all dedicated CPU cores of current physical machine due to NO contention with other thread roles

            perrole_required_dedicated_corecnt.insert(std::pair(TOTAL_STATISTICS_LOADER_THREAD_ROLE, Config::getCurrentPhysicalMachine().getCpuDedicatedCorecnt()));
        }
        else if (main_class_name == Util::TRACE_PREPROCESSOR_MAIN_NAME)
        {
            // The single thread role of trace preprocessor can occupy all dedicated CPU cores of current physical machine due to NO contention with other thread roles

            perrole_required_dedicated_corecnt.insert(std::pair(TRACE_PREPROCESSOR_THREAD_ROLE, Config::getCurrentPhysicalMachine().getCpuDedicatedCorecnt()));
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid main_class_name " << main_class_name;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }

    // For low-priority threads

    void ThreadLauncher::bindMainThreadToSharedCpuCore(const std::string main_thread_name)
    {
        checkIsValid_();

        pthread_t main_thread = pthread_self();
        bindSharedCpuCore_(main_thread_name, main_thread);
        return;
    }

    void ThreadLauncher::pthreadCreateLowPriority(const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr)
    {
        checkIsValid_();

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

    void ThreadLauncher::bindSharedCpuCore_(const std::string thread_name, const pthread_t& tid)
    {
        // Update low-priority thread cnt
        uint32_t original_low_priority_threadcnt = low_priority_threadcnt_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);

        // NOTE: bind each low-priority thread to the set of CPU shared cores

        // Calculate max shared CPU core ID
        const PhysicalMachine tmp_physical_machine = Config::getCurrentPhysicalMachine();
        const uint32_t tmp_cpu_dedicated_corecnt = tmp_physical_machine.getCpuDedicatedCorecnt();
        const uint32_t tmp_cpu_shared_corecnt = tmp_physical_machine.getCpuSharedCorecnt();
        const uint32_t tmp_min_cpuidx = tmp_cpu_shared_corecnt > 0 ? tmp_cpu_dedicated_corecnt : 0; // Bind low-priority threads to the set of all CPU cores if NO shared CPU cores
        const uint32_t tmp_max_cpuidx = tmp_cpu_dedicated_corecnt + tmp_cpu_shared_corecnt - 1;
        assert(tmp_max_cpuidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores
        assert(tmp_max_cpuidx >= tmp_min_cpuidx && tmp_max_cpuidx < (tmp_cpu_dedicated_corecnt + tmp_cpu_shared_corecnt)); // MUST within the range of [tmp_min_cpuidx, cpu_dedicated_corecnt + cpu_shared_corecnt - 1]

        // Initialize cpu_shared_coreset_ if NOT
        if (original_low_priority_threadcnt == 0) // The first low-priority thread
        {
            // Prepare cpu_shared_coreset_ within the range of [tmp_min_cpuidx, cpu_dedicated_corecnt + cpu_shared_corecnt - 1]
            CPU_ZERO(&cpu_shared_coreset_);
            for (uint32_t tmp_cpuidx = tmp_min_cpuidx; tmp_cpuidx <= tmp_max_cpuidx; tmp_cpuidx++)
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

    // For high-priority threads
    
    uint32_t ThreadLauncher::pthreadCreateHighPriority(const std::string thread_role, const std::string thread_name, pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr, const uint32_t* specified_cpuidx_ptr)
    {
        checkIsValid_();

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
        uint32_t tmp_cpuidx = bindDedicatedCpuCore_(thread_role, thread_name, *tid_ptr, specified_cpuidx_ptr);
        return tmp_cpuidx;
    }

    uint32_t ThreadLauncher::bindDedicatedCpuCore_(const std::string thread_role, const std::string thread_name, const pthread_t& tid, const uint32_t* specified_cpuidx_ptr)
    {
        // Update high-priority thread cnt
        uint32_t original_high_priority_threadcnt = high_priority_threadcnt_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);

        // Get reference of cur_high_priority_cpuidx for the given thread role
        std::unordered_map<std::string, DedicatedCoreAssignment>::iterator current_machine_dedicated_cores_assignment_iter = current_machine_dedicated_core_assignments_.find(thread_role);
        if (current_machine_dedicated_cores_assignment_iter == current_machine_dedicated_core_assignments_.end())
        {
            std::ostringstream oss;
            oss << "current physical machine does NOT play the role of " << thread_role << " -> failed to bind a dedicated CPU core for thread " << thread_name;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        if (!current_machine_dedicated_cores_assignment_iter->second.isAssigned())
        {
            std::ostringstream oss;
            oss << "current physical machine does NOT assign any dedicated CPU core for the role of " << thread_role << " -> failed to bind a dedicated CPU core for thread " << thread_name;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // Get valid dedicated core range for the given thread role
        const uint32_t left_inclusive_dedicated_coreidx = current_machine_dedicated_cores_assignment_iter->second.getLeftInclusiveDedicatedCoreidx();
        const uint32_t right_inclusive_dedicated_coreidx = current_machine_dedicated_cores_assignment_iter->second.getRightInclusiveDedicatedCoreidx();

        // Verify correctness of dedicated core range
        assert(right_inclusive_dedicated_coreidx >= left_inclusive_dedicated_coreidx);
        assert(right_inclusive_dedicated_coreidx < std::thread::hardware_concurrency()); // NOT exceed total # of CPU cores in current physical machine
        assert(right_inclusive_dedicated_coreidx < Config::getCurrentPhysicalMachine().getCpuDedicatedCorecnt()); // NOT exceed # of dedicated CPU cores in current physical machine

        // Calculate a dedicated CPU core ID for the high-priority thread
        uint32_t tmp_cpuidx = 0;
        if (specified_cpuidx_ptr == NULL)
        {
            tmp_cpuidx = current_machine_dedicated_cores_assignment_iter->second.assignDedicatedCore();
        }
        else
        {
            tmp_cpuidx = *specified_cpuidx_ptr;
        }
        //uint32_t tmp_cpuidx = 0 + tmp_cpuidx % tmp_cpu_dedicated_corecnt;

        // NOTE: cpuidx MUST within the dedicated core range of the given thread role
        if (tmp_cpuidx < left_inclusive_dedicated_coreidx || tmp_cpuidx > right_inclusive_dedicated_coreidx)
        {
            std::ostringstream oss;
            oss << "invalid cpuidx " << tmp_cpuidx << " for thread " << thread_name << " -> must within the range of [" << left_inclusive_dedicated_coreidx << ", " << right_inclusive_dedicated_coreidx << "]";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

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

        return tmp_cpuidx;
    }

    // Common utilities

    void ThreadLauncher::checkIsValid_()
    {
        assert(is_valid_);
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
}
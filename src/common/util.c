#include "common/util.h"

#include <assert.h>
#include <chrono> // system_clock
#include <errno.h> // ENOENT
#include <iostream> // cerr
#include <sched.h> // sched_param
#include <sstream> // ostringstream
#include <cmath> // pow

#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system.hpp>

#include "common/config.h"

namespace covered
{
    // (1) For CLI parameters

    // Main class name
    const std::string Util::SIMULATOR_MAIN_NAME("simulator");
    //const std::string Util::STATISTICS_AGGREGATOR_MAIN_NAME("statistics_aggregator");
    const std::string Util::TOTAL_STATISTICS_LOADER_MAIN_NAME("total_statistics_loader");
    const std::string Util::DATASET_LOADER_MAIN_NAME("dataset_loader");
    const std::string Util::CLIENT_MAIN_NAME("client");
    const std::string Util::EDGE_MAIN_NAME("edge");
    const std::string Util::CLOUD_MAIN_NAME("cloud");
    const std::string Util::EVALUATOR_MAIN_NAME("evaluator");

    // Workload name
    const std::string Util::FACEBOOK_WORKLOAD_NAME("facebook");

    // Cloud storage
    const std::string Util::HDD_NAME = "hdd";

    // Cache name
    const std::string Util::CACHELIB_CACHE_NAME("cachelib");
    const std::string Util::LFU_CACHE_NAME("lfu");
    const std::string Util::LRU_CACHE_NAME("lru");
    const std::string Util::SEGCACHE_CACHE_NAME("segcache");
    const std::string Util::COVERED_CACHE_NAME("covered");

    // Hash name
    const std::string Util::MMH3_HASH_NAME("mmh3");

    // (2) For utility functions

    // Type conversion/checking
    const int64_t Util::MAX_UINT16 = 65536;
    const int64_t Util::MAX_UINT32 = 4294967296;
    // Network
    const std::string Util::LOCALHOST_IPSTR("127.0.0.1"); // Pass network card
    //const std::string Util::LOCALHOST_IPSTR("localhost"); // NOT pass network card
    const std::string Util::ANY_IPSTR("0.0.0.0");
    const uint32_t Util::UDP_MAX_PKT_PAYLOAD = 65507; // 65535(ipmax) - 20(iphdr) - 8(udphdr)
    const uint32_t Util::UDP_FRAGHDR_SIZE = 5 * sizeof(uint32_t) + sizeof(uint16_t); // 4(fragment_idx) + 4(fragment_cnt) + 4(msg_payload_size) + 4(msg_seqnum) + 4(source_ip) + 2(source_port)
    const uint32_t Util::UDP_MAX_FRAG_PAYLOAD = Util::UDP_MAX_PKT_PAYLOAD - Util::UDP_FRAGHDR_SIZE;
    const uint16_t Util::UDP_MIN_PORT = 1024; // UDP port has to be >= 1024 (0-1023 are reserved for well-known usage; 1024-49151 are registered ports; 49152-65535 are custom ports)
    // Atomicity
    std::memory_order Util::LOAD_CONCURRENCY_ORDER = std::memory_order_acquire;
    std::memory_order Util::STORE_CONCURRENCY_ORDER = std::memory_order_release;
    std::memory_order Util::RMW_CONCURRENCY_ORDER = std::memory_order_acq_rel;
    // Workflow control
    const unsigned int Util::SLEEP_INTERVAL_US = MS2US(100); // 100ms
    // Workload generation
    const uint32_t Util::KVPAIR_GENERATION_SEED = 0;
    // Time measurement
    const int Util::START_YEAR = 1900;
    const long Util::NANOSECONDS_PERSECOND = 1000000000L;
    const uint32_t Util::SECOND_PRECISION = 4;
    // Task scheduling
    // TODO: pass nice value into each pthread for SCHED_OTHER
    //const int Util::SCHEDULING_POLICY = SCHED_OTHER; // Default policy used by Linux (nice value: min 19 to max -20), which relies on kernel.sched_latency_ns and kernel.sched_min_granularity_ns
    const int Util::SCHEDULING_POLICY = SCHED_RR; // Round-robin (priority: min 1 to max 99), which relies on /proc/sys/kernel/sched_rr_timeslice_ms

    // For charset
    const std::string Util::CHARSET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::mt19937_64 Util::string_randgen_(0);
    std::uniform_int_distribution<uint32_t> Util::string_randdist_(0, CHARSET.size() - 1); // Range of [0, CHARSET.size() - 1]

    const std::string Util::kClassName("Util");

    std::mutex Util::msgdump_lock_;

    // (1) I/O

    // (1.1) stdout/stderr I/O

    void Util::dumpNormalMsg(const std::string& class_name, const std::string& normal_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        std::cout << cur_timestr << " <" << class_name << "> " << normal_message << std::endl;
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpDebugMsg(const std::string& class_name, const std::string& debug_message)
    {
        if (Config::isDebug())
        {
            std::string cur_timestr = getCurrentTimestr();
            msgdump_lock_.lock();
            // \033 means ESC character, 1 means bold, 32 means green foreground, 0 means reset, and m is end character
            std::cout << "\033[1;32m" << cur_timestr << " [DEBUG] <" << class_name << "> " << debug_message << std::endl << "\033[0m";
            msgdump_lock_.unlock();
        }
        return;
    }

    void Util::dumpWarnMsg(const std::string& class_name, const std::string& warn_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        // \033 means ESC character, 1 means bold, 33 means yellow foreground, 0 means reset, and m is end character
        std::cout << "\033[1;33m" << cur_timestr << " [WARN] <" << class_name << "> " << warn_message << std::endl << "\033[0m";
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpErrorMsg(const std::string& class_name, const std::string& error_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        // \033 means ESC character, 1 means bold, 31 means red foreground, 0 means reset, and m is end character
        std::cerr << "\033[1;31m" << cur_timestr << " [ERROR] <" << class_name << "> " << error_message << std::endl << "\033[0m" << boost::stacktrace::stacktrace();
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpVariablesForDebug(const std::string& class_name, const uint32_t count, ...)
    {
        std::va_list args;
        va_start(args, count);

        std::ostringstream oss;
        for (uint32_t i = 0; i < count; i++)
        {
            const char* tmpstr = va_arg(args, const char*);
            oss << std::string(tmpstr) << " ";
        }

        dumpDebugMsg(class_name, oss.str());
        return;
    }

    // (1.2) File I/O

    bool Util::isFileExist(const std::string& filepath, const bool& is_silent)
    {
        return isPathExist_(filepath, true, is_silent);
    }

    bool Util::isDirectoryExist(const std::string& dirpath, const bool& is_silent)
    {
        return isPathExist_(dirpath, false, is_silent);
    }

    void Util::createDirectory(const std::string& dirpath)
    {
        assert(!isDirectoryExist(dirpath, true));
        //bool result = boost::filesystem::create_directory(dirpath);
        bool result = boost::filesystem::create_directories(dirpath); // Create directory path recursively
        if (!result)
        {
            std::ostringstream oss;
            oss << "fail to create directory " << dirpath;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    std::string Util::getParentDirpath(const std::string& filepath)
    {
        boost::filesystem::path boost_filepath(filepath);
        boost::filesystem::path boost_dirpath = boost_filepath.parent_path();
        return boost_dirpath.string();
    }

    std::string Util::getFilenameFromFilepath(const std::string& filepath)
    {
        boost::filesystem::path boost_filepath(filepath);
        boost::filesystem::path boost_filename = boost_filepath.filename();
        return boost_filename.string();
    }

    bool Util::isPathExist_(const std::string& path, const bool& is_file, const bool& is_silent)
    {
        // Get boost::file_status
        boost::filesystem::path boost_path(path);
        boost::system::error_code boost_errcode;
        boost::filesystem::file_status boost_pathstatus = boost::filesystem::status(boost_path, boost_errcode);

        // An error occurs
        bool is_error = bool(boost_errcode); // boost_errcode.m_val != 0
        if (is_error && boost_errcode.value() != ENOENT)
        {
            dumpErrorMsg(kClassName, boost_errcode.message());
            exit(1);
        }

        // File not exist
        if (!boost::filesystem::exists(boost_pathstatus))
        {
            if (!is_silent)
            {
                std::ostringstream oss;
                oss << path << " does not exist!";
                dumpWarnMsg(kClassName, oss.str());
            }
            return false;
        }

        // Judge file or directory
        if (is_file) // Should be a regular file
        {
            if (boost::filesystem::is_directory(boost_pathstatus))
            {
                std::ostringstream oss;
                oss << path << " is a directory!";
                dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        else // Should be a directory
        {
            if (boost::filesystem::is_regular_file(boost_pathstatus))
            {
                std::ostringstream oss;
                oss << path << " is a regular file!";
                dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        return true;
    }

    std::fstream* Util::openFile(const std::string& filepath, std::ios_base::openmode mode)
    {
        std::fstream* fs_ptr = new std::fstream(filepath.c_str(), mode);
        assert(fs_ptr != NULL);
        if (!(*fs_ptr))
        {
            std::ostringstream oss;
            oss << "fail to open statistics file " << filepath;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return fs_ptr; // Release outside Util
    }

    // (2) Time measurement

    struct timespec Util::getCurrentTimespec()
    {
        struct timespec current_timespec;
        clock_gettime(CLOCK_REALTIME, &current_timespec);
        return current_timespec;
    }

    std::string Util::getCurrentTimestr()
    {
        // Calculate reuiqred data
        struct timespec current_timespec = getCurrentTimespec();
        struct tm* current_datetime = localtime(&(current_timespec.tv_sec));
        int year = current_datetime->tm_year + START_YEAR;
        int month = current_datetime->tm_mon + 1;
        int day = current_datetime->tm_mday;
        int hour = current_datetime->tm_hour;
        int minute = current_datetime->tm_min;
        double second = static_cast<double>(current_datetime->tm_sec);
        double nanosecond = static_cast<double>(current_timespec.tv_nsec) / static_cast<double>(NANOSECONDS_PERSECOND);
        if (nanosecond >= (double(1.0) / pow(10, static_cast<double>(SECOND_PRECISION))))
        {
            second += nanosecond;
        }

        // Dump time string in a fixed format (keep the first two digits after decimal point for second)
        std::ostringstream oss;
        std::streamsize default_precision = oss.precision();
        oss << year << "." << month << "." << day << " " << hour << ":" << minute << ":";
        oss.setf(std::ios_base::fixed);
        oss.precision(SECOND_PRECISION);
        oss << second;
        oss.unsetf(std::ios_base::fixed);
        oss.precision(default_precision);
        return oss.str();
    }

    double Util::getDeltaTimeUs(const struct timespec& current_timespec, const struct timespec& previous_timespec)
    {
        struct timespec delta_timespec;
        delta_timespec.tv_sec = current_timespec.tv_sec - previous_timespec.tv_sec;
		delta_timespec.tv_nsec = current_timespec.tv_nsec - previous_timespec.tv_nsec;
		if (delta_timespec.tv_nsec < 0)
        {
			delta_timespec.tv_sec--;
			delta_timespec.tv_nsec += NANOSECONDS_PERSECOND;
		}

        double delta_time = SEC2US(delta_timespec.tv_sec) + double(delta_timespec.tv_nsec) / double(1000.0);
        return delta_time;
    }

    // (3) Type conversion/checking

    uint16_t Util::toUint16(const int64_t& val)
    {
        if (val >= 0 && val < MAX_UINT16)
        {
            uint16_t result = static_cast<uint16_t>(val);
            return result;
        }
        else
        {
            std::ostringstream oss;
            oss << "cannot convert " << val << " (>= " << MAX_UINT16 << ") to uint16_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    uint32_t Util::toUint32(const int64_t& val)
    {
        if (val >= 0 && val < MAX_UINT32)
        {
            uint32_t result = static_cast<uint32_t>(val);
            return result;
        }
        else
        {
            std::ostringstream oss;
            oss << "cannot convert " << val << " (>= " << MAX_UINT32 << ") to uint32_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    uint64_t Util::uint64Add(const uint64_t& a, const uint64_t& b)
    {
        uint64_t results = a + b;
        if (results < a || results < b)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << a << " + " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return results;
    }
    
    uint64_t Util::uint64Minus(const uint64_t& a, const uint64_t& b)
    {
        uint64_t results = a - b;
        if (results > a)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << a << " - " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return results;
    }

    void Util::uint64AddForAtomic(std::atomic<uint64_t>& a, const uint64_t& b)
    {
        uint64_t original_a = a.fetch_add(b, RMW_CONCURRENCY_ORDER);
        uint64_t results = a.load(LOAD_CONCURRENCY_ORDER);
        if (results < original_a || results < b)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << original_a << " + " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
    
    void Util::uint64MinusForAtomic(std::atomic<uint64_t>& a, const uint64_t& b)
    {
        uint64_t original_a = a.fetch_sub(b, RMW_CONCURRENCY_ORDER);
        uint64_t results = a.load(LOAD_CONCURRENCY_ORDER);
        if (results > original_a)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << original_a << " - " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    std::string Util::toString(void* pointer)
    {
        return std::to_string(reinterpret_cast<intptr_t>(pointer));
    }

    std::string Util::toString(const bool& boolean)
    {
        return boolean?"true":"false";
    }

    std::string Util::toString(const uint32_t& val)
    {
        return std::to_string(val);
    }

    // (4) Client-edge-cloud scenario

    // (4.1) Client

    uint16_t Util::getClientRecvmsgPort(const uint32_t& client_idx, const uint32_t& clientcnt)
    {
        int64_t client_recvmsg_startport = static_cast<int64_t>(Config::getClientRecvmsgStartport());
        return getNodePort_(client_recvmsg_startport, client_idx, clientcnt, Config::getClientIpstrCnt());
    }

    uint32_t Util::getClosestEdgeIdx(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt)
    {
        assert(edgecnt > 0);
        uint32_t peredge_clientcnt = clientcnt / edgecnt;
        assert(peredge_clientcnt > 0);
        uint32_t closest_edge_idx = client_idx / peredge_clientcnt;
        if (closest_edge_idx >= edgecnt)
        {
            closest_edge_idx = edgecnt - 1; // Assign tail clients to the last edge node
        }
        return closest_edge_idx;
    }

    std::string Util::getClosestEdgeIpstr(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt)
    {
        uint32_t closest_edge_idx = getClosestEdgeIdx(client_idx, clientcnt, edgecnt);
        return Config::getEdgeIpstr(closest_edge_idx, edgecnt);
    }

    uint16_t Util::getClosestEdgeCacheServerRecvreqPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt)
    {
        uint32_t closest_edge_idx = getClosestEdgeIdx(client_idx, clientcnt, edgecnt);
        return getEdgeCacheServerRecvreqPort(closest_edge_idx, edgecnt);
    }

    uint32_t Util::getGlobalClientWorkerIdx(const uint32_t& client_idx, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt)
    {
        uint32_t global_client_worker_idx = client_idx * perclient_workercnt + local_client_worker_idx;
        return global_client_worker_idx;
    }

    /*void Util::parseGlobalClientWorkerIdx(const uint32_t& global_client_worker_idx, const uint32_t& perclient_workercnt, uint32_t& client_idx, uint32_t& local_client_worker_idx)
    {
        client_idx = global_client_worker_idx / perclient_workercnt;
        local_client_worker_idx = global_client_worker_idx - client_idx * perclient_workercnt;
        assert(local_client_worker_idx < perclient_workercnt);
        return;
    }*/

    uint16_t Util::getClientWorkerRecvrspPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt)
    {
        assert(local_client_worker_idx < perclient_workercnt);

        // Get client recvrsp port if 1 worker per client
        int64_t client_worker_recvrsp_startport = static_cast<int64_t>(Config::getClientWorkerRecvrspStartport());
        int64_t client_recvrsp_port = static_cast<int64_t>(getNodePort_(client_worker_recvrsp_startport, client_idx, clientcnt, Config::getClientIpstrCnt()));
        assert(client_recvrsp_port >= client_worker_recvrsp_startport);

        // Get client worker recvrsp port
        int64_t client_worker_recvrsp_port = client_worker_recvrsp_startport + (client_recvrsp_port - client_worker_recvrsp_startport) * static_cast<int64_t>(perclient_workercnt) + static_cast<int64_t>(local_client_worker_idx);
        
        return Util::toUint16(client_worker_recvrsp_port);
    }

    // (4.2) Edge

    uint16_t Util::getEdgeRecvmsgPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_recvmsg_startport = static_cast<int64_t>(Config::getEdgeRecvmsgStartport());
        return getNodePort_(edge_recvmsg_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    uint16_t Util::getEdgeBeaconServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_beacon_server_recvreq_startport = static_cast<int64_t>(Config::getEdgeBeaconServerRecvreqStartport());
        return getNodePort_(edge_beacon_server_recvreq_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    uint16_t Util::getEdgeBeaconServerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_beacon_server_recvrsp_startport = static_cast<int64_t>(Config::getEdgeBeaconServerRecvrspStartport());
        return getNodePort_(edge_beacon_server_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    uint16_t Util::getEdgeCacheServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_cache_server_recvreq_startport = static_cast<int64_t>(Config::getEdgeCacheServerRecvreqStartport());
        return getNodePort_(edge_cache_server_recvreq_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    uint16_t Util::getEdgeCacheServerPlacementProcessorRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_cache_server_placement_processor_recvrsp_startport = static_cast<int64_t>(Config::getEdgeCacheServerPlacementProcessorRecvrspStartport());
        return getNodePort_(edge_cache_server_placement_processor_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    NetworkAddr Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(const NetworkAddr& edge_cache_server_worker_recvrsp_addr)
    {
        std::string edge_ipstr = edge_cache_server_worker_recvrsp_addr.getIpstr();
        uint16_t edge_cache_server_worker_recvreq_port = edge_cache_server_worker_recvrsp_addr.getPort() - Config::getEdgeCacheServerWorkerRecvrspStartport() + Config::getEdgeCacheServerWorkerRecvreqStartport();
        NetworkAddr edge_cache_server_worker_recvreq_addr(edge_ipstr, edge_cache_server_worker_recvreq_port);

        return edge_cache_server_worker_recvreq_addr;
    }

    uint16_t Util::getEdgeCacheServerWorkerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt)
    {
        int64_t edge_cache_server_worker_recvreq_startport = static_cast<int64_t>(Config::getEdgeCacheServerWorkerRecvreqStartport());
        int64_t edge_cache_server_recvreq_port = static_cast<int64_t>(getNodePort_(edge_cache_server_worker_recvreq_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt()));

        // Get cache server worker recvreq port
        int64_t cache_server_worker_recvreq_port = edge_cache_server_worker_recvreq_startport + (edge_cache_server_recvreq_port - edge_cache_server_worker_recvreq_startport) * static_cast<int64_t>(percacheserver_workercnt) + static_cast<int64_t>(local_cache_server_worker_idx);

        return Util::toUint16(cache_server_worker_recvreq_port);
    }

    uint16_t Util::getEdgeCacheServerWorkerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt)
    {
        int64_t edge_cache_server_worker_recvrsp_startport = static_cast<int64_t>(Config::getEdgeCacheServerWorkerRecvrspStartport());
        int64_t edge_cache_server_recvrsp_port = static_cast<int64_t>(getNodePort_(edge_cache_server_worker_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt()));

        // Get cache server worker recvreq port
        int64_t cache_server_worker_recvrsp_port = edge_cache_server_worker_recvrsp_startport + (edge_cache_server_recvrsp_port - edge_cache_server_worker_recvrsp_startport) * static_cast<int64_t>(percacheserver_workercnt) + static_cast<int64_t>(local_cache_server_worker_idx);

        return Util::toUint16(cache_server_worker_recvrsp_port);
    }

    uint16_t Util::getEdgeInvalidationServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_invalidation_server_recvreq_startport = static_cast<int64_t>(Config::getEdgeInvalidationServerRecvreqStartport());
        return getNodePort_(edge_invalidation_server_recvreq_startport, edge_idx, edgecnt, Config::getEdgeIpstrCnt());
    }

    // (4.3) Cloud

    uint16_t Util::getCloudRecvmsgPort(const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now
        assert(cloud_idx == 0);

        int64_t cloud_recvmsg_startport = static_cast<int64_t>(Config::getCloudRecvmsgStartport());
        return getNodePort_(cloud_recvmsg_startport, cloud_idx, 1, 1);
    }

    uint16_t Util::getCloudRecvreqPort(const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now
        assert(cloud_idx == 0);

        int64_t cloud_recvreq_startport = static_cast<int64_t>(Config::getCloudRecvreqStartport());
        return getNodePort_(cloud_recvreq_startport, cloud_idx, 1, 1);
    }

    uint16_t Util::getNodePort_(const int64_t& start_port, const uint32_t& node_idx, const uint32_t& nodecnt, const uint32_t& machine_cnt)
    {
        int64_t node_port = 0;
        if (Config::isSingleNode())
        {
            node_port = start_port + node_idx;
        }
        else
        {
            assert(machine_cnt > 0);
            uint32_t permachine_nodecnt = nodecnt / machine_cnt;
            assert(permachine_nodecnt > 0);
            uint32_t machine_idx = node_idx / permachine_nodecnt;

            uint32_t local_node_idx = 0;
            if (machine_idx < machine_cnt)
            {
                local_node_idx = node_idx - machine_idx * permachine_nodecnt;
                assert(local_node_idx < permachine_nodecnt);
            }
            else
            {
                local_node_idx = node_idx - (machine_cnt - 1) * permachine_nodecnt;
                assert(local_node_idx >= permachine_nodecnt);
            }

            node_port = start_port + local_node_idx;
        }
        return Util::toUint16(node_port);
    }

    // (5) Network

    uint32_t Util::getFragmentCnt(const uint32_t& msg_payload_size)
    {
        // Split message payload into multiple fragment payloads
        uint32_t max_fragment_payload = Util::UDP_MAX_FRAG_PAYLOAD;
        uint32_t fragment_cnt = 0;
        if (msg_payload_size == 0)
        {
            fragment_cnt = 1;
        }
        else
        {
            assert(max_fragment_payload > 0);
            fragment_cnt = (msg_payload_size - 1) / max_fragment_payload + 1;
        }
        return fragment_cnt;
    }

    uint32_t Util::getFragmentOffset(const uint32_t& fragment_idx)
    {
        return fragment_idx * UDP_MAX_FRAG_PAYLOAD;
    }

    uint32_t Util::getFragmentPayloadSize(const uint32_t& fragment_idx, const uint32_t& msg_payload_size)
    {
        uint32_t fragment_payload_size = UDP_MAX_FRAG_PAYLOAD;
        uint32_t fragment_cnt = getFragmentCnt(msg_payload_size);
        if (fragment_idx == fragment_cnt - 1)
        {
            fragment_payload_size = msg_payload_size % UDP_MAX_FRAG_PAYLOAD;
        }
        assert(fragment_payload_size <= UDP_MAX_FRAG_PAYLOAD);
        return fragment_payload_size;
    }

    // (6) Intermediate files

    std::string Util::getEvaluatorStatisticsDirpath(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        std::ostringstream oss;
        oss << Config::getOutputBasedir() << "/" << getInfixForEvaluatorStatisticsFilepath_(evaluator_cli_ptr);
        std::string client_statistics_dirpath = oss.str();
        return client_statistics_dirpath;
    }

    /*std::string Util::getClientStatisticsFilepath(const uint32_t& client_idx)
    {
        std::ostringstream oss;
        oss << getStatisticsDirpath() << "/client" << client_idx << "_statistics.out";
        std::string client_statistics_filepath = oss.str();
        return client_statistics_filepath;
    }*/

    std::string Util::getEvaluatorStatisticsFilepath(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        std::ostringstream oss;
        oss << getEvaluatorStatisticsDirpath(evaluator_cli_ptr) << "/evaluator_statistics.out";
        std::string evaluator_statistics_filepath = oss.str();
        return evaluator_statistics_filepath;
    }

    std::string Util::getCloudRocksdbBasedirForWorkload(const uint32_t& keycnt, const std::string& workload_name)
    {
        assert(keycnt > 0);

        std::ostringstream oss;
        // Example: /tmp/key1000000_facebook
        oss << Config::getCloudRocksdbBasedir() << "/key" << keycnt << "_" << workload_name;
        return oss.str();
    }

    std::string Util::getCloudRocksdbDirpath(const uint32_t& keycnt, const std::string& workload_name, const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now!
        assert(cloud_idx == 0);

        std::ostringstream oss;
        oss << getCloudRocksdbBasedirForWorkload(keycnt, workload_name) << "/cloud" << cloud_idx << ".db";
        std::string cloud_rocksdb_dirpath = oss.str();
        return cloud_rocksdb_dirpath;
    }

    std::string Util::getInfixForEvaluatorStatisticsFilepath_(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        const std::string cache_name = evaluator_cli_ptr->getCacheName();
        const uint64_t capacity_bytes = evaluator_cli_ptr->getCapacityBytes();
        const uint32_t clientcnt = evaluator_cli_ptr->getClientcnt();
        const bool is_warmup_speedup = evaluator_cli_ptr->isWarmupSpeedup();
        const uint32_t edgecnt = evaluator_cli_ptr->getEdgecnt();
        const std::string hash_name = evaluator_cli_ptr->getHashName();
        const uint32_t keycnt = evaluator_cli_ptr->getKeycnt();
        const uint32_t opcnt = evaluator_cli_ptr->getOpcnt();
        const uint32_t percacheserver_workercnt = evaluator_cli_ptr->getPercacheserverWorkercnt();
        const uint32_t perclient_workercnt = evaluator_cli_ptr->getPerclientWorkercnt();
        const uint32_t peredge_synced_victimcnt = evaluator_cli_ptr->getPeredgeSyncedVictimcnt();
        const uint32_t propagation_latency_clientedge_us = evaluator_cli_ptr->getPropagationLatencyClientedgeUs();
        const uint32_t propagation_latency_crossedge_us = evaluator_cli_ptr->getPropagationLatencyCrossedgeUs();
        const uint32_t propagation_latency_edgecloud_us = evaluator_cli_ptr->getPropagationLatencyEdgecloudUs();
        const uint32_t max_warmup_duration_sec = evaluator_cli_ptr->getMaxWarmupDurationSec();
        const uint32_t stresstest_duration_sec = evaluator_cli_ptr->getStresstestDurationSec();
        const std::string workload_name = evaluator_cli_ptr->getWorkloadName();
        
        // ONLY for COVERED
        uint64_t local_uncached_capacitymb = B2MB(evaluator_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes());
        uint64_t popularity_aggregation_capacitymb = B2MB(evaluator_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes());
        double popularity_collection_change_ratio = evaluator_cli_ptr->getCoveredPopularityCollectionChangeRatio();
        uint32_t topk_edgecnt = evaluator_cli_ptr->getCoveredTopkEdgecnt();

        std::ostringstream oss;
        // NOTE: NOT consider variables which do NOT affect evaluation results
        // (i) Config variables from JSON, e.g., is_debug and is_track_event
        // (ii) Config variables from CLI, e.g., main_class_name, --config_filepath, and is_single_node
        // (iii) Unchanged CLI parameters, e.g., --cloud_storage
        // Example: covered_capacitymb1000_clientcnt1_warmupspeedup1_edgecnt1_hashnamemmh3_keycnt1000000_opcnt1000000_percacheserverworkercnt1_perclientworkercnt1_peredgesyncedvictimcnt3_propagationus100010000100000_maxwarmupdurationsec10_stresstestdurationsec10_facebook
        oss << cache_name << "_capacitymb" << B2MB(capacity_bytes) << "_clientcnt" << clientcnt << "_warmupspeedup" << (is_warmup_speedup?"1":"0") << "_edgecnt" << edgecnt << "_hashname" << hash_name << "_keycnt" << keycnt << "_opcnt" << opcnt << "_percacheserverworkercnt" << percacheserver_workercnt << "_perclientworkercnt" << perclient_workercnt << "_peredgesyncedvictimcnt" << peredge_synced_victimcnt << "_propagationus" << propagation_latency_clientedge_us << propagation_latency_crossedge_us << propagation_latency_edgecloud_us << "_maxwarmupdurationsec" << max_warmup_duration_sec << "_stresstestdurationsec" << stresstest_duration_sec << "_" << workload_name;
        if (cache_name == "covered")
        {
            // Example: _localuncachedcapacitymb1_popaggregationcapacitymb1_popcollectchangeratio0.0_topkedgecnt3
            oss << "_localuncachedcapacitymb" << local_uncached_capacitymb << "_popaggregationcapacitymb" << popularity_aggregation_capacitymb << "_popcollectchangeratio" << popularity_collection_change_ratio << "_topkedgecnt" << topk_edgecnt;
        }
        std::string infixstr = oss.str();
        return infixstr;
    }

    // (7) Task scheduling

    int Util::pthreadCreateLowPriority(pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr)
    {
        int ret = 0;

        // Prepare thread attributes
        pthread_attr_t attr;
        preparePthreadAttr_(&attr);

        // Prepare scheduling parameters
        sched_param param;
        ret = pthread_attr_getschedparam(&attr, &param);
        assert(ret >= 0);
        param.sched_priority = sched_get_priority_min(SCHEDULING_POLICY);
        ret = pthread_attr_setschedparam(&attr, &param);
        assert(ret >= 0);

        // Launch pthread
        ret = pthread_create(tid_ptr, &attr, start_routine, arg_ptr);
        return ret;
    }
    
    int Util::pthreadCreateHighPriority(pthread_t* tid_ptr, void *(*start_routine)(void *), void* arg_ptr)
    {
        int ret = 0;

        // Prepare thread attributes
        pthread_attr_t attr;
        preparePthreadAttr_(&attr);

        // Prepare scheduling parameters
        sched_param param;
        ret = pthread_attr_getschedparam(&attr, &param);
        assert(ret >= 0);
        param.sched_priority = sched_get_priority_max(SCHEDULING_POLICY);
        ret = pthread_attr_setschedparam(&attr, &param);
        assert(ret >= 0);

        // Launch pthread
        ret = pthread_create(tid_ptr, &attr, start_routine, arg_ptr);
        return ret;
    }

    void Util::preparePthreadAttr_(pthread_attr_t* attr_ptr)
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

    // (8) Others

    uint32_t Util::getTimeBasedRandomSeed()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        return static_cast<uint32_t>(seed);
    }

    std::string Util::getRandomString(const uint32_t& length)
    {
        std::string random_string(length, '0');
        for (uint32_t i = 0; i < length; i++)
        {
            uint32_t random_index = string_randdist_(string_randgen_);
            assert(random_index < CHARSET.length());
            random_string[i] = CHARSET[random_index];
        }
        return random_string;
    }
}
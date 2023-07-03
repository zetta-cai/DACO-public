/*
 * Util: provide utilities, e.g., I/O and type conversion.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef UTIL_H
#define UTIL_H

#define BOOST_STACKTRACE_USE_BACKTRACE

#include <atomic> // std::memory_order
#include <cstdarg> // std::va_list, va_start, va_arg, and va_end
#include <fstream>
#include <mutex>
#include <string>
#include <time.h> // struct timespec

#define UNUSED(var) (void(var))

namespace covered
{
    class Util
    {
    public:
        // Type conversion
        static const int64_t MAX_UINT16;
        static const int64_t MAX_UINT32;
        // Network (UDP message payload -> UDP fragment payloads; UDP fragment payload + UDP fragment header -> UDP packet payload)
        static const std::string LOCALHOST_IPSTR;
        static const std::string ANY_IPSTR;
        static const uint32_t UDP_MAX_PKT_PAYLOAD; // Pkt payload = fraghdr + frag payload
        static const uint32_t UDP_FRAGHDR_SIZE;
        static const uint32_t UDP_MAX_FRAG_PAYLOAD;
        static const uint16_t UDP_MIN_PORT;
        // Atomicity
        static std::memory_order LOAD_CONCURRENCY_ORDER;
        static std::memory_order STORE_CONCURRENCY_ORDER;
        static std::memory_order RMW_CONCURRENCY_ORDER; // read-modify-write
        // Workflow control
        static const unsigned int SLEEP_INTERVAL_US; // Sleep interval for polling
        // Workload generation
        static const uint32_t KVPAIR_GENERATION_SEED; // Deterministic seed to generate key-value objects (dataset instead of workload)
        // Time measurement
        static const int START_YEAR;
        static const long NANOSECONDS_PERSECOND; // # of nanoseconds per second
        static const uint32_t SECOND_PRECISION; // # of digits after decimal point of second shown in time string

        // (1) I/O

        // (1.1) stdout/stderr I/O
        static void dumpNormalMsg(const std::string& class_name, const std::string& normal_message);
        static void dumpDebugMsg(const std::string& class_name, const std::string& debug_message);
        static void dumpWarnMsg(const std::string& class_name, const std::string& warn_message);
        static void dumpErrorMsg(const std::string& class_name, const std::string& error_message);
        static void dumpVariablesForDebug(const std::string& class_name, const uint32_t count, ...); // Only for temporary debugging

        // (1.2) File I/O
        static bool isFileExist(const std::string& filepath, const bool& is_silent=false);
        static bool isDirectoryExist(const std::string& dirpath, const bool& is_silent=false);
        // NOTE: avoid confliction in CLI::createRequiredDirectories_ (statistics or RocksDB directory)
        static void createDirectory(const std::string& dirpath);
        // Open a file (create the file if not exist)
        // NOTE: no confliction as each file (statistics or RocksDB) is accessed by a unique thread (client or cloud)
        static std::fstream* openFile(const std::string& filepath, std::ios_base::openmode mode);
        static std::string getParentDirpath(const std::string& filepath);
        static std::string getFilenameFromFileath(const std::string& filepath);

        // (2) Time measurement

        static struct timespec getCurrentTimespec();
        static std::string getCurrentTimestr();
        static double getDeltaTimeUs(const struct timespec& current_timespec, const struct timespec& previous_timespec); // In units of microseconds

        // (3) Type conversion

        static uint16_t toUint16(const int64_t& val);
        static uint32_t toUint32(const int64_t& val);
        static std::string toString(void* pointer);
        static std::string toString(const bool& boolean);
        static std::string toString(const uint32_t& val);

        // (4) Client-edge-cloud scenario

        // (4.1) Client
        static uint32_t getClosestEdgeIdx(const uint32_t& client_idx);
        static std::string getClosestEdgeIpstr(const uint32_t& client_idx);
        static uint16_t getClosestEdgeCacheServerRecvreqPort(const uint32_t& client_idx); // Calculate the recvreq port of the closest edge node for client
        static uint32_t getGlobalClientWorkerIdx(const uint32_t& client_idx, const uint32_t local_client_worker_idx);

        // (4.2) Edge and cloud
        static uint16_t getEdgeBeaconServerRecvreqPort(const uint32_t& edge_idx);
        static uint16_t getEdgeCacheServerRecvreqPort(const uint32_t& edge_idx);
        static uint16_t getEdgeInvalidationServerRecvreqPort(const uint32_t& edge_idx);
        static uint16_t getCloudRecvreqPort(const uint32_t& cloud_idx);

        // (5) Network

        static uint32_t getFragmentCnt(const uint32_t& msg_payload_size);
        static uint32_t getFragmentOffset(const uint32_t& fragment_idx);
        static uint32_t getFragmentPayloadSize(const uint32_t& fragment_idx, const uint32_t& msg_payload_size);

        // (6) Intermediate files

        static std::string getClientStatisticsDirpath();
        static std::string getClientStatisticsFilepath(const uint32_t& client_idx);
        static std::string getCloudRocksdbDirpath(const uint32_t& cloud_idx); // Calculate the RocksDB dirpath for the cloud node

        // (7) Others

        static uint32_t getTimeBasedRandomSeed(); // Get a random seed (instead of deterministic) based on current time
    private:
        static const std::string kClassName;

        // I/O
        static std::mutex msgdump_lock_;
        static bool isPathExist_(const std::string& path, const bool& is_file, const bool& is_silent); // File or directory 

        // Client-edge-cloud scenario
        static uint16_t getEdgePort_(const int64_t& start_port, const uint32_t edge_idx);

        // Intermediate files
        static std::string getInfixForFilepath_();
    };
}

#endif
/*
 * Util: provide utilities, e.g., I/O and type conversion.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef UTIL_H
#define UTIL_H

#define BOOST_STACKTRACE_USE_BACKTRACE

#include <assert.h>
#include <atomic> // std::memory_order
#include <cstdarg> // std::va_list, va_start, va_arg, and va_end
#include <iostream> // cout, cerr
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <random> // std::mt19937_64
#include <string>
#include <time.h> // struct timespec

#include "cli/evaluator_cli.h"
#include "common/config.h"
#include "common/covered_common_header.h"
#include "network/network_addr.h"

// Avoid conflicting macros
#ifndef UNUSED
#define UNUSED(var) (void(var))
#endif

#define KB2B(var) var * 1024
#define MB2B(var) var * 1024 * 1024
#define GB2B(var) var * 1024 * 1024 * 1024
#define B2KB(var) var / 1024
#define B2MB(var) var / 1024 / 1024

#define MS2US(var) var * 1000
#define SEC2US(var) var * 1000 * 1000

#define MYASSERT(condition) \
{ \
    if (Config::isAssert()) \
    { \
        assert(condition); \
    } \
}

// For internal cache engine (e.g., Segcache, CacheLib, and COVERED)
// (i) Common: 100MiB for internal unused capacity to avoid internal eviction (NOT used for cooperative edge caching and hence NOT affect cache performance)
#define COMMON_ENGINE_INTERNAL_UNUSED_CAPACITY_BYTES 100 * 1024 * 1024
// (ii) Segcache: 1GiB for min capacity bytes by default (over-provisioned capacity will NOT be used for cooperative edge caching and hence NOT affect cache performance)
#define SEGCACHE_ENGINE_MIN_CAPACITY_BYTES 1* 1024 * 1024 * 1024
// (iii) CacheLib: 4MiB for max slab size and 1GiB for min capacity bytes by default (over-provisioned capacity will NOT be used for cooperative edge caching and hence NOT affect cache performance)
#define CACHELIB_ENGINE_MAX_SLAB_SIZE 4 * 1024 * 1024
#define CACHELIB_ENGINE_MIN_CAPACITY_BYTES 1 * 1024 * 1024 * 1024

namespace covered
{
    class Util
    {
    public:
        // (1) For CLI parameters

        // Main class name
        static const std::string SIMULATOR_MAIN_NAME;
        //static const std::string STATISTICS_AGGREGATOR_MAIN_NAME;
        static const std::string TOTAL_STATISTICS_LOADER_MAIN_NAME;
        static const std::string DATASET_LOADER_MAIN_NAME;
        static const std::string CLIENT_MAIN_NAME;
        static const std::string EDGE_MAIN_NAME;
        static const std::string CLOUD_MAIN_NAME;
        static const std::string EVALUATOR_MAIN_NAME;
        static const std::string CLIUTIL_MAIN_NAME;

        // Workload name
        static const std::string FACEBOOK_WORKLOAD_NAME; // Workload generator type
        static const std::string WIKIPEDIA_IMAGE_WORKLOAD_NAME;
        static const std::string WIKIPEDIA_TEXT_WORKLOAD_NAME;

        // Cloud storage
        static const std::string HDD_NAME; // NOTE: a single RocksDB size on HDD should NOT exceed 500 GiB

        // Cache name
        static const std::string ARC_CACHE_NAME;
        static const std::string BESTGUESS_CACHE_NAME; // Canonical cooperaive caching
        static const std::string CACHELIB_CACHE_NAME;
        static const std::string FIFO_CACHE_NAME;
        static const std::string FROZENHOT_CACHE_NAME;
        static const std::string GLCACHE_CACHE_NAME;
        static const std::string GDSF_CACHE_NAME; // Greedy dual
        static const std::string GDSIZE_CACHE_NAME; // Greedy dual
        static const std::string LFUDA_CACHE_NAME; // Greedy dual
        static const std::string LRUK_CACHE_NAME; // Greedy dual
        static const std::string LFU_CACHE_NAME;
        static const std::string LHD_CACHE_NAME;
        static const std::string LRB_CACHE_NAME;
        static const std::string LRU_CACHE_NAME;
        static const std::string S3FIFO_CACHE_NAME;
        static const std::string SEGCACHE_CACHE_NAME;
        static const std::string SIEVE_CACHE_NAME;
        static const std::string SLRU_CACHE_NAME;
        static const std::string WTINYLFU_CACHE_NAME;
        static const std::string COVERED_CACHE_NAME;

        // Extend cache names (e.g.., ARC+)
        static const std::string EXTENDED_ARC_CACHE_NAME;
        static const std::string EXTENDED_CACHELIB_CACHE_NAME;
        static const std::string EXTENDED_FIFO_CACHE_NAME;
        static const std::string EXTENDED_FROZENHOT_CACHE_NAME;
        static const std::string EXTENDED_GLCACHE_CACHE_NAME;
        static const std::string EXTENDED_GDSF_CACHE_NAME; // Greedy dual
        static const std::string EXTENDED_GDSIZE_CACHE_NAME; // Greedy dual
        static const std::string EXTENDED_LFUDA_CACHE_NAME; // Greedy dual
        static const std::string EXTENDED_LRUK_CACHE_NAME; // Greedy dual
        static const std::string EXTENDED_LFU_CACHE_NAME;
        static const std::string EXTENDED_LHD_CACHE_NAME;
        static const std::string EXTENDED_LRB_CACHE_NAME;
        static const std::string EXTENDED_LRU_CACHE_NAME; // The same as Shark
        static const std::string EXTENDED_S3FIFO_CACHE_NAME;
        static const std::string EXTENDED_SEGCACHE_CACHE_NAME;
        static const std::string EXTENDED_SIEVE_CACHE_NAME;
        static const std::string EXTENDED_SLRU_CACHE_NAME;
        static const std::string EXTENDED_WTINYLFU_CACHE_NAME;

        // Hash name
        static const std::string MMH3_HASH_NAME;

        // (2) For utility functions

        // Type conversion
        static const int64_t MAX_UINT16;
        static const int64_t MAX_UINT32;
        static const double DOUBLE_IOTA;

        // Network (UDP message payload -> UDP fragment payloads; UDP fragment payload + UDP fragment header -> UDP packet payload)
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
        // NOTE: SLEEP_INTERVAL_US MUST be able to support EvaluatorCLI::warmup_max_duration_sec/stresstest_duration_sec and Config::client_raw_statistics_slot_interval_sec
        static const unsigned int SLEEP_INTERVAL_US; // Sleep interval for polling

        // Workload generation
        static const uint32_t DATASET_KVPAIR_GENERATION_SEED; // Deterministic seed to generate key-value objects for dataset (the same for all clients to ensure the same dataset)
        //static const uint32_t WORKLOAD_KVPAIR_GENERATION_SEED; // (OBSOLETE: homogeneous cache access patterns is a WRONG assumption -> we should ONLY follow homogeneous workload distribution yet still with heterogeneous cache access patterns) Deterministic seed to generate key-value objects for workload (the same for all clients to ensure homogeneous cache access patterns)

        // Time measurement
        static const int START_YEAR;
        static const long NANOSECONDS_PERSECOND; // # of nanoseconds per second
        static const uint32_t SECOND_PRECISION; // # of digits after decimal point of second shown in time string

        // Charset
        static const std::string CHARSET;

        // I/O
        static const int64_t MAX_MMAP_BLOCK_SIZE; // Maximum mmap block size (in units of bytes)
        static const int LINE_SEP_CHAR; // Line separator character
        static const int TSV_SEP_CHAR; // Tab-separated value separator character

        // (0) Cache names

        static bool isSingleNodeCache(const std::string cache_name);

        // (1) I/O

        // (1.1) stdout/stderr I/O
        static void dumpNormalMsg(const std::string& class_name, const std::string& normal_message);
        static void dumpInfoMsg(const std::string& class_name, const std::string& info_message);
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
        static int openFile(const std::string& filepath, const int& flags);
        static void closeFile(const int& fd);
        static std::string getParentDirpath(const std::string& filepath);
        static std::string getFilenameFromFilepath(const std::string& filepath);

        // (2) Time measurement

        static struct timespec getCurrentTimespec();
        static uint64_t getCurrentTimeUs();
        static std::string getCurrentTimestr();
        static double getDeltaTimeUs(const struct timespec& current_timespec, const struct timespec& previous_timespec); // In units of microseconds

        // (3) Type conversion/checking

        static uint16_t toUint16(const int64_t& val);
        static uint32_t toUint32(const int64_t& val);

        static uint64_t uint64Add(const uint64_t& a, const uint64_t& b); // a + b
        static uint64_t uint64Minus(const uint64_t& a, const uint64_t& b); // a - b
        static void uint64AddForAtomic(std::atomic<uint64_t>& a, const uint64_t& b); // a + b
        static void uint64MinusForAtomic(std::atomic<uint64_t>& a, const uint64_t& b); // a - b

        static Popularity popularityDivide(const Popularity& a, const Popularity& b); // a / b
        static Popularity popularityNonegMinus(const Popularity& a, const Popularity& b); // min(a - b, 0)
        static Popularity popularityAbsMinus(const Popularity& a, const Popularity& b); // |a - b|
        static Popularity popularityMultiply(const Popularity& a, const Popularity& b); // a * b
        static Popularity popularityAdd(const Popularity& a, const Popularity& b); // a + b

        static bool isApproxLargerEqual(const double& a, const double& b); // a >= b (with iota)
        static bool isApproxLargerEqual(const float& a, const float& b); // a >= b (with iota)
        static bool isApproxEqual(const double& a, const double& b); // a == b (with iota)
        static bool isApproxEqual(const float& a, const float& b); // a == b (with iota)

        static std::string toString(void* pointer);
        static std::string toString(const bool& boolean);
        static std::string toString(const uint32_t& val);

        // (4) Client-edge-cloud scenario

        // (4.1) Client
        static uint16_t getClientRecvmsgPort(const uint32_t& client_idx, const uint32_t& clientcnt);
        static uint32_t getClosestEdgeIdx(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt);
        static std::string getClosestEdgeIpstr(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt);
        static uint16_t getClosestEdgeCacheServerRecvreqPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt); // Calculate the recvreq port of the closest edge node for client
        static uint32_t getGlobalClientWorkerIdx(const uint32_t& client_idx, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt);
        //static void parseGlobalClientWorkerIdx(const uint32_t& global_client_worker_idx, const uint32_t& perclient_workercnt, uint32_t& client_idx, uint32_t& local_client_worker_idx);
        static uint16_t getClientWorkerRecvrspPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt);

        // (4.2) Edge
        // UDP port for receiving requests is edge_XXX_recvreq_startport + edge_idx
        static uint16_t getEdgeRecvmsgPort(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static uint16_t getEdgeBeaconServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static uint16_t getEdgeBeaconServerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static uint16_t getEdgeCacheServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static uint16_t getEdgeCacheServerPlacementProcessorRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static NetworkAddr getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(const NetworkAddr& edge_cache_server_worker_recvrsp_addr);
        static uint32_t getEdgeIdxFromCacheServerWorkerRecvreqAddr(const NetworkAddr& edge_cache_server_worker_recvreq_addr, const bool& is_private_ipstr, const uint32_t& edgecnt);
        static uint16_t getEdgeCacheServerWorkerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt);
        static uint16_t getEdgeCacheServerWorkerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt);

        // (4.3) Cloud
        static uint16_t getCloudRecvmsgPort(const uint32_t& cloud_idx);
        static uint16_t getCloudRecvreqPort(const uint32_t& cloud_idx);

        // (5) Network

        static uint32_t getFragmentCnt(const uint32_t& msg_payload_size);
        static uint32_t getFragmentOffset(const uint32_t& fragment_idx);
        static uint32_t getFragmentPayloadSize(const uint32_t& fragment_idx, const uint32_t& msg_payload_size);

        // (6) Intermediate files

        static std::string getEvaluatorStatisticsDirpath(EvaluatorCLI* evaluator_cli_ptr);
        static std::string getEvaluatorStatisticsFilepath(EvaluatorCLI* evaluator_cli_ptr);
        static std::string getCloudRocksdbBasedirForWorkload(const uint32_t& keycnt, const std::string& workload_name);
        static std::string getCloudRocksdbDirpath(const uint32_t& keycnt, const std::string& workload_name, const uint32_t& cloud_idx); // Calculate the RocksDB dirpath for the cloud node

        // (7) System settings
        static uint32_t getNetCoreRmemMax();

        // (8) Others

        static uint32_t getTimeBasedRandomSeed(); // Get a random seed (instead of deterministic) based on current time
        static std::string getRandomString(const uint32_t& length);

        template<class T>
        static void initializeAtomicArray(std::atomic<T>* atomic_array, const uint32_t& array_size, const T& default_value)
        {
            assert(atomic_array != NULL);
            for (uint32_t i = 0; i < array_size; i++)
            {
                atomic_array[i].store(default_value, Util::STORE_CONCURRENCY_ORDER);
            }
            return;
        }
    private:
        static const std::string kClassName;

        // (1) I/O
        static std::mutex msgdump_lock_;
        static bool isPathExist_(const std::string& path, const bool& is_file, const bool& is_silent); // File or directory 

        // (3) Type conversion
        static void assertPopularity_(const Popularity& a, const std::string& opstr = "");

        // (4) Client-edge-cloud scenario
        static uint16_t getNodePort_(const int64_t& start_port, const uint32_t& node_idx, const uint32_t& nodecnt, const uint32_t& machine_cnt);

        // (6) Intermediate files
        static std::string getInfixForEvaluatorStatisticsFilepath_(EvaluatorCLI* evaluator_cli_ptr);

        // (8) Others
        static std::mt19937_64 string_randgen_;
        static std::uniform_int_distribution<uint32_t> string_randdist_;
    };
}

#endif
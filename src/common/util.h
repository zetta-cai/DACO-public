/*
 * Util: provide utilities, e.g., I/O and type conversion.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <time.h>

namespace covered
{
    class Util
    {
    public:
        static const int64_t MAX_UINT16; // toUint16 in Util
        static const std::string LOCALHOST_IPSTR; // getLocalEdgeNodeIpstr in Util
        static std::memory_order LOAD_CONCURRENCY_ORDER; // ClientParam
        static std::memory_order STORE_CONCURRENCY_ORDER; // ClientParam
        static const unsigned int SLEEP_INTERVAL_US; // simulator and client (TODO)
        static const uint32_t KVPAIR_GENERATION_SEED; // FacebookWorkload and SyntheticWorkload (TODO)

        // (1) I/O

        // (1.1) stdout/stderr I/O
        static void dumpNormalMsg(const std::string& class_name, const std::string& normal_message);
        static void dumpDebugMsg(const std::string& class_name, const std::string& debug_message);
        static void dumpWarnMsg(const std::string& class_name, const std::string& warn_message);
        static void dumpErrorMsg(const std::string& class_name, const std::string& error_message);

        // (1.2) File I/O
        static bool isFileExist(const std::string& filepath);

        // (2) Time measurement (in units of microseconds)

        static struct timespec getCurrentTimespec();
        static double getDeltaTime(const struct timespec& current_timespec, const struct timespec& previous_timespec);

        // (3) Type conversion

        static uint16_t toUint16(const int64_t& val);

        // (4) Client-edge-cloud scenario

        // (4.1) Client
        //static uint16_t getLocalClientSendreqStartport(const uint32_t& global_client_idx);
        static std::string getLocalEdgeNodeIpstr(const uint32_t& global_client_idx);
        static uint32_t getGlobalWorkerIdx(const uint32_t& global_client_idx, const uint32_t local_worker_idx);

        // (4.2) Edge
        static uint16_t getLocalEdgeRecvreqPort(const uint32_t& global_edge_idx);
    private:
        static const std::string kClassName;
    };
}

#endif
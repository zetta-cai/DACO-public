/*
 * Param: store CLI parameters for dynamic configurations.
 *
 * NOTE: all parameters regarding client/edge/cloud settings should be passed into each instance when launching threads, except those with no effect on results (see comments of "// NO effect on results").
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef PARAM_H
#define PARAM_H

#include <string>

namespace covered
{
    class Param
    {
    public:
        // For cache name
        static const std::string LRU_CACHE_NAME;
        static const std::string COVERED_CACHE_NAME;
        // For cloud storage
        static const std::string HDD_NAME; // NOTE: a single RocksDB size on HDD should NOT exceed 500 GiB
        // For hash name
        static const std::string MMH3_HASH_NAME;

        static void setParameters(const std::string& cache_name, const uint64_t& capacity_bytes, const std::string& cloud_storage, const std::string& hash_name, const uint32_t& max_warmup_duration_sec, const uint32_t& percacheserver_workercnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& stresstest_duration_sec);

        static std::string getCacheName();
        static uint64_t getCapacityBytes();
        static std::string getCloudStorage();
        static std::string getHashName();
        static uint32_t getMaxWarmupDurationSec();
        static uint32_t getPercacheserverWorkercnt();
        static uint32_t getPropagationLatencyClientedgeUs();
        static uint32_t getPropagationLatencyCrossedgeUs();
        static uint32_t getPropagationLatencyEdgecloudUs();
        static uint32_t getStresstestDurationSec();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkCacheName_();
        static void checkCloudStorage_();
        static void checkHashName_();
        static void verifyIntegrity_();

        static void checkIsValid_();

        static bool is_valid_;
        
        static std::string cache_name_;
        static uint64_t capacity_bytes_;
        static std::string cloud_storage_;
        static std::string hash_name_;
        static uint32_t max_warmup_duration_sec_;
        static uint32_t percacheserver_workercnt_;
        static uint32_t propagation_latency_clientedge_us_; // 1/2 RTT between client and edge (bidirectional link)
        static uint32_t propagation_latency_crossedge_us_; // 1/2 RTT between edge and neighbor (bidirectional link)
        static uint32_t propagation_latency_edgecloud_us_; // 1/2 RTT between edge and cloud (bidirectional link)
        static uint32_t stresstest_duration_sec_;
    };
}

#endif
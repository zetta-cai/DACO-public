/*
 * Param: store CLI parameters for dynamic configurations.
 *
 * NOTE: all parameters regarding client/edge/cloud settings should be passed into each instance when launching threads, except those with no effect on results (see comments in Util::getInfixForFilepath_).
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
        // For main class name
        static const std::string SIMULATOR_MAIN_NAME;
        static const std::string STATISTICS_AGGREGATOR_MAIN_NAME;
        static const std::string CLIENT_MAIN_NAME;
        static const std::string EDGE_MAIN_NAME;
        static const std::string CLOUD_MAIN_NAME;
        // For cache name
        static const std::string LRU_CACHE_NAME;
        static const std::string COVERED_CACHE_NAME;
        // For cloud storage
        static const std::string HDD_NAME; // NOTE: a single RocksDB size on HDD should NOT exceed 500 GiB
        // For hash name
        static const std::string MMH3_HASH_NAME;
        // For workload name
        static const std::string FACEBOOK_WORKLOAD_NAME; // Workload generator type

        static void setParameters(const std::string& main_class_name, const bool& is_single_node, const std::string& cache_name, const uint32_t& capacity_bytes, const uint32_t& clientcnt, const std::string& cloud_storage, const std::string& config_filepath, const bool& is_debug, const double& duration, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& percacheserver_workercnt, const uint32_t& perclient_workercnt, const uint32_t& propagation_latency_clientedge, const uint32_t& propagation_latency_crossedge, const uint32_t& propagation_latency_edgecloud, const bool& track_event, const std::string& workload_name);

        static std::string getMainClassName();
        static bool isSingleNode();
        static std::string getCacheName();
        static uint32_t getCapacityBytes();
        static uint32_t getClientcnt();
        static std::string getCloudStorage();
        static std::string getConfigFilepath();
        static bool isDebug();
        static double getDuration();
        static uint32_t getEdgecnt();
        static std::string getHashName();
        static uint32_t getKeycnt();
        static uint32_t getOpcnt();
        static uint32_t getPercacheserverWorkercnt();
        static uint32_t getPerclientWorkercnt();
        static uint32_t getPropagationLatencyClientedge();
        static uint32_t getPropagationLatencyCrossedge();
        static uint32_t getPropagationLatencyEdgecloud();
        static bool isTrackEvent();
        static std::string getWorkloadName();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkMainClassName_();
        static void checkCacheName_();
        static void checkCloudStorage_();
        static void checkHashName_();
        static void checkWorkloadName_();
        static void verifyIntegrity_();

        static void checkIsValid_();

        static std::string main_class_name_;
        static bool is_valid_;
        
        static bool is_single_node_;
        static std::string cache_name_;
        static uint32_t capacity_bytes_;
        static uint32_t clientcnt_;
        static std::string cloud_storage_;
        static std::string config_filepath_;
        static bool is_debug_;
        static double duration_;
        static uint32_t edgecnt_;
        static std::string hash_name_;
        static uint32_t keycnt_;
        static uint32_t opcnt_;
        static uint32_t percacheserver_workercnt_;
        static uint32_t perclient_workercnt_;
        static uint32_t propagation_latency_clientedge_; // 1/2 RTT between client and edge (bidirectional link)
        static uint32_t propagation_latency_crossedge_; // 1/2 RTT between edge and neighbor (bidirectional link)
        static uint32_t propagation_latency_edgecloud_; // 1/2 RTT between edge and cloud (bidirectional link)
        static bool track_event_;
        static std::string workload_name_;
    };
}

#endif
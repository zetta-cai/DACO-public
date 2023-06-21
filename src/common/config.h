/*
 * Config: load JSON config file for static configurations.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <boost/json.hpp>

namespace covered
{
    class Config
    {
    public:
        // Key strings of JSON config file for static configurations (only used by Config)
        static const std::string CLOUD_IPSTR_KEYSTR;
        static const std::string CLOUD_RECVREQ_STARTPORT_KEYSTR;
        static const std::string CLOUD_ROCKSDB_BASEDIR_KEYSTR;
        static const std::string EDGE_IPSTRS_KEYSTR;
        static const std::string EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_INVALIDATION_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string FACEBOOK_CONFIG_FILEPATH_KEYSTR;
        static const std::string LATENCY_HISTOGRAM_SIZE_KEYSTR;
        static const std::string OUTPUT_BASEDIR_KEYSTR;
        static const std::string VERSION_KEYSTR;

        static void loadConfig();

        static std::string getCloudIpstr();
        static uint16_t getCloudRecvreqStartport();
        static std::string getCloudRocksdbBasedir();
        static std::string getEdgeIpstr(const uint32_t& edge_idx);
        static uint16_t getEdgeBeaconServerRecvreqStartport();
        static uint16_t getEdgeCacheServerRecvreqStartport();
        static uint16_t getEdgeInvalidationServerRecvreqStartport();
        static std::string getFacebookConfigFilepath();
        static uint32_t getLatencyHistogramSize();
        static std::string getOutputBasedir();
        static std::string getVersion();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void parseJsonFile_(const std::string& config_filepath);
        static boost::json::key_value_pair* find_(const std::string& key);
        static void checkIsValid_();

        static bool is_valid_;
        static boost::json::object json_object_;
        static std::string cloud_ipstr_;
        static uint16_t cloud_recvreq_startport_;
        static std::string cloud_rocksdb_basedir_;
        static std::vector<std::string> edge_ipstrs_;
        static uint16_t edge_beacon_server_recvreq_startport_;
        static uint16_t edge_cache_server_recvreq_startport_;
        static uint16_t edge_invalidation_server_recvreq_startport_;
        static std::string facebook_config_filepath_;
        static uint32_t latency_histogram_size_;
        static std::string output_basedir_;
        static std::string version_;
    };
}

#endif
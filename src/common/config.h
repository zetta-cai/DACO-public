/*
 * Config: load JSON config file for static configurations.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include <boost/json.hpp>

#include "network/network_addr.h"

namespace covered
{
    class Config
    {
    public:
        // Key strings of JSON config file for static configurations (only used by Config)
        static const std::string CLIENT_IPSTRS_KEYSTR;
        static const std::string CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string CLOUD_IPSTR_KEYSTR;
        static const std::string CLOUD_RECVREQ_STARTPORT_KEYSTR;
        static const std::string CLOUD_ROCKSDB_BASEDIR_KEYSTR;
        static const std::string EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string EDGE_INVALIDATION_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_IPSTRS_KEYSTR;
        static const std::string FACEBOOK_CONFIG_FILEPATH_KEYSTR;
        static const std::string FINE_GRAINED_LOCKING_SIZE_KEYSTR;
        static const std::string LATENCY_HISTOGRAM_SIZE_KEYSTR;
        static const std::string OUTPUT_BASEDIR_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_CLIENT_TOEDGE_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLIENT_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOEDGE_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLOUD_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_CLOUD_TOEDGE_KEYSTR;
        static const std::string VERSION_KEYSTR;

        static void loadConfig();

        static std::string getClientIpstr(const uint32_t& client_idx, const uint32_t& clientcnt);
        static uint32_t getClientIpstrCnt();
        static uint16_t getClientWorkerRecvrspStartport();
        static std::string getCloudIpstr();
        static uint16_t getCloudRecvreqStartport();
        static std::string getCloudRocksdbBasedir();
        static uint16_t getEdgeBeaconServerRecvreqStartport();
        static uint16_t getEdgeBeaconServerRecvrspStartport();
        static uint32_t getEdgeCacheServerDataRequestBufferSize();
        static uint16_t getEdgeCacheServerRecvreqStartport();
        static uint16_t getEdgeCacheServerWorkerRecvreqStartport();
        static uint16_t getEdgeCacheServerWorkerRecvrspStartport();
        static uint16_t getEdgeInvalidationServerRecvreqStartport();
        static std::string getEdgeIpstr(const uint32_t& edge_idx, const uint32_t& edgecnt);
        static uint32_t getEdgeIpstrCnt();
        static std::string getFacebookConfigFilepath();
        static uint32_t getFineGrainedLockingSize();
        static uint32_t getLatencyHistogramSize();
        static std::string getOutputBasedir();
        static uint32_t getPropagationItemBufferSizeClientToedge();
        static uint32_t getPropagationItemBufferSizeEdgeToclient();
        static uint32_t getPropagationItemBufferSizeEdgeToedge();
        static uint32_t getPropagationItemBufferSizeEdgeTocloud();
        static uint32_t getPropagationItemBufferSizeCloudToedge();
        static std::string getVersion();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void parseJsonFile_(const std::string& config_filepath);
        static boost::json::key_value_pair* find_(const std::string& key);
        static void checkIsValid_();

        static bool is_valid_;
        static boost::json::object json_object_;

        static std::vector<std::string> client_ipstrs_;
        static uint16_t client_worker_recvrsp_startport_;
        static std::string cloud_ipstr_;
        static uint16_t cloud_recvreq_startport_;
        static std::string cloud_rocksdb_basedir_;
        static uint16_t edge_beacon_server_recvreq_startport_;
        static uint16_t edge_beacon_server_recvrsp_startport_;
        static uint32_t edge_cache_server_data_request_buffer_size_;
        static uint16_t edge_cache_server_recvreq_startport_;
        static uint16_t edge_cache_server_worker_recvreq_startport_;
        static uint16_t edge_cache_server_worker_recvrsp_startport_;
        static uint16_t edge_invalidation_server_recvreq_startport_;
        static std::vector<std::string> edge_ipstrs_;
        static std::string facebook_config_filepath_;
        static uint32_t fine_grained_locking_size_;
        static uint32_t latency_histogram_size_;
        static std::string output_basedir_;
        static uint32_t propagation_item_buffer_size_client_toedge_;
        static uint32_t propagation_item_buffer_size_edge_toclient_;
        static uint32_t propagation_item_buffer_size_edge_toedge_;
        static uint32_t propagation_item_buffer_size_edge_tocloud_;
        static uint32_t propagation_item_buffer_size_cloud_toedge_;
        static std::string version_;
    };
}

#endif
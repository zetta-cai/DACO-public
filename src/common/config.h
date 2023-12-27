/*
 * Config: load JSON config file for static configurations.
 *
 * NOTE: Config tracks all static parameters as well as debugging parameters (NOT changed during evaluation).
 * 
 * NOTE: when get ipstr of client/edge/cloud/evaluator, is_launch indicates whether the logical node is being launched at the current physical machine; if is_launch = true, assert that the physical machine idx of the logical node MUST be current physical machine idx.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <map>
#include <string>

#include <boost/json.hpp>

#include "network/network_addr.h"

namespace covered
{
    class PhysicalMachine
    {
    public:
        PhysicalMachine();
        PhysicalMachine(const std::string& ipstr, const uint32_t& cpu_dedicated_corecnt, const uint32_t& cpu_shared_corecnt);
        ~PhysicalMachine();

        std::string getIpstr() const;
        uint32_t getCpuDedicatedCorecnt() const;
        uint32_t getCpuSharedCorecnt() const;

        std::string toString() const;

        const PhysicalMachine& operator=(const PhysicalMachine& other);
    private:
        static const std::string kClassName;

        std::string ipstr_;
        uint32_t cpu_dedicated_corecnt_;
        uint32_t cpu_shared_corecnt_;
    };

    class Config
    {
    public:
        // Key strings of JSON config file for static configurations (only used by Config)
        static const std::string CLIENT_DEDICATED_CORECNT_KEYSTR;
        static const std::string CLIENT_MACHINE_INDEXES_KEYSTR;
        static const std::string CLIENT_RAW_STATISTICS_SLOT_INTERVAL_SEC_KEYSTR;
        static const std::string CLIENT_RECVMSG_STARTPORT_KEYSTR;
        static const std::string CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string CLOUD_DEDICATED_CORECNT_KEYSTR;
        static const std::string CLOUD_MACHINE_INDEX_KEYSTR;
        static const std::string CLOUD_RECVMSG_STARTPORT_KEYSTR;
        static const std::string CLOUD_RECVREQ_STARTPORT_KEYSTR;
        static const std::string CLOUD_ROCKSDB_BASEDIR_KEYSTR;
        static const std::string COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO_KEYSTR;
        static const std::string COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_RATIO_KEYSTR;
        static const std::string DATASET_LOADER_SLEEP_FOR_COMPACTION_SEC_KEYSTR;
        static const std::string EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_RECVRSP_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR;
        static const std::string EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR;
        static const std::string EDGE_DEDICATED_CORECNT_KEYSTR;
        static const std::string EDGE_MACHINE_INDEXES_KEYSTR;
        static const std::string EDGE_RECVMSG_STARTPORT_KEYSTR;
        static const std::string EVALUATOR_DEDICATED_CORECNT_KEYSTR;
        static const std::string EVALUATOR_MACHINE_INDEX_KEYSTR;
        static const std::string EVALUATOR_RECVMSG_PORT_KEYSTR;
        static const std::string LIBRARY_DIRPATH_KEYSTR;
        static const std::string LIBRARY_DIRPATH_RELATIVE_FACEBOOK_CONFIG_FILEPATH_KEYSTR;
        static const std::string FINE_GRAINED_LOCKING_SIZE_KEYSTR;
        static const std::string IS_ASSERT_KEYSTR;
        static const std::string IS_DEBUG_KEYSTR;
        static const std::string IS_INFO_KEYSTR;
        static const std::string IS_GENERATE_RANDOM_VALUESTR_KEYSTR;
        static const std::string IS_TRACK_EVENT_KEYSTR;
        static const std::string LATENCY_HISTOGRAM_SIZE_KEYSTR;
        //static const std::string MIN_CAPACITY_MB_KEYSTR;
        static const std::string OUTPUT_BASEDIR_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_CLIENT_TOEDGE_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLIENT_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOEDGE_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLOUD_KEYSTR;
        static const std::string PROPAGATION_ITEM_BUFFER_SIZE_CLOUD_TOEDGE_KEYSTR;
        static const std::string VERSION_KEYSTR;

        // For all physical machines
        static const std::string PHYSICAL_MACHINES_KEYSTR;

        static void loadConfig(const std::string& config_filepath, const std::string& main_class_name);

        static std::string getMainClassName();

        // For client physical machines
        static uint32_t getClientMachineCnt();
        static std::string getClientIpstr(const uint32_t& client_idx, const uint32_t& clientcnt, const bool& is_launch); // clientcnt is the number of logical client nodes

        static uint32_t getClientRawStatisticsSlotIntervalSec();
        static uint16_t getClientRecvmsgStartport();
        static uint16_t getClientWorkerRecvrspStartport();
        static std::string getCloudIpstr(const bool& is_launch); // For cloud physical machine
        static uint16_t getCloudRecvmsgStartport();
        static uint16_t getCloudRecvreqStartport();
        static std::string getCloudRocksdbBasedir();
        static double getCoveredLocalUncachedMaxMemUsageRatio();
        static double getCoveredPopularityAggregationMaxMemUsageRatio();
        static uint32_t getDatasetLoaderSleepForCompactionSec();
        static uint16_t getEdgeBeaconServerRecvreqStartport();
        static uint16_t getEdgeBeaconServerRecvrspStartport();
        static uint32_t getEdgeCacheServerDataRequestBufferSize();
        static uint16_t getEdgeCacheServerRecvreqStartport();
        static uint16_t getEdgeCacheServerPlacementProcessorRecvrspStartport();
        static uint16_t getEdgeCacheServerWorkerRecvreqStartport();
        static uint16_t getEdgeCacheServerWorkerRecvrspStartport();

        // For edge physical machines
        static uint32_t getEdgeMachineCnt();
        static std::string getEdgeIpstr(const uint32_t& edge_idx, const uint32_t& edgecnt, const bool& is_launch); // edgecnt is the number of logical edge nodes
        static uint32_t getEdgeLocalMachineIdxByIpstr(const std::string& edge_ipstr); // Return the position (i.e., local machine index) in edge_machine_idxes_, instead of the position (i.e., global machine index = edge_machine_idxes_[local_machine_index]) in physical_machines_

        static uint16_t getEdgeRecvmsgStartport();
        static std::string getEvaluatorIpstr(const bool& is_launch); // For evaluator physical machine
        static std::uint16_t getEvaluatorRecvmsgPort();
        static std::string getLibraryDirpath();
        static std::string getFacebookConfigFilepath();
        static uint32_t getFineGrainedLockingSize();
        static bool isAssert();
        static bool isDebug();
        static bool isInfo();
        static bool isGenerateRandomValuestr();
        static bool isTrackEvent();
        static uint32_t getLatencyHistogramSize();
        //static uint64_t getMinCapacityMB();
        static std::string getOutputBasedir();
        static uint32_t getPropagationItemBufferSizeClientToedge();
        static uint32_t getPropagationItemBufferSizeEdgeToclient();
        static uint32_t getPropagationItemBufferSizeEdgeToedge();
        static uint32_t getPropagationItemBufferSizeEdgeTocloud();
        static uint32_t getPropagationItemBufferSizeCloudToedge();
        static std::string getVersion();

        // For current physical machine
        static PhysicalMachine getCurrentPhysicalMachine();
        static void getCurrentMachineClientIdxRange(const uint32_t& clientcnt, uint32_t& left_inclusive_client_idx, uint32_t& right_inclusive_client_idx);
        static void getCurrentMachineEdgeIdxRange(const uint32_t& edgecnt, uint32_t& left_inclusive_edge_idx, uint32_t& right_inclusive_edge_idx);
        static uint32_t getCurrentMachineEvaluatorDedicatedCorecnt();
        static uint32_t getCurrentMachineCloudDedicatedCorecnt();
        static uint32_t getCurrentMachineEdgeDedicatedCorecnt(const uint32_t& edgecnt);
        static uint32_t getCurrentMachineClientDedicatedCorecnt(const uint32_t& clientcnt);

        // For port verification
        static void portVerification(const uint16_t& startport, const uint16_t& finalport);

        static std::string toString();
    private:
        static const std::string kClassName;

        static void parseJsonFile_(const std::string& config_filepath);
        static boost::json::key_value_pair* find_(const std::string& key);
        static void checkIsValid_();
        static void checkMainClassName_();
        
        // For all physical machines
        static void checkPhysicalMachinesAndSetCuridx_();
        static PhysicalMachine getPhysicalMachine_(const uint32_t& physial_machine_idx);
        static uint32_t getNodeGlobalMachineIdx_(const uint32_t& node_idx, const uint32_t& nodecnt, const std::vector<uint32_t>& node_machine_idxes);

        // For current physical machine
        static void getCurrentMachineNodeIdxRange_(const uint32_t& nodecnt, const std::vector<uint32_t>& node_machine_idxes, uint32_t& left_inclusive_node_idx, uint32_t& right_inclusive_node_idx); // Return if current machine as the node

        // For port verification
        static void tryToFindStartport_(const std::string& keystr, uint16_t* startport_ptr);

        static bool is_valid_;
        static boost::json::object json_object_;

        // Come from CLI parameters yet NOT affect evaluation results
        static std::string config_filepath_; // Configuration file path for COVERED to load static configurations
        static std::string main_class_name_; // Come from argv[0]

        // Come from config file
        static uint32_t client_dedicated_corecnt_; // Number of dedicated CPU cores for each client physical machine
        static std::vector<uint32_t> client_machine_idxes_; // Physical machine indexes of physical client nodes
        static uint32_t client_raw_statistics_slot_interval_sec_; // Slot interval for client raw statistics in units of seconds
        static uint16_t client_recvmsg_startport_; // Start UDP port for client to receive benchmark control messages
        static uint16_t client_worker_recvrsp_startport_; // Start UDP port for client worker to receive local responses
        static uint32_t cloud_dedicated_corecnt_; // Number of dedicated CPU cores for cloud physical machine
        static uint32_t cloud_machine_idx_; // Physical machine index of physical cloud node
        static uint16_t cloud_recvmsg_startport_; // Start UDP port for cloud to receive benchmark control messages
        static uint16_t cloud_recvreq_startport_; // Start UDP port for cloud to receive global requests
        static std::string cloud_rocksdb_basedir_; // Base directory of RocksDB in cloud
        static double covered_local_uncached_max_mem_usage_ratio_; // The maximum memory usage ratio for local uncached metadata (ONLY for COVERED)
        static double covered_popularity_aggregation_max_mem_usage_ratio_; // The maximum memory usage ratio for popularity aggregation (ONLY for COVERED)
        static uint32_t dataset_loader_sleep_for_compaction_sec_; // Sleep time for dataset loader to wait for compaction in units of seconds
        static uint16_t edge_beacon_server_recvreq_startport_; // Start UDP port for edge beacon server to receive cooperation control requests
        static uint16_t edge_beacon_server_recvrsp_startport_; // Start UDP port for edge beacon server to receive cooperation control responses
        static uint32_t edge_cache_server_data_request_buffer_size_; // Buffer size for edge cache server to store local/redirected data requests
        static uint16_t edge_cache_server_recvreq_startport_; // Start UDP port for edge cache server to receive local/redirected requests
        static uint16_t edge_cache_server_placement_processor_recvrsp_startport_; // Start UDP port for edge cache server placement processor to receive cooperation control responses
        static uint16_t edge_cache_server_worker_recvreq_startport_; // Start UDP port for edge cache server worker to receive cooperation control requests
        static uint16_t edge_cache_server_worker_recvrsp_startport_; // Start UDP port for edge cache server worker to receive global responses
        static uint16_t edge_invalidation_server_recvreq_startport_; // Start UDP port for edge invalidation server to receive cooperation control requests
        static uint32_t edge_dedicated_corecnt_; // Number of dedicated CPU cores for each edge physical machine
        static std::vector<uint32_t> edge_machine_idxes_; // Physical machine indexes of physical edge nodes
        static uint16_t edge_recvmsg_startport_; // Start UDP port for edge to receive benchmark control messages
        static uint32_t evaluator_dedicated_corecnt_; // Number of dedicated CPU cores for evaluator physical machine
        static uint32_t evaluator_machine_idx_; // Physical machine index of physical evaluator node
        static uint16_t evaluator_recvmsg_port_; // UDP port for evaluator to receive benchmark control messages
        static std::string library_dirpath_; // Library dirpath
        static std::string facebook_config_filepath_; // Configuration file path for Facebook CDN workload under library dirpath
        static uint32_t fine_grained_locking_size_; // Bucket size of fine-grained locking
        static bool is_assert_; // Whether to make assertions -> time-consuming assertions may degrade performance
        static bool is_debug_; // Whether to dump debug information -> NOT affect evaluation results and NOT changed during evaluation
        static bool is_info_; // Whether to dump info log -> NOT affect evaluation and NOT changed during evaluation
        static bool is_generate_random_valuestr_; // Whether to generate random string to fill up value content
        static bool is_track_event_; // Whether to track per-message events for debugging -> NOT affect evaluation results and NOT changed during evaluation
        static uint32_t latency_histogram_size_; // Size of latency histogram
        //static uint64_t min_capacity_mb_; // Size of minimum capacity in units of MiB (avoid too small cache capacity which cannot work due to large-value objects and necessary memory usage of CacheLib engine)
        static std::string output_basedir_; // Base directory for output files
        static uint32_t propagation_item_buffer_size_client_toedge_; // Buffer size for client-to-edge propagated messages
        static uint32_t propagation_item_buffer_size_edge_toclient_; // Buffer size for edge-to-client propagated messages
        static uint32_t propagation_item_buffer_size_edge_toedge_; // Buffer size for edge-to-edge propagated messages
        static uint32_t propagation_item_buffer_size_edge_tocloud_; // Buffer size for edge-to-cloud propagated messages
        static uint32_t propagation_item_buffer_size_cloud_toedge_; // Buffer size for cloud-to-edge propagated messages
        static std::string version_; // Version of COVERED

        // For all physical machines
        static std::vector<PhysicalMachine> physical_machines_; // Physical machines

        // For current physical machine
        static uint32_t current_machine_idx_; // Current physical machine index

        // For port verification
        static std::map<uint16_t, std::string> startport_keystr_map_; // The map between start ports and keystrs (in ascending order by default)
    };
}

#endif
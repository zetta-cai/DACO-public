#include "common/config.h"

#include <arpa/inet.h> // INET_ADDRSTRLEN
#include <assert.h> // assert
#include <fstream> // ifstream
#include <ifaddrs.h> // getifaddrs
#include <sstream> // ostringstream
#include <thread> // std::thread::hardware_concurrency
#include <unordered_set>
#include <vector>

#include "common/util.h"

namespace covered
{
    // PhysicalMachine

    const std::string PhysicalMachine::kClassName("PhysicalMachine");

    PhysicalMachine::PhysicalMachine() : private_ipstr_(""), public_ipstr_(""), cpu_dedicated_corecnt_(0), cpu_shared_corecnt_(0)
    {
    }

    PhysicalMachine::PhysicalMachine(const std::string& private_ipstr, const std::string& public_ipstr, const uint32_t& cpu_dedicated_corecnt, const uint32_t& cpu_shared_corecnt)
    {
        private_ipstr_ = private_ipstr;
        public_ipstr_ = public_ipstr;
        cpu_dedicated_corecnt_ = cpu_dedicated_corecnt;
        cpu_shared_corecnt_ = cpu_shared_corecnt;
    }

    PhysicalMachine::~PhysicalMachine() {}

    std::string PhysicalMachine::getPrivateIpstr() const
    {
        return private_ipstr_;
    }

    std::string PhysicalMachine::getPublicIpstr() const
    {
        return public_ipstr_;
    }

    uint32_t PhysicalMachine::getCpuDedicatedCorecnt() const
    {
        return cpu_dedicated_corecnt_;
    }

    uint32_t PhysicalMachine::getCpuSharedCorecnt() const
    {
        return cpu_shared_corecnt_;
    }

    std::string PhysicalMachine::toString() const
    {
        std::ostringstream oss;
        oss << "private_ipstr: " << private_ipstr_ << "; public_ipstr: " << public_ipstr_ << ", cpu_dedicated_corecnt: " << cpu_dedicated_corecnt_ << ", cpu_shared_corecnt: " << cpu_shared_corecnt_;
        return oss.str();
    }

    const PhysicalMachine& PhysicalMachine::operator=(const PhysicalMachine& other)
    {
        private_ipstr_ = other.private_ipstr_;
        public_ipstr_ = other.public_ipstr_;
        cpu_dedicated_corecnt_ = other.cpu_dedicated_corecnt_;
        cpu_shared_corecnt_ = other.cpu_shared_corecnt_;

        return *this;
    }

    // Config

    const std::string Config::CLIENT_DEDICATED_CORECNT_KEYSTR("client_dedicated_corecnt");
    const std::string Config::CLIENT_MACHINE_INDEXES_KEYSTR("client_machine_indexes");
    const std::string Config::CLIENT_RAW_STATISTICS_SLOT_INTERVAL_SEC_KEYSTR("client_raw_statistics_slot_interval_sec");
    const std::string Config::CLIENT_RECVMSG_STARTPORT_KEYSTR("client_recvmsg_startport");
    const std::string Config::CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR("client_worker_recvrsp_startport");
    const std::string Config::CLOUD_DEDICATED_CORECNT_KEYSTR("cloud_dedicated_corecnt");
    const std::string Config::CLOUD_MACHINE_INDEX_KEYSTR("cloud_machine_index");
    const std::string Config::CLOUD_RECVMSG_STARTPORT_KEYSTR("cloud_recvmsg_startport");
    const std::string Config::CLOUD_RECVREQ_STARTPORT_KEYSTR("cloud_recvreq_startport");
    const std::string Config::CLOUD_ROCKSDB_BASEDIR_KEYSTR("cloud_rocksdb_basedir");
    const std::string Config::COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO_KEYSTR("covered_local_uncached_max_mem_usage_ratio");
    const std::string Config::COVERED_LOCAL_UNCACHED_LRU_MAX_RATIO_KEYSTR("covered_local_uncached_lru_max_ratio");
    const std::string Config::COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_RATIO_KEYSTR("covered_popularity_aggregation_max_mem_usage_ratio");
    const std::string Config::DATASET_LOADER_SLEEP_FOR_COMPACTION_SEC_KEYSTR("dataset_loader_sleep_for_compaction_sec");
    const std::string Config::DYNAMIC_RULECNT_KEYSTR("dynamic_rulecnt");
    const std::string Config::EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_beacon_server_recvreq_startport");
    const std::string Config::EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR("edge_beacon_server_recvrsp_startport");
    const std::string Config::EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR("edge_cache_server_data_request_buffer_size");
    const std::string Config::EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_cache_server_recvreq_startport");
    const std::string Config::EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_RECVRSP_STARTPORT_KEYSTR("edge_cache_server_placement_processor_recvrsp_startport");
    const std::string Config::EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR("edge_cache_server_worker_recvreq_startport");
    const std::string Config::EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR("edge_cache_server_worker_recvrsp_startport");
    const std::string Config::EDGE_DEDICATED_CORECNT_KEYSTR("edge_dedicated_corecnt");
    const std::string Config::EDGE_MACHINE_INDEXES_KEYSTR("edge_machine_indexes");
    const std::string Config::EDGE_RECVMSG_STARTPORT_KEYSTR("edge_recvmsg_startport");
    const std::string Config::EVALUATOR_DEDICATED_CORECNT_KEYSTR("evaluator_dedicated_corecnt");
    const std::string Config::EVALUATOR_MACHINE_INDEX_KEYSTR("evaluator_machine_index");
    const std::string Config::EVALUATOR_RECVMSG_PORT_KEYSTR("evaluator_recvmsg_port");
    const std::string Config::LIBRARY_DIRPATH_KEYSTR("library_dirpath");
    const std::string Config::LIBRARY_DIRPATH_RELATIVE_FACEBOOK_CONFIG_FILEPATH_KEYSTR("library_dirpath_relative_facebook_config_filepath");
    const std::string Config::FINE_GRAINED_LOCKING_SIZE_KEYSTR("fine_grained_locking_size");
    const std::string Config::IS_ASSERT_KEYSTR("is_assert");
    const std::string Config::IS_DEBUG_KEYSTR("is_debug");
    const std::string Config::IS_INFO_KEYSTR("is_info");
    const std::string Config::IS_GENERATE_RANDOM_VALUESTR_KEYSTR("is_generate_random_valuestr");
    const std::string Config::IS_TRACK_EVENT_KEYSTR("is_track_event");
    const std::string Config::LATENCY_HISTOGRAM_SIZE_KEYSTR("latency_histogram_size");
    //const std::string Config::MIN_CAPACITY_MB_KEYSTR("min_capacity_mb"); // <"min_capacity_mb": 10,> in config.json
    const std::string Config::OUTPUT_DIRPATH_KEYSTR("output_dirpath");
    const std::string Config::PARALLEL_EVICTION_MAX_VICTIMCNT_KEYSTR("parallel_eviction_max_victimcnt");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_CLIENT_TOEDGE_KEYSTR("propagation_item_buffer_size_client_toedge");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLIENT_KEYSTR("propagation_item_buffer_size_edge_toclient");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOEDGE_KEYSTR("propagation_item_buffer_size_edge_toedge");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLOUD_KEYSTR("propagation_item_buffer_size_edge_tocloud");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_CLOUD_TOEDGE_KEYSTR("propagation_item_buffer_size_cloud_toedge");
    const std::string Config::TRACE_DIRPATH_KEYSTR("trace_dirpath");
    const std::string Config::TRACE_DIRPATH_RELATIVE_WIKIIMAGE_TRACE_FILEPATHS_KEYSTR("trace_dirpath_relative_wikiimage_trace_filepaths");
    const std::string Config::TRACE_DIRPATH_RELATIVE_WIKITEXT_TRACE_FILEPATHS_KEYSTR("trace_dirpath_relative_wikitext_trace_filepaths");
    const std::string Config::TRACE_SAMPLE_OPCNT_KEYSTR("trace_sample_opcnt");
    const std::string Config::TRACE_WIKIIMAGE_KEYCNT_KEYSTR("trace_wikiimage_keycnt");
    const std::string Config::TRACE_WIKITEXT_KEYCNT_KEYSTR("trace_wikitext_keycnt");
    const std::string Config::VERSION_KEYSTR("version");

    // For all physical machines
    const std::string Config::PHYSICAL_MACHINES_KEYSTR("physical_machines");

    const std::string Config::kClassName("Config");

    // Initialize config variables by default
    bool Config::is_valid_ = false;
    boost::json::object Config::json_object_ = boost::json::object();

    std::string Config::config_filepath_("");
    std::string Config::main_class_name_(""); // Come from argv[0]

    uint32_t Config::client_dedicated_corecnt_ = 2;
    std::vector<uint32_t> Config::client_machine_idxes_(0);
    uint32_t Config::client_raw_statistics_slot_interval_sec_(1);
    uint16_t Config::client_recvmsg_startport_ = 4100; // [4096, 65536]
    uint16_t Config::client_worker_recvrsp_startport_ = 4200; // [4096, 65536]
    uint32_t Config::cloud_dedicated_corecnt_ = 2;
    uint32_t Config::cloud_machine_idx_ = 0;
    uint16_t Config::cloud_recvmsg_startport_ = 4300; // [4096, 65536]
    uint16_t Config::cloud_recvreq_startport_ = 4400; // [4096, 65536]
    std::string Config::cloud_rocksdb_basedir_("/tmp/cloud");
    double Config::covered_local_uncached_max_mem_usage_ratio_ = 0.01;
    double Config::covered_local_uncached_lru_max_ratio_ = 0.01;
    double Config::covered_popularity_aggregation_max_mem_usage_ratio_ = 0.01;
    uint32_t Config::dataset_loader_sleep_for_compaction_sec_ = 30;
    uint32_t Config::dynamic_rulecnt_ = 10000;
    uint16_t Config::edge_beacon_server_recvreq_startport_ = 4500; // [4096, 65536]
    uint16_t Config::edge_beacon_server_recvrsp_startport_ = 4600; // [4096, 65536]
    uint32_t Config::edge_cache_server_data_request_buffer_size_ = 10000;
    uint16_t Config::edge_cache_server_recvreq_startport_ = 4700; // [4096, 65536]
    uint16_t Config::edge_cache_server_placement_processor_recvrsp_startport_ = 4800; // [4096, 65536]
    uint16_t Config::edge_cache_server_worker_recvreq_startport_ = 4900; // [4096, 65536]
    uint16_t Config::edge_cache_server_worker_recvrsp_startport_ = 5000; // [4096, 65536]
    uint32_t Config::edge_dedicated_corecnt_ = 8;
    std::vector<uint32_t> Config::edge_machine_idxes_(0);
    uint16_t Config::edge_recvmsg_startport_ = 5100; // [4096, 65536]
    uint32_t Config::evaluator_dedicated_corecnt_ = 1;
    uint32_t Config::evaluator_machine_idx_ = 0;
    uint16_t Config::evaluator_recvmsg_port_ = 5200; // [4096, 65536]
    std::string Config::library_dirpath_("lib");
    std::string Config::facebook_config_filepath_("lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json");
    uint32_t Config::fine_grained_locking_size_ = 1000;
    bool Config::is_assert_ = false;
    bool Config::is_debug_ = false;
    bool Config::is_info_ = false;
    bool Config::is_generate_random_valuestr_ = false;
    bool Config::is_track_event_ = false;
    uint32_t Config::latency_histogram_size_ = 1000000; // Track latency up to 1000 ms
    //uint64_t Config::min_capacity_mb_ = 10;
    std::string Config::output_dirpath_("output");
    uint32_t Config::parallel_eviction_max_victimcnt_ = 1000; // NOTE: MUST < the ring buffer sizes for edge-to-edge propagation simulator, cache server, edge-to-cloud & cloud-to-edge propagation simulator
    uint32_t Config::propagation_item_buffer_size_client_toedge_ = 10000;
    uint32_t Config::propagation_item_buffer_size_edge_toclient_ = 10000;
    uint32_t Config::propagation_item_buffer_size_edge_toedge_ = 10000;
    uint32_t Config::propagation_item_buffer_size_edge_tocloud_ = 10000;
    uint32_t Config::propagation_item_buffer_size_cloud_toedge_ = 10000;
    std::string Config::trace_dirpath_("data");
    std::vector<std::string> Config::wikiimage_trace_filepaths_(0);
    std::vector<std::string> Config::wikitext_trace_filepaths_(0);
    uint32_t Config::trace_sample_opcnt_ = 0;
    uint32_t Config::trace_wikiimage_keycnt_ = 0;
    uint32_t Config::trace_wikitext_keycnt_ = 0;
    std::string Config::version_("1.0");

    // For all physical machines
    std::vector<PhysicalMachine> Config::physical_machines_(0);
    uint32_t Config::current_machine_idx_ = 0;

    // For port verification
    std::map<uint16_t, std::string> Config::startport_keystr_map_;

    void Config::loadConfig(const std::string& config_filepath, const std::string& main_class_name)
    {
        if (!is_valid_) // Invoked at most once
        {
            bool is_exist = Util::isFileExist(config_filepath, true);

            if (is_exist)
            {
                // Come fro CLI parameters
                config_filepath_ = config_filepath;
                main_class_name_ = main_class_name;
                checkMainClassName_();

                std::ostringstream oss;
                oss << "load config file from " << config_filepath << "!";
                Util::dumpNormalMsg(kClassName, oss.str());

                // parse config file to set json_object_
                parseJsonFile_(config_filepath);

                // Overwrite default values of config variables if any
                boost::json::key_value_pair* kv_ptr = NULL;
                kv_ptr = find_(CLIENT_DEDICATED_CORECNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_corecnt = kv_ptr->value().get_int64();
                    client_dedicated_corecnt_ = Util::toUint32(tmp_corecnt);
                }
                kv_ptr = find_(CLIENT_MACHINE_INDEXES_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        int64_t tmp_machine_idx = iter->get_int64();
                        client_machine_idxes_.push_back(Util::toUint32(tmp_machine_idx));
                    }
                }
                kv_ptr = find_(CLIENT_RAW_STATISTICS_SLOT_INTERVAL_SEC_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_interval = kv_ptr->value().get_int64();
                    client_raw_statistics_slot_interval_sec_ = Util::toUint32(tmp_interval);
                }
                tryToFindStartport_(CLIENT_RECVMSG_STARTPORT_KEYSTR, &client_recvmsg_startport_);
                tryToFindStartport_(CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR, &client_worker_recvrsp_startport_);
                kv_ptr = find_(CLOUD_DEDICATED_CORECNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_corecnt = kv_ptr->value().get_int64();
                    cloud_dedicated_corecnt_ = Util::toUint32(tmp_corecnt);
                }
                kv_ptr = find_(CLOUD_MACHINE_INDEX_KEYSTR);
                if (kv_ptr != NULL)
                {
                    cloud_machine_idx_ = Util::toUint32(kv_ptr->value().get_int64());
                }
                tryToFindStartport_(CLOUD_RECVMSG_STARTPORT_KEYSTR, &cloud_recvmsg_startport_);
                tryToFindStartport_(CLOUD_RECVREQ_STARTPORT_KEYSTR, &cloud_recvreq_startport_);
                kv_ptr = find_(CLOUD_ROCKSDB_BASEDIR_KEYSTR);
                if (kv_ptr != NULL)
                {
                    cloud_rocksdb_basedir_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO_KEYSTR);
                if (kv_ptr != NULL)
                {
                    covered_local_uncached_max_mem_usage_ratio_ = kv_ptr->value().get_double();
                }
                kv_ptr = find_(COVERED_LOCAL_UNCACHED_LRU_MAX_RATIO_KEYSTR);
                if (kv_ptr != NULL)
                {
                    covered_local_uncached_lru_max_ratio_ = kv_ptr->value().get_double();
                }
                kv_ptr = find_(COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_RATIO_KEYSTR);
                if (kv_ptr != NULL)
                {
                    covered_popularity_aggregation_max_mem_usage_ratio_ = kv_ptr->value().get_double();
                }
                kv_ptr = find_(DATASET_LOADER_SLEEP_FOR_COMPACTION_SEC_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_sec = kv_ptr->value().get_int64();
                    dataset_loader_sleep_for_compaction_sec_ = Util::toUint32(tmp_sec);
                }
                kv_ptr = find_(DYNAMIC_RULECNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_rulecnt = kv_ptr->value().get_int64();
                    dynamic_rulecnt_ = Util::toUint32(tmp_rulecnt);
                }
                tryToFindStartport_(EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR, &edge_beacon_server_recvreq_startport_);
                tryToFindStartport_(EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR, &edge_beacon_server_recvrsp_startport_);
                kv_ptr = find_(EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    edge_cache_server_data_request_buffer_size_ = Util::toUint32(tmp_size);
                }
                tryToFindStartport_(EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR, &edge_cache_server_recvreq_startport_);
                tryToFindStartport_(EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_RECVRSP_STARTPORT_KEYSTR, &edge_cache_server_placement_processor_recvrsp_startport_);
                tryToFindStartport_(EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR, &edge_cache_server_worker_recvreq_startport_);
                tryToFindStartport_(EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR, &edge_cache_server_worker_recvrsp_startport_);
                kv_ptr = find_(EDGE_DEDICATED_CORECNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_corecnt = kv_ptr->value().get_int64();
                    edge_dedicated_corecnt_ = Util::toUint32(tmp_corecnt);
                }
                kv_ptr = find_(EDGE_MACHINE_INDEXES_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        int64_t tmp_machine_idx = iter->get_int64();
                        edge_machine_idxes_.push_back(Util::toUint32(tmp_machine_idx));
                    }
                }
                tryToFindStartport_(EDGE_RECVMSG_STARTPORT_KEYSTR, &edge_recvmsg_startport_);
                kv_ptr = find_(EVALUATOR_DEDICATED_CORECNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_corecnt = kv_ptr->value().get_int64();
                    evaluator_dedicated_corecnt_ = Util::toUint32(tmp_corecnt);
                }
                kv_ptr = find_(EVALUATOR_MACHINE_INDEX_KEYSTR);
                if (kv_ptr != NULL)
                {
                    evaluator_machine_idx_ = Util::toUint32(kv_ptr->value().get_int64());
                }
                kv_ptr = find_(EVALUATOR_RECVMSG_PORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    evaluator_recvmsg_port_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(LIBRARY_DIRPATH_KEYSTR);
                if (kv_ptr != NULL)
                {
                    library_dirpath_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(LIBRARY_DIRPATH_RELATIVE_FACEBOOK_CONFIG_FILEPATH_KEYSTR);
                if (kv_ptr != NULL)
                {
                    const std::string library_dirpath_relative_facebook_config_filepath = std::string(kv_ptr->value().get_string().c_str());
                    facebook_config_filepath_ = library_dirpath_ + "/" + library_dirpath_relative_facebook_config_filepath;
                }
                kv_ptr = find_(FINE_GRAINED_LOCKING_SIZE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    fine_grained_locking_size_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(IS_ASSERT_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_assert_ = tmp_value==1?true:false;
                }
                kv_ptr = find_(IS_DEBUG_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_debug_ = tmp_value==1?true:false;
                }
                kv_ptr = find_(IS_INFO_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_info_ = tmp_value==1?true:false;
                }
                kv_ptr = find_(IS_GENERATE_RANDOM_VALUESTR_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_generate_random_valuestr_ = tmp_value==1?true:false;
                }
                kv_ptr = find_(IS_TRACK_EVENT_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_track_event_ = tmp_value==1?true:false;
                }
                kv_ptr = find_(LATENCY_HISTOGRAM_SIZE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    latency_histogram_size_ = Util::toUint32(tmp_size);
                }
                // kv_ptr = find_(MIN_CAPACITY_MB_KEYSTR);
                // if (kv_ptr != NULL)
                // {
                //     int64_t tmp_capacity = kv_ptr->value().get_int64();
                //     min_capacity_mb_ = static_cast<uint64_t>(tmp_capacity);
                // }
                kv_ptr = find_(OUTPUT_DIRPATH_KEYSTR);
                if (kv_ptr != NULL)
                {
                    output_dirpath_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(PARALLEL_EVICTION_MAX_VICTIMCNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_victimcnt = kv_ptr->value().get_int64();
                    parallel_eviction_max_victimcnt_ = Util::toUint32(tmp_victimcnt);
                }
                kv_ptr = find_(PROPAGATION_ITEM_BUFFER_SIZE_CLIENT_TOEDGE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    propagation_item_buffer_size_client_toedge_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLIENT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    propagation_item_buffer_size_edge_toclient_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOEDGE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    propagation_item_buffer_size_edge_toedge_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLOUD_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    propagation_item_buffer_size_edge_tocloud_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(PROPAGATION_ITEM_BUFFER_SIZE_CLOUD_TOEDGE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    propagation_item_buffer_size_cloud_toedge_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(TRACE_DIRPATH_KEYSTR);
                if (kv_ptr != NULL)
                {
                    trace_dirpath_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(TRACE_DIRPATH_RELATIVE_WIKIIMAGE_TRACE_FILEPATHS_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        const std::string trace_dirpath_relative_wikiimage_trace_filepath = std::string(iter->get_string().c_str());
                        wikiimage_trace_filepaths_.push_back(trace_dirpath_ + "/" + trace_dirpath_relative_wikiimage_trace_filepath);
                    }
                }
                kv_ptr = find_(TRACE_DIRPATH_RELATIVE_WIKITEXT_TRACE_FILEPATHS_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        const std::string trace_dirpath_relative_wikitext_trace_filepath = std::string(iter->get_string().c_str());
                        wikitext_trace_filepaths_.push_back(trace_dirpath_ + "/" + trace_dirpath_relative_wikitext_trace_filepath);
                    }
                }
                kv_ptr = find_(TRACE_SAMPLE_OPCNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_opcnt = kv_ptr->value().get_int64();
                    trace_sample_opcnt_ = Util::toUint32(tmp_opcnt);
                }
                kv_ptr = find_(TRACE_WIKIIMAGE_KEYCNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_keycnt = kv_ptr->value().get_int64();
                    trace_wikiimage_keycnt_ = Util::toUint32(tmp_keycnt);
                }
                kv_ptr = find_(TRACE_WIKITEXT_KEYCNT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_keycnt = kv_ptr->value().get_int64();
                    trace_wikitext_keycnt_ = Util::toUint32(tmp_keycnt);
                }
                kv_ptr = find_(VERSION_KEYSTR);
                if (kv_ptr != NULL)
                {
                    version_ = std::string(kv_ptr->value().get_string().c_str());
                }

                // For all physical machines
                kv_ptr = find_(PHYSICAL_MACHINES_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        std::string private_ipstr = std::string(iter->get_object().at("private_ipstr").get_string().c_str());
                        std::string public_ipstr = std::string(iter->get_object().at("public_ipstr").get_string().c_str());
                        int64_t tmp_cpu_dedicated_corecnt = iter->get_object().at("cpu_dedicated_corecnt").get_int64();
                        int64_t tmp_cpu_shared_corecnt = iter->get_object().at("cpu_shared_corecnt").get_int64();
                        physical_machines_.push_back(PhysicalMachine(private_ipstr, public_ipstr, Util::toUint32(tmp_cpu_dedicated_corecnt), Util::toUint32(tmp_cpu_shared_corecnt)));
                    }
                    assert(physical_machines_.size() == kv_ptr->value().get_array().size());
                }
                checkPhysicalMachinesAndSetCuridx_();

                // Integrity verification
                verifyIntegrity_();

                is_valid_ = true; // valid Config
            }
            else
            {
                std::ostringstream oss;

                //oss << config_filepath << " does not exist; use default config!";
                //Util::dumpWarnMsg(kClassName, oss.str());
                ////is_valid_ = false; // invalid Config

                oss << config_filepath << " does not exist!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        } // End of if (is_valid_)
        
        return;
    }

    std::string Config::getMainClassName()
    {
        checkIsValid_();
        return main_class_name_;
    }

    bool Config::isSimulation()
    {
        checkIsValid_();
        return main_class_name_ == Util::SINGLE_NODE_SIMULATOR_MAIN_NAME;
    }

    // For client physical machines

    uint32_t Config::getClientMachineCnt()
    {
        checkIsValid_();
        return client_machine_idxes_.size();
    }
    
    std::string Config::getClientIpstr(const uint32_t& client_idx, const uint32_t& clientcnt, const bool& is_private_ipstr, const bool& is_launch)
    {
        checkIsValid_();

        assert(client_idx < clientcnt);

        const uint32_t tmp_client_global_machine_idx = getNodeGlobalMachineIdx_(client_idx, clientcnt, client_machine_idxes_);
        if (!isSimulation() && is_launch)
        {
            if (tmp_client_global_machine_idx != current_machine_idx_)
            {
                std::ostringstream oss;
                oss << "client " << client_idx << " should be launched in physical machine " << tmp_client_global_machine_idx << " instead of current machine " << current_machine_idx_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        if (is_private_ipstr)
        {
            return getPhysicalMachine_(tmp_client_global_machine_idx).getPrivateIpstr();
        }
        else
        {
            return getPhysicalMachine_(tmp_client_global_machine_idx).getPublicIpstr();
        }
    }

    uint32_t Config::getClientRawStatisticsSlotIntervalSec()
    {
        checkIsValid_();
        return client_raw_statistics_slot_interval_sec_;
    }

    uint16_t Config::getClientRecvmsgStartport()
    {
        checkIsValid_();
        return client_recvmsg_startport_;
    }

    uint16_t Config::getClientWorkerRecvrspStartport()
    {
        checkIsValid_();
        return client_worker_recvrsp_startport_;
    }
    
    std::string Config::getCloudIpstr(const bool& is_private_ipstr, const bool& is_launch)
    {
        checkIsValid_();

        if (!isSimulation() && is_launch)
        {
            if (cloud_machine_idx_ != current_machine_idx_)
            {
                std::ostringstream oss;
                const uint32_t cloud_idx = 0; // TODO: ONLY support one cloud node now
                oss << "cloud " << cloud_idx << " should be launched in physical machine " << cloud_machine_idx_ << " instead of current machine " << current_machine_idx_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        
        if (is_private_ipstr)
        {
            return getPhysicalMachine_(cloud_machine_idx_).getPrivateIpstr();
        }
        else
        {
            return getPhysicalMachine_(cloud_machine_idx_).getPublicIpstr();
        }
    }

    uint16_t Config::getCloudRecvmsgStartport()
    {
        checkIsValid_();
        return cloud_recvmsg_startport_;
    }

    uint16_t Config::getCloudRecvreqStartport()
    {
        checkIsValid_();
        return cloud_recvreq_startport_;
    }

    std::string Config::getCloudRocksdbBasedir()
    {
        checkIsValid_();
        return cloud_rocksdb_basedir_;
    }

    double Config::getCoveredLocalUncachedMaxMemUsageRatio()
    {
        checkIsValid_();
        return covered_local_uncached_max_mem_usage_ratio_;
    }

    double Config::getCoveredLocalUncachedLruMaxRatio()
    {
        checkIsValid_();
        return covered_local_uncached_lru_max_ratio_;
    }

    double Config::getCoveredPopularityAggregationMaxMemUsageRatio()
    {
        checkIsValid_();
        return covered_popularity_aggregation_max_mem_usage_ratio_;
    }

    uint32_t Config::getDatasetLoaderSleepForCompactionSec()
    {
        checkIsValid_();
        return dataset_loader_sleep_for_compaction_sec_;
    }

    uint32_t Config::getDynamicRulecnt()
    {
        checkIsValid_();
        return dynamic_rulecnt_;
    }

    uint16_t Config::getEdgeBeaconServerRecvreqStartport()
    {
        checkIsValid_();
        return edge_beacon_server_recvreq_startport_;
    }

    uint16_t Config::getEdgeBeaconServerRecvrspStartport()
    {
        checkIsValid_();
        return edge_beacon_server_recvrsp_startport_;
    }

    uint32_t Config::getEdgeCacheServerDataRequestBufferSize()
    {
        checkIsValid_();
        return edge_cache_server_data_request_buffer_size_;
    }

    uint16_t Config::getEdgeCacheServerRecvreqStartport()
    {
        checkIsValid_();
        return edge_cache_server_recvreq_startport_;
    }

    uint16_t Config::getEdgeCacheServerPlacementProcessorRecvrspStartport()
    {
        checkIsValid_();
        return edge_cache_server_placement_processor_recvrsp_startport_;
    }

    uint16_t Config::getEdgeCacheServerWorkerRecvreqStartport()
    {
        checkIsValid_();
        return edge_cache_server_worker_recvreq_startport_;
    }

    uint16_t Config::getEdgeCacheServerWorkerRecvrspStartport()
    {
        checkIsValid_();
        return edge_cache_server_worker_recvrsp_startport_;
    }

    // For edge physical machines

    uint32_t Config::getEdgeMachineCnt()
    {
        checkIsValid_();
        return edge_machine_idxes_.size();
    }

    std::string Config::getEdgeIpstr(const uint32_t& edge_idx, const uint32_t& edgecnt, const bool& is_private_ipstr, const bool& is_launch)
    {
        checkIsValid_();

        assert(edge_idx < edgecnt);

        const uint32_t tmp_edge_global_machine_idx = getNodeGlobalMachineIdx_(edge_idx, edgecnt, edge_machine_idxes_);
        if (!isSimulation() && is_launch)
        {
            if (tmp_edge_global_machine_idx != current_machine_idx_)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " should be launched in physical machine " << tmp_edge_global_machine_idx << " instead of current machine " << current_machine_idx_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        if (is_private_ipstr)
        {
            return getPhysicalMachine_(tmp_edge_global_machine_idx).getPrivateIpstr();
        }
        else
        {
            return getPhysicalMachine_(tmp_edge_global_machine_idx).getPublicIpstr();
        }
    }

    uint32_t Config::getEdgeLocalMachineIdxByIpstr(const std::string& edge_ipstr, const bool& is_private_ipstr)
    {
        checkIsValid_();

        assert(edge_machine_idxes_.size() > 0);

        for (uint32_t tmp_edge_local_machine_idx = 0; tmp_edge_local_machine_idx < edge_machine_idxes_.size(); tmp_edge_local_machine_idx++)
        {
            const uint32_t tmp_edge_global_machine_idx = edge_machine_idxes_[tmp_edge_local_machine_idx];
            if (is_private_ipstr && getPhysicalMachine_(tmp_edge_global_machine_idx).getPrivateIpstr() == edge_ipstr)
            {
                return tmp_edge_local_machine_idx;
            }
            else if (!is_private_ipstr && getPhysicalMachine_(tmp_edge_global_machine_idx).getPublicIpstr() == edge_ipstr)
            {
                return tmp_edge_local_machine_idx;
            }
        }

        // Should NOT arrive here
        //assert(false);
        std::ostringstream oss;
        oss << "edge_ipstr " << edge_ipstr << " (is_private_ipstr: " << Util::toString(is_private_ipstr) << ") is not found in edge physical machines!";
        Util::dumpErrorMsg(kClassName, oss.str());
        exit(1);
    }

    uint16_t Config::getEdgeRecvmsgStartport()
    {
        checkIsValid_();
        return edge_recvmsg_startport_;
    }

    std::string Config::getEvaluatorIpstr(const bool& is_private_ipstr, const bool& is_launch)
    {
        checkIsValid_();

        if (!isSimulation() && is_launch)
        {
            if (evaluator_machine_idx_ != current_machine_idx_)
            {
                std::ostringstream oss;
                oss << "evaluator should be launched in physical machine " << evaluator_machine_idx_ << " instead of current machine " << current_machine_idx_ << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        
        if (is_private_ipstr)
        {
            return getPhysicalMachine_(evaluator_machine_idx_).getPrivateIpstr();
        }
        else
        {
            return getPhysicalMachine_(evaluator_machine_idx_).getPublicIpstr();
        }
    }

    uint16_t Config::getEvaluatorRecvmsgPort()
    {
        checkIsValid_();
        return evaluator_recvmsg_port_;
    }
    
    std::string Config::getLibraryDirpath()
    {
        checkIsValid_();
        return library_dirpath_;
    }

    std::string Config::getFacebookConfigFilepath()
    {
        checkIsValid_();
        return facebook_config_filepath_;
    }

    uint32_t Config::getFineGrainedLockingSize()
    {
        checkIsValid_();
        return fine_grained_locking_size_;
    }

    bool Config::isAssert()
    {
        checkIsValid_();
        return is_assert_;
    }

    bool Config::isDebug()
    {
        checkIsValid_();
        return is_debug_;
    }

    bool Config::isInfo()
    {
        checkIsValid_();
        return is_info_;
    }

    bool Config::isGenerateRandomValuestr()
    {
        checkIsValid_();
        return is_generate_random_valuestr_;
    }
    
    bool Config::isTrackEvent()
    {
        checkIsValid_();
        return is_track_event_;
    }

    uint32_t Config::getLatencyHistogramSize()
    {
        checkIsValid_();
        return latency_histogram_size_;
    }

    // uint64_t Config::getMinCapacityMB()
    // {
    //     checkIsValid_();
    //     return min_capacity_mb_;
    // }

    std::string Config::getOutputDirpath()
    {
        checkIsValid_();
        return output_dirpath_;
    }

    uint32_t Config::getParallelEvictionMaxVictimcnt()
    {
        checkIsValid_();
        return parallel_eviction_max_victimcnt_;
    }

    uint32_t Config::getPropagationItemBufferSizeClientToedge()
    {
        checkIsValid_();
        return propagation_item_buffer_size_client_toedge_;
    }

    uint32_t Config::getPropagationItemBufferSizeEdgeToclient()
    {
        checkIsValid_();
        return propagation_item_buffer_size_edge_toclient_;
    }

    uint32_t Config::getPropagationItemBufferSizeEdgeToedge()
    {
        checkIsValid_();
        return propagation_item_buffer_size_edge_toedge_;
    }

    uint32_t Config::getPropagationItemBufferSizeEdgeTocloud()
    {
        checkIsValid_();
        return propagation_item_buffer_size_edge_tocloud_;
    }

    uint32_t Config::getPropagationItemBufferSizeCloudToedge()
    {
        checkIsValid_();
        return propagation_item_buffer_size_cloud_toedge_;
    }

    std::string Config::getTraceDirpath()
    {
        checkIsValid_();
        return trace_dirpath_;
    }

    std::vector<std::string> Config::getWikiimageTraceFilepaths()
    {
        checkIsValid_();
        return wikiimage_trace_filepaths_;
    }

    std::vector<std::string> Config::getWikitextTraceFilepaths()
    {
        checkIsValid_();
        return wikitext_trace_filepaths_;
    }

    uint32_t Config::getTraceSampleOpcnt(const std::string& workload_name)
    {
        checkIsValid_();

        // NOTE: trace sample opcnt in config.json ONLY works for replayed traces, as non-replayed traces can directly generate dataset with different sizes with NO need of sampling
        if (!Util::isReplayedWorkload(workload_name))
        {
            std::ostringstream oss;
            oss << "trace sample opcnt in config.json is only for replayed traces, yet workload " << workload_name << " is non-replayed!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // NOTE: NOT use static variables in Util module due to undefined order of static variable initialization in C++
        assert(trace_sample_opcnt_ > 0);
        return trace_sample_opcnt_;
    }

    uint32_t Config::getTraceKeycnt(const std::string& workload_name)
    {
        checkIsValid_();

        // NOTE: keycnt in config.json ONLY works for replayed traces, as non-replayed traces get dataset size from CLI module
        if (!Util::isReplayedWorkload(workload_name))
        {
            std::ostringstream oss;
            oss << "keycnt in config.json is only for replayed traces, yet workload " << workload_name << " is non-replayed!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // NOTE: NOT use static variables in Util module due to undefined order of static variable initialization in C++
        if (workload_name == "wikiimage")
        {
            return trace_wikiimage_keycnt_;
        }
        else if (workload_name == "wikitext")
        {
            return trace_wikitext_keycnt_;
        }
        else
        {
            std::ostringstream oss;
            oss << "please run trace_preprocessor and update config.json for " << workload_name << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    std::string Config::getVersion()
    {
        checkIsValid_();
        return version_;
    }

    // For current physical machine

    PhysicalMachine Config::getCurrentPhysicalMachine()
    {
        checkIsValid_();
        return getPhysicalMachine_(current_machine_idx_);
    }

    void Config::getCurrentMachineClientIdxRange(const uint32_t& clientcnt, uint32_t& left_inclusive_client_idx, uint32_t& right_inclusive_client_idx)
    {
        checkIsValid_();

        getCurrentMachineNodeIdxRange_(clientcnt, client_machine_idxes_, left_inclusive_client_idx, right_inclusive_client_idx);

        return;
    }

    void Config::getCurrentMachineEdgeIdxRange(const uint32_t& edgecnt, uint32_t& left_inclusive_edge_idx, uint32_t& right_inclusive_edge_idx)
    {
        checkIsValid_();

        getCurrentMachineNodeIdxRange_(edgecnt, edge_machine_idxes_, left_inclusive_edge_idx, right_inclusive_edge_idx);

        return;
    }

    bool Config::getCurrentMachineEvaluatorDedicatedCorecnt(uint32_t& current_machine_evaluator_dedicated_corecnt)
    {
        checkIsValid_();

        bool is_current_machine_as_evaluator = false;
        current_machine_evaluator_dedicated_corecnt = 0;
        if (current_machine_idx_ == evaluator_machine_idx_)
        {
            is_current_machine_as_evaluator = true;
            current_machine_evaluator_dedicated_corecnt = evaluator_dedicated_corecnt_;
        }
        
        return is_current_machine_as_evaluator;
    }

    bool Config::getCurrentMachineCloudDedicatedCorecnt(uint32_t& current_machine_cloud_dedicated_corecnt)
    {
        checkIsValid_();

        bool is_current_machine_as_cloud = false;
        current_machine_cloud_dedicated_corecnt = 0;
        if (current_machine_idx_ == cloud_machine_idx_)
        {
            is_current_machine_as_cloud = true;
            current_machine_cloud_dedicated_corecnt = cloud_dedicated_corecnt_;
        }

        return is_current_machine_as_cloud;
    }

    bool Config::getCurrentMachineEdgeDedicatedCorecnt(const uint32_t& edgecnt, uint32_t& current_machine_edge_dedicated_corecnt)
    {
        checkIsValid_();

        bool is_current_machine_as_edge = false;
        for (uint32_t i = 0; i < edge_machine_idxes_.size(); i++)
        {
            if (edge_machine_idxes_[i] == current_machine_idx_)
            {
                is_current_machine_as_edge = true;
                break;
            }
        }

        current_machine_edge_dedicated_corecnt = 0;
        if (is_current_machine_as_edge)
        {
            // Get current_machine_edgecnt
            uint32_t left_inclusive_edge_idx = 0;
            uint32_t right_inclusive_edge_idx = 0;
            getCurrentMachineNodeIdxRange_(edgecnt, edge_machine_idxes_, left_inclusive_edge_idx, right_inclusive_edge_idx);
            assert(right_inclusive_edge_idx >= left_inclusive_edge_idx);
            const uint32_t current_machine_edgecnt = right_inclusive_edge_idx - left_inclusive_edge_idx + 1;

            current_machine_edge_dedicated_corecnt = current_machine_edgecnt * edge_dedicated_corecnt_;
        }

        return is_current_machine_as_edge;
    }

    bool Config::getCurrentMachineClientDedicatedCorecnt(const uint32_t& clientcnt, uint32_t& current_machine_client_dedicated_corecnt)
    {
        checkIsValid_();

        bool is_current_machine_as_client = false;
        for (uint32_t i = 0; i < client_machine_idxes_.size(); i++)
        {
            if (client_machine_idxes_[i] == current_machine_idx_)
            {
                is_current_machine_as_client = true;
                break;
            }
        }

        current_machine_client_dedicated_corecnt = 0;
        if (is_current_machine_as_client)
        {
            // Get current_machine_clientcnt
            uint32_t left_inclusive_client_idx = 0;
            uint32_t right_inclusive_client_idx = 0;
            getCurrentMachineNodeIdxRange_(clientcnt, client_machine_idxes_, left_inclusive_client_idx, right_inclusive_client_idx);
            assert(right_inclusive_client_idx >= left_inclusive_client_idx);
            const uint32_t current_machine_clientcnt = right_inclusive_client_idx - left_inclusive_client_idx + 1;

            current_machine_client_dedicated_corecnt = current_machine_clientcnt * client_dedicated_corecnt_;
        }

        return is_current_machine_as_client;
    }

    // For port verification
    
    void Config::portVerification(const uint16_t& startport, const uint16_t& finalport)
    {
        if (isSimulation())
        {
            // NOTE: NO need to verify ports for single-node simulator, which does NOT launch any compnents
            return;
        }

        std::map<uint16_t, std::string>::const_iterator startport_keystr_map_const_iter = startport_keystr_map_.find(startport);
        if (startport_keystr_map_const_iter == startport_keystr_map_.end())
        {
            // NOTE: start port MUST exist in Config module
            std::cout << "[ERROR] start port " << startport << " does NOT exist in Config module!" << std::endl;
            exit(1);
        }
        else
        {
            const std::string keystr = startport_keystr_map_const_iter->second;
            startport_keystr_map_const_iter++; // Move to the next start port

            // NOTE: the port should >= start port yet < the next start port if any
            if ((startport_keystr_map_const_iter != startport_keystr_map_.end() && startport_keystr_map_const_iter->first <= finalport))
            {
                std::cout << "[ERROR] final port " << finalport << " starting from " << keystr << " of start port " << startport << " overlaps with " << startport_keystr_map_const_iter->second << " of start port " << startport_keystr_map_const_iter->first << "!" << std::endl;
                exit(1);
            }
        }

        return;
    }

    std::string Config::toString()
    {
        checkIsValid_();
        std::ostringstream oss;
        oss << "[Static configurations from " << config_filepath_ << "]" << std::endl;
        oss << "Main class name: " << main_class_name_ << std::endl;
        oss << "Client dedicated corecnt: " << client_dedicated_corecnt_ << std::endl;
        oss << "Client physical machine indexes: ";
        for (uint32_t i = 0; i < client_machine_idxes_.size(); i++)
        {
            oss << client_machine_idxes_[i] << " ";
        }
        oss << std::endl;
        oss << "Client raw statistics slot interval second: " << client_raw_statistics_slot_interval_sec_ << std::endl;
        oss << "Client recvmsg startport: " << client_recvmsg_startport_ << std::endl;
        oss << "Client worker recvrsp startport: " << client_worker_recvrsp_startport_ << std::endl;
        oss << "Cloud dedicated corecnt: " << cloud_dedicated_corecnt_ << std::endl;
        oss << "Cloud physical machine index: " << cloud_machine_idx_ << std::endl;
        oss << "Cloud recvmsg startport: " << cloud_recvmsg_startport_ << std::endl;
        oss << "Cloud recvreq startport: " << cloud_recvreq_startport_ << std::endl;
        oss << "Cloud RocksDB base directory: " << cloud_rocksdb_basedir_ << std::endl;
        oss << "Covered local uncached max mem usage ratio: " << covered_local_uncached_max_mem_usage_ratio_ << std::endl; // ONLY used by COVERED
        oss << "Covered local uncached LRU max ratio: " << covered_local_uncached_lru_max_ratio_ << std::endl; // ONLY used by COVERED
        oss << "Covered popularity aggregation max mem usage ratio: " << covered_popularity_aggregation_max_mem_usage_ratio_ << std::endl; // ONLY used by COVERED
        oss << "Dataset loader sleep for compaction seconds: " << dataset_loader_sleep_for_compaction_sec_ << std::endl;
        oss << "Dynamic rulecnt: " << dynamic_rulecnt_ << std::endl;
        oss << "Edge beacon server recvreq startport: " << edge_beacon_server_recvreq_startport_ << std::endl;
        oss << "Edge cache server data request buffer size: " << edge_cache_server_data_request_buffer_size_ << std::endl;
        oss << "Edge cache server recvreq startport: " << edge_cache_server_recvreq_startport_ << std::endl;
        oss << "Edge cache server placement processor recvrsp startport: " << edge_cache_server_placement_processor_recvrsp_startport_ << std::endl;
        oss << "Edge cache server worker recvreq startport: " << edge_cache_server_worker_recvreq_startport_ << std::endl;
        oss << "Edge cache server worker recvrsp startport: " << edge_cache_server_worker_recvrsp_startport_ << std::endl;
        oss << "Edge dedicated corecnt: " << edge_dedicated_corecnt_ << std::endl;
        oss << "Edge physical machine indexes: ";
        for (uint32_t i = 0; i < edge_machine_idxes_.size(); i++)
        {
            oss << edge_machine_idxes_[i] << " ";
        }
        oss << std::endl;
        oss << "Edge recvmsg startport: " << edge_recvmsg_startport_ << std::endl;
        oss << "Evaluator dedicated corecnt: " << evaluator_dedicated_corecnt_ << std::endl;
        oss << "Evaluator physical machine index:" << evaluator_machine_idx_ << std::endl;
        oss << "Evaluator recvmsg port: " << evaluator_recvmsg_port_ << std::endl;
        oss << "Third-party library dirpath: " << library_dirpath_ << std::endl;
        oss << "Facebook config filepath (relative to library dirpath): " << facebook_config_filepath_ << std::endl;
        oss << "Fine-grained locking size: " << fine_grained_locking_size_ << std::endl;
        oss << "Is assert: " << (is_assert_?"true":"false") << std::endl;
        oss << "Is debug: " << (is_debug_?"true":"false") << std::endl;
        oss << "Is info: " << (is_info_?"true":"false") << std::endl;
        oss << "Is generate random valuestr: " << (is_generate_random_valuestr_?"true":"false") << std::endl;
        oss << "Is track event: " << (is_track_event_?"true":"false") << std::endl;
        oss << "Latency histogram size: " << latency_histogram_size_ << std::endl;
        //oss << "Min capacity MiB: " << min_capacity_mb_ << std::endl;
        oss << "Output dirpath: " << output_dirpath_ << std::endl;
        oss << "Parallel eviction max victimcnt: " << parallel_eviction_max_victimcnt_ << std::endl;
        oss << "Propagation item buffer size from client to edge: " << propagation_item_buffer_size_client_toedge_ << std::endl;
        oss << "Propagation item buffer size from edge to client: " << propagation_item_buffer_size_edge_toclient_ << std::endl;
        oss << "Propagation item buffer size from edge to edge: " << propagation_item_buffer_size_edge_toedge_ << std::endl;
        oss << "Propagation item buffer size from edge to cloud: " << propagation_item_buffer_size_edge_tocloud_ << std::endl;
        oss << "Propagation item buffer size from cloud to edge: " << propagation_item_buffer_size_cloud_toedge_ << std::endl;
        oss << "Trace dirpath: " << trace_dirpath_ << std::endl;
        oss << "Wikiimage trace filepaths (unsampled traces): ";
        for (uint32_t i = 0; i < wikiimage_trace_filepaths_.size(); i++)
        {
            oss << wikiimage_trace_filepaths_[i];
            if (i != wikiimage_trace_filepaths_.size() - 1)
            {
                oss << ", ";
            }
            else
            {
                oss << std::endl;
            }
        }
        oss << "Wikitext trace filepaths (unsampled traces): ";
        for (uint32_t i = 0; i < wikitext_trace_filepaths_.size(); i++)
        {
            oss << wikitext_trace_filepaths_[i];
            if (i != wikitext_trace_filepaths_.size() - 1)
            {
                oss << ", ";
            }
            else
            {
                oss << std::endl;
            }
        }
        oss << "Trace sample opcnt: " << trace_sample_opcnt_ << std::endl;
        oss << "Trace wikiimage keycnt: " << trace_wikiimage_keycnt_ << std::endl;
        oss << "Trace wikitext keycnt: " << trace_wikitext_keycnt_ << std::endl;
        oss << "Version: " << version_ << std::endl;

        for (uint32_t i = 0; i < physical_machines_.size(); i++)
        {
            oss << "Physical machine [" << i << "]: " << physical_machines_[i].toString();
            if (i != physical_machines_.size() - 1)
            {
                oss << std::endl;
            }
        }
        return oss.str();
    }

    void Config::parseJsonFile_(const std::string& config_filepath)
    {
        // Open a binary file and not eat whitespaces
        std::ifstream ifs(config_filepath, std::ios::binary);
        ifs.unsetf(std::ios::skipws);

        // Get file size
        ifs.seekg(0, std::ios::end); // Set the current position to the end of the file
        std::ifstream::pos_type filesize = ifs.tellg(); // Get the current position (i.e., the file size)
        ifs.seekg(0, std::ios::beg); // Set the current position to the beginning of the file

        // Read all bytes of the config file
        std::vector<char> bytes;
        bytes.reserve(int(filesize));
        bytes.insert(bytes.begin(), std::istream_iterator<char>(ifs), std::istream_iterator<char>());
        //ifs.read(bytes, filesize); // read only supports char array instead of vector
        std::ostringstream oss;
        oss << "read " << config_filepath << " with " << filesize << " bytes";
        Util::dumpNormalMsg(kClassName, oss.str());

        // Parse the bytes
        boost::json::stream_parser boost_json_parser;
        boost::json::error_code boost_json_errcode;
        boost_json_parser.write(bytes.data(), filesize, boost_json_errcode);
        bool is_error = bool(boost_json_errcode);
        if (is_error)
        {
            Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
            exit(1);
        }

        // Finish byte parsing
        boost_json_parser.finish(boost_json_errcode);
        is_error = bool(boost_json_errcode);
        if (is_error)
        {
            Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
            exit(1);
        }

        // Get the json object
        boost::json::value boost_json_value = boost_json_parser.release();
        assert(boost_json_value.kind() == boost::json::kind::object);
        json_object_ = boost_json_value.get_object();
        return;
    }

    boost::json::key_value_pair* Config::find_(const std::string& key)
    {
        boost::json::key_value_pair* kv_ptr = json_object_.find(key);
        if (kv_ptr == NULL)
        {
            std::ostringstream oss;
            oss << "no json entry for " << key << "; use default setting";
            Util::dumpWarnMsg(kClassName, oss.str());
        }
        return kv_ptr;
    }

    void Config::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid Config (config file has not been loaded)!");
            exit(1);
        }
        return;
    }

    void Config::checkMainClassName_()
    {
        // Obsolete: STATISTICS_AGGREGATOR_MAIN_NAME
        if (main_class_name_ != Util::SINGLE_NODE_PROTOTYPE_MAIN_NAME && main_class_name_ != Util::SINGLE_NODE_SIMULATOR_MAIN_NAME && main_class_name_ != Util::TOTAL_STATISTICS_LOADER_MAIN_NAME && main_class_name_ != Util::DATASET_LOADER_MAIN_NAME && main_class_name_ != Util::CLIENT_MAIN_NAME && main_class_name_ != Util::EDGE_MAIN_NAME && main_class_name_ != Util::CLOUD_MAIN_NAME && main_class_name_ != Util::EVALUATOR_MAIN_NAME && main_class_name_ != Util::CLIUTIL_MAIN_NAME && main_class_name_ != Util::TRACE_PREPROCESSOR_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "main class name " << main_class_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    // For all physical machines

    void Config::checkPhysicalMachinesAndSetCuridx_()
    {
        assert(physical_machines_.size() > 0);

        // (i) Different physical machines should NOT have duplicate private/public ipstrs
        std::unordered_set<std::string> tmp_private_ipstr_set;
        std::unordered_set<std::string> tmp_public_ipstr_set;
        for (uint32_t i = 0; i < physical_machines_.size(); i++)
        {
            // Check private ipstrs
            std::string tmp_private_ipstr = physical_machines_[i].getPrivateIpstr();
            if (tmp_private_ipstr_set.find(tmp_private_ipstr) != tmp_private_ipstr_set.end())
            {
                std::ostringstream oss;
                oss << "physical machine [" << i << "] has a duplicate private ipstr " << tmp_private_ipstr << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
            else
            {
                tmp_private_ipstr_set.insert(tmp_private_ipstr);
            }

            // Check public ipstrs
            std::string tmp_public_ipstr = physical_machines_[i].getPublicIpstr();
            if (tmp_public_ipstr_set.find(tmp_public_ipstr) != tmp_public_ipstr_set.end())
            {
                std::ostringstream oss;
                oss << "physical machine [" << i << "] has a duplicate public ipstr " << tmp_public_ipstr << "!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
            else
            {
                tmp_public_ipstr_set.insert(tmp_public_ipstr);
            }
        }

        // (ii) Each physical machine MUST have positive cpu dedicated corecnt and non-negative cpu shared corecnt
        for (uint32_t i = 0; i < physical_machines_.size(); i++)
        {
            assert(physical_machines_[i].getCpuDedicatedCorecnt() > 0);
            assert(physical_machines_[i].getCpuSharedCorecnt() >= 0); // NOTE: if CPU shared corecnt = 0, low-priority threads will be bound to the set of all CPU cores
        }

        // (iii) Set current physical machine index by mathcing ipstr

        // Get ifaddrs of all interfaces in current physical machine
        struct ifaddrs* current_ifaddrs = NULL;
        int result = getifaddrs(&current_ifaddrs);
        if (result != 0)
        {
            std::ostringstream oss;
            oss << "failed to getifaddrs() (errno: " << errno << ")!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        assert(current_ifaddrs != NULL);

        // Convert ifaddrs into ipstrs in current physical machine
        std::unordered_set<std::string> current_ipstrs;
        for (struct ifaddrs* tmp_ifaddr_ptr = current_ifaddrs; tmp_ifaddr_ptr != nullptr; tmp_ifaddr_ptr = tmp_ifaddr_ptr->ifa_next)
        {
            // Skip interface without valid network address
            if (tmp_ifaddr_ptr->ifa_addr == nullptr)
            {
                continue;
            }

            sa_family_t tmp_address_family = tmp_ifaddr_ptr->ifa_addr->sa_family;
            if (tmp_address_family == AF_INET) // IPv4
            {
                char tmp_ipstr[INET_ADDRSTRLEN];
                memset(tmp_ipstr, 0, INET_ADDRSTRLEN);
                inet_ntop(tmp_address_family, &((struct sockaddr_in*)(tmp_ifaddr_ptr->ifa_addr))->sin_addr, tmp_ipstr, INET_ADDRSTRLEN);
                current_ipstrs.insert(std::string(tmp_ipstr));
            }
            else if (tmp_address_family == AF_INET6) // IPv6
            {
                char tmp_ipstr[INET6_ADDRSTRLEN];
                memset(tmp_ipstr, 0, INET6_ADDRSTRLEN);
                inet_ntop(tmp_address_family, &((struct sockaddr_in6*)(tmp_ifaddr_ptr->ifa_addr))->sin6_addr, tmp_ipstr, INET6_ADDRSTRLEN);
                current_ipstrs.insert(std::string(tmp_ipstr));
            }
            else // Others such as AF_UNIX, AF_UNSPEC, and AF_PACKET
            {
                // Do nothing
            }
        }
        assert(current_ipstrs.size() > 0);

        // Match public/private ipstr to set current physical machine index
        bool is_found = false;
        for (uint32_t i = 0; i < physical_machines_.size(); i++)
        {
            // Match public ipstr
            std::string tmp_public_ipstr = physical_machines_[i].getPublicIpstr();
            for (std::unordered_set<std::string>::const_iterator current_ipstrs_const_iter = current_ipstrs.cbegin(); current_ipstrs_const_iter != current_ipstrs.cend(); current_ipstrs_const_iter++)
            {
                if (tmp_public_ipstr == *current_ipstrs_const_iter)
                {
                    current_machine_idx_ = i;
                    is_found = true;
                    break;
                }
            }

            // Match privaate ipstr
            std::string tmp_private_ipstr = physical_machines_[i].getPrivateIpstr();
            for (std::unordered_set<std::string>::const_iterator current_ipstrs_const_iter = current_ipstrs.cbegin(); current_ipstrs_const_iter != current_ipstrs.cend(); current_ipstrs_const_iter++)
            {
                if (tmp_private_ipstr == *current_ipstrs_const_iter)
                {
                    current_machine_idx_ = i;
                    is_found = true;
                    break;
                }
            }
        }
        assert(is_found);

        // Release ifaddrs of all interfaces in current physical machine
        freeifaddrs(current_ifaddrs);
        current_ifaddrs = NULL;

        // (iv) cpu_dedicated_corecnt + cpu_shared_corecnt MUST <= total CPU corecnt for the current physical machine

        const uint32_t current_cpu_dedicated_corecnt = physical_machines_[current_machine_idx_].getCpuDedicatedCorecnt();
        const uint32_t current_cpu_shared_corecnt = physical_machines_[current_machine_idx_].getCpuSharedCorecnt();
        const uint32_t current_total_cpu_corecnt = std::thread::hardware_concurrency();
        if (current_total_cpu_corecnt < current_cpu_dedicated_corecnt + current_cpu_shared_corecnt)
        {
            // NOTE: total # of CPU cores should >= cpu_dedicated_corecnt + cpu_shared_corecnt
            std::ostringstream oss;
            oss << "In current physical machine, total_cpu_corecnt " << current_total_cpu_corecnt << " is less than cpu_dedicated_corecnt " << current_cpu_dedicated_corecnt << " + cpu_shared_corecnt " << current_cpu_shared_corecnt << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        else if (current_total_cpu_corecnt > current_cpu_dedicated_corecnt + current_cpu_shared_corecnt)
        {
            // Pose INFO to hint more CPU cores can be used if total # of CPU cores < cpu_dedicated_corecnt + cpu_shared_corecnt
            std::ostringstream oss;
            oss << "In current physical machine, total_cpu_corecnt " << current_total_cpu_corecnt << " is more than cpu_dedicated_corecnt " << current_cpu_dedicated_corecnt << " + cpu_shared_corecnt " << current_cpu_shared_corecnt << " -> more CPU cores can be assigned to high-/low-priority threads for dedicated/shared usage!";
            Util::dumpWarnMsg(kClassName, oss.str());
        }

        // (v) client/edge/cloud/evaluator physical machine indexes MUST be valid
        assert(client_machine_idxes_.size() > 0);
        for (uint32_t i = 0; i < client_machine_idxes_.size(); i++)
        {
            if (client_machine_idxes_[i] >= physical_machines_.size())
            {
                std::ostringstream oss;
                oss << "invalid client_machine_idxes_[" << i << "] " << client_machine_idxes_[i] << " >= physical_machines_.size() " << physical_machines_.size();
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        assert(edge_machine_idxes_.size() > 0);
        for (uint32_t i = 0; i < edge_machine_idxes_.size(); i++)
        {
            if (edge_machine_idxes_[i] >= physical_machines_.size())
            {
                std::ostringstream oss;
                oss << "invalid edge_machine_idxes_[" << i << "] " << edge_machine_idxes_[i] << " >= physical_machines_.size() " << physical_machines_.size();
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        if (cloud_machine_idx_ >= physical_machines_.size())
        {
            std::ostringstream oss;
            oss << "invalid cloud_machine_idx_ " << cloud_machine_idx_ << " >= physical_machines_.size() " << physical_machines_.size();
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        if (evaluator_machine_idx_ >= physical_machines_.size())
        {
            std::ostringstream oss;
            oss << "invalid evaluator_machine_idx_ " << evaluator_machine_idx_ << " >= physical_machines_.size() " << physical_machines_.size();
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // (vi) client/edge/cloud/evaluator MUST be current machine idx if under single-node prototype

        // NOTE: single-node simulator does NOT launch client/edge/cloud/evaluator threads, and hence NO need to check the correpsonding settings in the config file
        
        if (main_class_name_ == Util::SINGLE_NODE_PROTOTYPE_MAIN_NAME)
        {
            assert(client_machine_idxes_.size() == 1);
            assert(client_machine_idxes_[0] == current_machine_idx_);
            assert(edge_machine_idxes_.size() == 1);
            assert(edge_machine_idxes_[0] == current_machine_idx_);
            assert(cloud_machine_idx_ == current_machine_idx_);
            assert(evaluator_machine_idx_ == current_machine_idx_);
        }

        return;
    }

    PhysicalMachine Config::getPhysicalMachine_(const uint32_t& physial_machine_idx)
    {
        checkIsValid_();
        
        if (physial_machine_idx >= physical_machines_.size())
        {
            std::ostringstream oss;
            oss << "invalid physial_machine_idx " << physial_machine_idx << " >= physical_machines_.size() " << physical_machines_.size();
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return physical_machines_[physial_machine_idx];
    }

    uint32_t Config::getNodeGlobalMachineIdx_(const uint32_t& node_idx, const uint32_t& nodecnt, const std::vector<uint32_t>& node_machine_idxes)
    {
        const uint32_t node_physical_machine_cnt = node_machine_idxes.size();
        assert(node_physical_machine_cnt > 0);

        uint32_t tmp_node_local_machine_idx = 0;
        if (nodecnt <= node_physical_machine_cnt)
        {
            tmp_node_local_machine_idx = node_idx;
        }
        else
        {
            uint32_t permachine_nodecnt = nodecnt / node_physical_machine_cnt;
            assert(permachine_nodecnt > 0);
            tmp_node_local_machine_idx = node_idx / permachine_nodecnt;
            if (tmp_node_local_machine_idx >= node_physical_machine_cnt)
            {
                tmp_node_local_machine_idx = node_physical_machine_cnt - 1; // Assign tail edges to the last machine
            }
        }

        const uint32_t tmp_node_global_machine_idx = node_machine_idxes[tmp_node_local_machine_idx];
        return tmp_node_global_machine_idx;
    }

    // For current physical machine

    void Config::getCurrentMachineNodeIdxRange_(const uint32_t& nodecnt, const std::vector<uint32_t>& node_machine_idxes, uint32_t& left_inclusive_node_idx, uint32_t& right_inclusive_node_idx)
    {
        const uint32_t node_physical_machine_cnt = node_machine_idxes.size();
        assert(node_physical_machine_cnt > 0);

        // Get current local machine idx in node machine indexes
        bool is_current_machine_as_node = false;
        uint32_t current_node_local_machine_idx = 0;
        for (uint32_t tmp_node_local_machine_idx = 0; tmp_node_local_machine_idx < node_physical_machine_cnt; tmp_node_local_machine_idx++)
        {
            uint32_t tmp_node_global_machine_idx = node_machine_idxes[tmp_node_local_machine_idx];
            if (tmp_node_global_machine_idx == current_machine_idx_)
            {
                is_current_machine_as_node = true;
                current_node_local_machine_idx = tmp_node_local_machine_idx;
                break;
            }
        }
        assert(is_current_machine_as_node);
        assert(current_node_local_machine_idx < node_physical_machine_cnt);

        if (nodecnt <= node_physical_machine_cnt) // Each node is launched in an individual node physical machine
        {
            assert(current_node_local_machine_idx < nodecnt);

            left_inclusive_node_idx = current_node_local_machine_idx;
            right_inclusive_node_idx = current_node_local_machine_idx;
        }
        else // One or multiple node(s) are launched in a node physical machine
        {
            uint32_t permachine_nodecnt = nodecnt / node_physical_machine_cnt;
            assert(permachine_nodecnt > 0);

            left_inclusive_node_idx = permachine_nodecnt * current_node_local_machine_idx;
            if (current_node_local_machine_idx != node_physical_machine_cnt - 1) // Not the last node physical machine
            {
                right_inclusive_node_idx = left_inclusive_node_idx + permachine_nodecnt - 1;
            }
            else // The last node physical machine
            {
                right_inclusive_node_idx = nodecnt - 1;
            }
            assert(left_inclusive_node_idx < nodecnt);
            assert(right_inclusive_node_idx < nodecnt);
        }

        return;
    }

    // For port verification

    void Config::tryToFindStartport_(const std::string& keystr, uint16_t* startport_ptr)
    {
        assert(startport_ptr != NULL);

        boost::json::key_value_pair* kv_ptr = find_(keystr);
        if (kv_ptr != NULL)
        {
            int64_t tmp_port = kv_ptr->value().get_int64();
            *startport_ptr = Util::toUint16(tmp_port);
        }
        else
        {
            // Keep the default value of the start port
        }

        // Add the mapping between start port and keystr for port verification
        std::map<uint16_t, std::string>::const_iterator startport_keystr_map_const_iter = startport_keystr_map_.find(*startport_ptr);
        if (startport_keystr_map_const_iter != startport_keystr_map_.end())
        {
            // NOTE: all start ports MUST be unique
            std::cout << "[ERROR] " << keystr << " has the same value with " << startport_keystr_map_const_iter->second << " of port " << startport_keystr_map_const_iter->first << std::endl;
            exit(1);
        }
        else
        {
            startport_keystr_map_const_iter = startport_keystr_map_.insert(std::pair(*startport_ptr, keystr)).first;
            assert(startport_keystr_map_const_iter != startport_keystr_map_.end());
        }

        return;
    }

    // For integrity verification

    void Config::verifyIntegrity_()
    {
        // (1) parallel_eviction_max_victimcnt_ MUST < the ring buffer sizes for edge-to-edge propagation simulator, cache server, edge-to-cloud & cloud-to-edge propagation simulator
        assert(parallel_eviction_max_victimcnt_ < propagation_item_buffer_size_edge_toedge_);
        assert(parallel_eviction_max_victimcnt_ < edge_cache_server_data_request_buffer_size_);
        assert(parallel_eviction_max_victimcnt_ < propagation_item_buffer_size_edge_tocloud_);
        assert(parallel_eviction_max_victimcnt_ < propagation_item_buffer_size_cloud_toedge_);

        return;
    }
}
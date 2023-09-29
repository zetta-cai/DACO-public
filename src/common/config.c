#include "common/config.h"

#include <assert.h> // assert
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>

#include "common/util.h"

namespace covered
{
    const std::string Config::CLIENT_IPSTRS_KEYSTR("client_ipstrs");
    const std::string Config::CLIENT_RAW_STATISTICS_SLOT_INTERVAL_SEC_KEYSTR("client_raw_statistics_slot_interval_sec");
    const std::string Config::CLIENT_RECVMSG_STARTPORT_KEYSTR("client_recvmsg_startport");
    const std::string Config::CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR("client_worker_recvrsp_startport");
    const std::string Config::CLOUD_IPSTR_KEYSTR("cloud_ipstr");
    const std::string Config::CLOUD_RECVMSG_STARTPORT_KEYSTR("cloud_recvmsg_startport");
    const std::string Config::CLOUD_RECVREQ_STARTPORT_KEYSTR("cloud_recvreq_startport");
    const std::string Config::CLOUD_ROCKSDB_BASEDIR_KEYSTR("cloud_rocksdb_basedir");
    const std::string Config::COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_RATIO_KEYSTR("covered_local_uncached_max_mem_usage_ratio");
    const std::string Config::COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_RATIO_KEYSTR("covered_popularity_aggregation_max_mem_usage_ratio");
    const std::string Config::DATASET_LOADER_SLEEP_FOR_COMPACTION_SEC_KEYSTR("dataset_loader_sleep_for_compaction_sec");
    const std::string Config::EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_beacon_server_recvreq_startport");
    const std::string Config::EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR("edge_beacon_server_recvrsp_startport");
    const std::string Config::EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR("edge_cache_server_data_request_buffer_size");
    const std::string Config::EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_cache_server_recvreq_startport");
    const std::string Config::EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_RECVRSP_STARTPORT_KEYSTR("edge_cache_server_placement_processor_recvrsp_startport");
    const std::string Config::EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR("edge_cache_server_worker_recvreq_startport");
    const std::string Config::EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR("edge_cache_server_worker_recvrsp_startport");
    const std::string Config::EDGE_INVALIDATION_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_invalidation_server_recvreq_startport");
    const std::string Config::EDGE_IPSTRS_KEYSTR("edge_ipstrs");
    const std::string Config::EDGE_RECVMSG_STARTPORT_KEYSTR("edge_recvmsg_startport");
    const std::string Config::EVALUATOR_IPSTR_KEYSTR("evaluator_ipstr");
    const std::string Config::EVALUATOR_RECVMSG_PORT_KEYSTR("evaluator_recvmsg_port");
    const std::string Config::FACEBOOK_CONFIG_FILEPATH_KEYSTR("facebook_config_filepath");
    const std::string Config::FINE_GRAINED_LOCKING_SIZE_KEYSTR("fine_grained_locking_size");
    const std::string Config::IS_DEBUG_KEYSTR("is_debug");
    const std::string Config::IS_GENERATE_RANDOM_VALUESTR_KEYSTR("is_generate_random_valuestr");
    const std::string Config::IS_TRACK_EVENT_KEYSTR("is_track_event");
    const std::string Config::LATENCY_HISTOGRAM_SIZE_KEYSTR("latency_histogram_size");
    const std::string Config::OUTPUT_BASEDIR_KEYSTR("output_basedir");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_CLIENT_TOEDGE_KEYSTR("propagation_item_buffer_size_client_toedge");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLIENT_KEYSTR("propagation_item_buffer_size_edge_toclient");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOEDGE_KEYSTR("propagation_item_buffer_size_edge_toedge");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_EDGE_TOCLOUD_KEYSTR("propagation_item_buffer_size_edge_tocloud");
    const std::string Config::PROPAGATION_ITEM_BUFFER_SIZE_CLOUD_TOEDGE_KEYSTR("propagation_item_buffer_size_cloud_toedge");
    const std::string Config::VERSION_KEYSTR("version");

    const std::string Config::kClassName("Config");

    // Initialize config variables by default
    bool Config::is_valid_ = false;
    boost::json::object Config::json_object_ = boost::json::object();

    std::string Config::config_filepath_("");
    std::string Config::main_class_name_(""); // Come from argv[0]
    bool Config::is_single_node_ = true;

    std::vector<std::string> Config::client_ipstrs_(0);
    uint32_t Config::client_raw_statistics_slot_interval_sec_(1);
    uint16_t Config::client_recvmsg_startport_ = 4100; // [4096, 65536]
    uint16_t Config::client_worker_recvrsp_startport_ = 4200; // [4096, 65536]
    std::string Config::cloud_ipstr_ = Util::LOCALHOST_IPSTR;
    uint16_t Config::cloud_recvmsg_startport_ = 4300; // [4096, 65536]
    uint16_t Config::cloud_recvreq_startport_ = 4400; // [4096, 65536]
    std::string Config::cloud_rocksdb_basedir_("/tmp/cloud");
    double Config::covered_local_uncached_max_mem_usage_ratio_ = 0.01;
    double Config::covered_popularity_aggregation_max_mem_usage_ratio_ = 0.01;
    uint32_t Config::dataset_loader_sleep_for_compaction_sec_ = 30;
    uint16_t Config::edge_beacon_server_recvreq_startport_ = 4500; // [4096, 65536]
    uint16_t Config::edge_beacon_server_recvrsp_startport_ = 4600; // [4096, 65536]
    uint32_t Config::edge_cache_server_data_request_buffer_size_ = 1000;
    uint16_t Config::edge_cache_server_recvreq_startport_ = 4700; // [4096, 65536]
    uint16_t Config::edge_cache_server_placement_processor_recvrsp_startport_ = 4800; // [4096, 65536]
    uint16_t Config::edge_cache_server_worker_recvreq_startport_ = 4900; // [4096, 65536]
    uint16_t Config::edge_cache_server_worker_recvrsp_startport_ = 5000; // [4096, 65536]
    uint16_t Config::edge_invalidation_server_recvreq_startport_ = 5100; // [4096, 65536]
    std::vector<std::string> Config::edge_ipstrs_(0);
    uint16_t Config::edge_recvmsg_startport_ = 5200; // [4096, 65536]
    std::string Config::evaluator_ipstr_ = Util::LOCALHOST_IPSTR;
    uint16_t Config::evaluator_recvmsg_port_ = 5300; // [4096, 65536]
    std::string Config::facebook_config_filepath_("lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json");
    uint32_t Config::fine_grained_locking_size_ = 1000;
    bool Config::is_debug_ = false;
    bool Config::is_generate_random_valuestr_ = false;
    bool Config::is_track_event_ = false;
    uint32_t Config::latency_histogram_size_ = 1000000; // Track latency up to 1000 ms
    std::string Config::output_basedir_("output");
    uint32_t Config::propagation_item_buffer_size_client_toedge_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_toclient_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_toedge_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_tocloud_ = 1000;
    uint32_t Config::propagation_item_buffer_size_cloud_toedge_ = 1000;
    std::string Config::version_("1.0");

    void Config::loadConfig(const std::string& config_filepath, const std::string& main_class_name, const bool& is_single_node)
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
                is_single_node_ = is_single_node;

                std::ostringstream oss;
                oss << "load config file from " << config_filepath << "!";
                Util::dumpNormalMsg(kClassName, oss.str());

                // parse config file to set json_object_
                parseJsonFile_(config_filepath);

                // Overwrite default values of config variables if any
                boost::json::key_value_pair* kv_ptr = NULL;
                kv_ptr = find_(CLIENT_IPSTRS_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        client_ipstrs_.push_back(std::string(iter->get_string().c_str()));
                    }
                }
                kv_ptr = find_(CLIENT_RAW_STATISTICS_SLOT_INTERVAL_SEC_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_interval = kv_ptr->value().get_int64();
                    client_raw_statistics_slot_interval_sec_ = Util::toUint32(tmp_interval);
                }
                kv_ptr = find_(CLIENT_RECVMSG_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    client_recvmsg_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(CLIENT_WORKER_RECVRSP_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    client_worker_recvrsp_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(CLOUD_IPSTR_KEYSTR);
                if (kv_ptr != NULL)
                {
                    cloud_ipstr_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(CLOUD_RECVMSG_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    cloud_recvmsg_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(CLOUD_RECVREQ_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    cloud_recvreq_startport_ = Util::toUint16(tmp_port);
                }
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
                kv_ptr = find_(EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_beacon_server_recvreq_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_BEACON_SERVER_RECVRSP_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_beacon_server_recvrsp_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    edge_cache_server_data_request_buffer_size_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_cache_server_recvreq_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_RECVRSP_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_cache_server_placement_processor_recvrsp_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_CACHE_SERVER_WORKER_RECVREQ_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_cache_server_worker_recvreq_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_CACHE_SERVER_WORKER_RECVRSP_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_cache_server_worker_recvrsp_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_INVALIDATION_SERVER_RECVREQ_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_invalidation_server_recvreq_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EDGE_IPSTRS_KEYSTR);
                if (kv_ptr != NULL)
                {
                    for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                    {
                        edge_ipstrs_.push_back(std::string(iter->get_string().c_str()));
                    }
                }
                kv_ptr = find_(EDGE_RECVMSG_STARTPORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    edge_recvmsg_startport_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(EVALUATOR_IPSTR_KEYSTR);
                if (kv_ptr != NULL)
                {
                    evaluator_ipstr_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(EVALUATOR_RECVMSG_PORT_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_port = kv_ptr->value().get_int64();
                    evaluator_recvmsg_port_ = Util::toUint16(tmp_port);
                }
                kv_ptr = find_(FACEBOOK_CONFIG_FILEPATH_KEYSTR);
                if (kv_ptr != NULL)
                {
                    facebook_config_filepath_ = std::string(kv_ptr->value().get_string().c_str());
                }
                kv_ptr = find_(FINE_GRAINED_LOCKING_SIZE_KEYSTR);
                if (kv_ptr != NULL)
                {
                    int64_t tmp_size = kv_ptr->value().get_int64();
                    fine_grained_locking_size_ = Util::toUint32(tmp_size);
                }
                kv_ptr = find_(IS_DEBUG_KEYSTR);
                {
                    int64_t tmp_value = kv_ptr->value().get_int64();
                    is_debug_ = tmp_value==1?true:false;
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
                kv_ptr = find_(OUTPUT_BASEDIR_KEYSTR);
                if (kv_ptr != NULL)
                {
                    output_basedir_ = std::string(kv_ptr->value().get_string().c_str());
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
                kv_ptr = find_(VERSION_KEYSTR);
                if (kv_ptr != NULL)
                {
                    version_ = std::string(kv_ptr->value().get_string().c_str());
                }

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
        }
        
        return;
    }

    std::string Config::getClientIpstr(const uint32_t& client_idx, const uint32_t& clientcnt)
    {
        checkIsValid_();
        if (isSingleNode()) // NOT check client_idx for single-node mode
        {
            return Util::LOCALHOST_IPSTR;
        }
        else
        {
            assert(client_idx < clientcnt);
            assert(client_ipstrs_.size() > 0);
            if (clientcnt <= client_ipstrs_.size())
            {
                return client_ipstrs_[client_idx];
            }
            else
            {
                uint32_t permachine_clientcnt = clientcnt / client_ipstrs_.size();
                assert(permachine_clientcnt > 0);
                uint32_t machine_idx = client_idx / permachine_clientcnt;
                if (machine_idx >= client_ipstrs_.size())
                {
                    machine_idx = client_ipstrs_.size() - 1; // Assign tail clients to the last machine
                }
                return client_ipstrs_[machine_idx];
            }
        }
    }

    std::string Config::getMainClassName()
    {
        checkIsValid_();
        return main_class_name_;
    }

    bool Config::isSingleNode()
    {
        checkIsValid_();

        if (!(main_class_name_ == Util::SIMULATOR_MAIN_NAME || main_class_name_ == Util::CLIENT_MAIN_NAME || main_class_name_ == Util::EDGE_MAIN_NAME || main_class_name_ == Util::CLOUD_MAIN_NAME || main_class_name_ == Util::EVALUATOR_MAIN_NAME))
        {
            std::ostringstream oss;
            oss << main_class_name_ << " should NOT use is_single_node!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return is_single_node_;
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

    uint32_t Config::getClientIpstrCnt()
    {
        checkIsValid_();
        return client_ipstrs_.size();
    }

    uint16_t Config::getClientWorkerRecvrspStartport()
    {
        checkIsValid_();
        return client_worker_recvrsp_startport_;
    }

    std::string Config::getCloudIpstr()
    {
        checkIsValid_();
        if (isSingleNode())
        {
            return Util::LOCALHOST_IPSTR;
        }
        else
        {
            return cloud_ipstr_;
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

    uint16_t Config::getEdgeInvalidationServerRecvreqStartport()
    {
        checkIsValid_();
        return edge_invalidation_server_recvreq_startport_;
    }

    std::string Config::getEdgeIpstr(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        checkIsValid_();
        if (isSingleNode()) // NOT check edge_idx for single-node mode
        {
            return Util::LOCALHOST_IPSTR;
        }
        else
        {
            assert(edge_idx < edgecnt);
            assert(edge_ipstrs_.size() > 0);
            if (edgecnt <= edge_ipstrs_.size())
            {
                return edge_ipstrs_[edge_idx];
            }
            else
            {
                uint32_t permachine_edgecnt = edgecnt / edge_ipstrs_.size();
                assert(permachine_edgecnt > 0);
                uint32_t machine_idx = edge_idx / permachine_edgecnt;
                if (machine_idx >= edge_ipstrs_.size())
                {
                    machine_idx = edge_ipstrs_.size() - 1; // Assign tail edges to the last machine
                }
                return edge_ipstrs_[machine_idx];
            }
        }
    }

    uint32_t Config::getEdgeIpstrCnt()
    {
        checkIsValid_();
        return edge_ipstrs_.size();
    }

    uint16_t Config::getEdgeRecvmsgStartport()
    {
        checkIsValid_();
        return edge_recvmsg_startport_;
    }

    std::string Config::getEvaluatorIpstr()
    {
        checkIsValid_();
        return evaluator_ipstr_;
    }

    uint16_t Config::getEvaluatorRecvmsgPort()
    {
        checkIsValid_();
        return evaluator_recvmsg_port_;
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

    bool Config::isDebug()
    {
        checkIsValid_();
        return is_debug_;
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

    std::string Config::getOutputBasedir()
    {
        checkIsValid_();
        return output_basedir_;
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

    std::string Config::getVersion()
    {
        checkIsValid_();
        return version_;
    }

    std::string Config::toString()
    {
        checkIsValid_();
        std::ostringstream oss;
        oss << "[Static configurations from " << config_filepath_ << "]" << std::endl;
        oss << "Main class name: " << main_class_name_ << std::endl;
        oss << "Is single node: " << (is_single_node_?"true":"false") << std::endl;
        oss << "Client ipstrs: ";
        for (uint32_t i = 0; i < client_ipstrs_.size(); i++)
        {
            oss << client_ipstrs_[i] << " ";
        }
        oss << std::endl;
        oss << "Client raw statistics slot interval second: " << client_raw_statistics_slot_interval_sec_ << std::endl;
        oss << "Client recvmsg startport: " << client_recvmsg_startport_ << std::endl;
        oss << "Client worker recvrsp startport: " << client_worker_recvrsp_startport_ << std::endl;
        oss << "Cloud ipstr: " << cloud_ipstr_ << std::endl;
        oss << "Cloud recvmsg startport: " << cloud_recvmsg_startport_ << std::endl;
        oss << "Cloud recvreq startport: " << cloud_recvreq_startport_ << std::endl;
        oss << "Cloud RocksDB base directory: " << cloud_rocksdb_basedir_ << std::endl;
        oss << "Covered local uncached max mem usage ratio: " << covered_local_uncached_max_mem_usage_ratio_ << std::endl; // ONLY for COVERED
        oss << "Covered popularity aggregation max mem usage ratio: " << covered_popularity_aggregation_max_mem_usage_ratio_ << std::endl; // ONLY for COVERED
        oss << "Dataset loader sleep for compaction seconds: " << dataset_loader_sleep_for_compaction_sec_ << std::endl;
        oss << "Edge beacon server recvreq startport: " << edge_beacon_server_recvreq_startport_ << std::endl;
        oss << "Edge cache server data request buffer size: " << edge_cache_server_data_request_buffer_size_ << std::endl;
        oss << "Edge cache server recvreq startport: " << edge_cache_server_recvreq_startport_ << std::endl;
        oss << "Edge cache server placement processor recvrsp startport: " << edge_cache_server_placement_processor_recvrsp_startport_ << std::endl;
        oss << "Edge cache server worker recvreq startport: " << edge_cache_server_worker_recvreq_startport_ << std::endl;
        oss << "Edge cache server worker recvrsp startport: " << edge_cache_server_worker_recvrsp_startport_ << std::endl;
        oss << "Edge invalidation server recvreq startport: " << edge_invalidation_server_recvreq_startport_ << std::endl;
        oss << "Edge ipstrs: ";
        for (uint32_t i = 0; i < edge_ipstrs_.size(); i++)
        {
            oss << edge_ipstrs_[i] << " ";
        }
        oss << std::endl;
        oss << "Edge recvmsg startport: " << edge_recvmsg_startport_ << std::endl;
        oss << "Evaluator ipstr:" << evaluator_ipstr_ << std::endl;
        oss << "Evaluator recvmsg port: " << evaluator_recvmsg_port_ << std::endl;
        oss << "Facebook config filepath: " << facebook_config_filepath_ << std::endl;
        oss << "Fine-grained locking size: " << fine_grained_locking_size_ << std::endl;
        oss << "Is debug: " << (is_debug_?"true":"false") << std::endl;
        oss << "Is generate random valuestr: " << (is_generate_random_valuestr_?"true":"false") << std::endl;
        oss << "Is track event: " << (is_track_event_?"true":"false") << std::endl;
        oss << "Latency histogram size: " << latency_histogram_size_ << std::endl;
        oss << "Output base directory: " << output_basedir_ << std::endl;
        oss << "Propagation item buffer size from client to edge: " << propagation_item_buffer_size_client_toedge_ << std::endl;
        oss << "Propagation item buffer size from edge to client: " << propagation_item_buffer_size_edge_toclient_ << std::endl;
        oss << "Propagation item buffer size from edge to edge: " << propagation_item_buffer_size_edge_toedge_ << std::endl;
        oss << "Propagation item buffer size from edge to cloud: " << propagation_item_buffer_size_edge_tocloud_ << std::endl;
        oss << "Propagation item buffer size from cloud to edge: " << propagation_item_buffer_size_cloud_toedge_ << std::endl;
        oss << "Version: " << version_;
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
        if (main_class_name_ != Util::SIMULATOR_MAIN_NAME && main_class_name_ != Util::TOTAL_STATISTICS_LOADER_MAIN_NAME && main_class_name_ != Util::DATASET_LOADER_MAIN_NAME && main_class_name_ != Util::CLIENT_MAIN_NAME && main_class_name_ != Util::EDGE_MAIN_NAME && main_class_name_ != Util::CLOUD_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "main class name " << main_class_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
}
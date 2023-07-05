#include "common/config.h"

#include <assert.h> // assert
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string CLIENT_IPSTRS_KEYSTR("client_ipstrs");
    const std::string CLIENT_RECVRSP_STARTPORT_KEYSTR("client_recvrsp_startport");
    const std::string Config::CLOUD_IPSTR_KEYSTR("cloud_ipstr");
    const std::string Config::CLOUD_RECVREQ_STARTPORT_KEYSTR("cloud_recvreq_startport");
    const std::string Config::CLOUD_ROCKSDB_BASEDIR_KEYSTR("cloud_rocksdb_basedir");
    const std::string Config::EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_beacon_server_recvreq_startport");
    const std::string Config::EDGE_CACHE_SERVER_DATA_REQUEST_BUFFER_SIZE_KEYSTR("edge_cache_server_data_request_buffer_size");
    const std::string Config::EDGE_CACHE_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_cache_server_recvreq_startport");
    const std::string Config::EDGE_INVALIDATION_SERVER_RECVREQ_STARTPORT_KEYSTR("edge_invalidation_server_recvreq_startport");
    const std::string Config::EDGE_IPSTRS_KEYSTR("edge_ipstrs");
    const std::string Config::FACEBOOK_CONFIG_FILEPATH_KEYSTR("facebook_config_filepath");
    const std::string Config::FINE_GRAINED_LOCKING_SIZE_KEYSTR("fine_grained_locking_size");
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

    std::vector<std::string> Config::client_ipstrs_(0);
    uint16_t Config::client_recvrsp_startport_ = 4100; // [4096, 65536]
    std::string Config::cloud_ipstr_ = Util::LOCALHOST_IPSTR;
    uint16_t Config::cloud_recvreq_startport_ = 4200; // [4096, 65536]
    std::string Config::cloud_rocksdb_basedir_("/tmp/cloud");
    uint16_t Config::edge_beacon_server_recvreq_startport_ = 4300; // [4096, 65536]
    uint32_t Config::edge_cache_server_data_request_buffer_size_ = 1000;
    uint16_t Config::edge_cache_server_recvreq_startport_ = 4400; // [4096, 65536]
    uint16_t Config::edge_invalidation_server_recvreq_startport_ = 4500; // [4096, 65536]
    std::vector<std::string> Config::edge_ipstrs_(0);
    std::string Config::facebook_config_filepath_("lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json");
    uint32_t Config::fine_grained_locking_size_ = 1000;
    uint32_t Config::latency_histogram_size_ = 1000000; // Track latency up to 1000 ms
    std::string Config::output_basedir_("output");
    uint32_t Config::propagation_item_buffer_size_client_toedge_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_toclient_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_toedge_ = 1000;
    uint32_t Config::propagation_item_buffer_size_edge_tocloud_ = 1000;
    uint32_t Config::propagation_item_buffer_size_cloud_toedge_ = 1000;
    std::string Config::version_("1.0");

    void Config::loadConfig()
    {
        std::string config_filepath = Param::getConfigFilepath();
        bool is_exist = Util::isFileExist(config_filepath, true);

        if (is_exist)
        {
            // parse config file to set json_object_
            parseJsonFile_(config_filepath);

            // Overwrite default values of config variables if any
            boost::json::key_value_pair* kv_ptr = NULL;
            kv_ptr = find_(CLIENT_IPSTRS_KEYSTR);
            if (kv_ptr != NULL)
            {
                for (boost::json::array::iterator iter = kv_ptr->value().get_array().begin(); iter != kv_ptr->value().get_array().end(); iter++)
                {
                    client_ipstrs_.push_back(static_cast<std::string>(iter->get_string()));
                }
            }
            kv_ptr = find_(CLIENT_RECVRSP_STARTPORT_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_port = kv_ptr->value().get_int64();
                client_recvrsp_startport_ = Util::toUint16(tmp_port);
            }
            kv_ptr = find_(CLOUD_IPSTR_KEYSTR);
            if (kv_ptr != NULL)
            {
                cloud_ipstr_ = kv_ptr->value().get_string();
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
                cloud_rocksdb_basedir_ = kv_ptr->value().get_string();
            }
            kv_ptr = find_(EDGE_BEACON_SERVER_RECVREQ_STARTPORT_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_port = kv_ptr->value().get_int64();
                edge_beacon_server_recvreq_startport_ = Util::toUint16(tmp_port);
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
                    edge_ipstrs_.push_back(static_cast<std::string>(iter->get_string()));
                }
            }
            kv_ptr = find_(FACEBOOK_CONFIG_FILEPATH_KEYSTR);
            if (kv_ptr != NULL)
            {
                facebook_config_filepath_ = kv_ptr->value().get_string();
            }
            kv_ptr = find_(FINE_GRAINED_LOCKING_SIZE_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_size = kv_ptr->value().get_int64();
                fine_grained_locking_size_ = Util::toUint32(tmp_size);
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
                output_basedir_ = kv_ptr->value().get_string();
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
                version_ = kv_ptr->value().get_string();
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
        return;
    }

    std::string Config::getClientIpstr(const uint32_t& client_idx, const uint32_t& clientcnt)
    {
        checkIsValid_();
        if (Param::isSingleNode()) // NOT check client_idx for single-node mode
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

    uint32_t Config::getClientIpstrCnt()
    {
        checkIsValid_();
        return client_ipstrs_.size();
    }

    uint16_t Config::getClientRecvrspStartport()
    {
        checkIsValid_();
        return client_recvrsp_startport_;
    }

    std::string Config::getCloudIpstr()
    {
        checkIsValid_();
        if (Param::isSingleNode())
        {
            return Util::LOCALHOST_IPSTR;
        }
        else
        {
            return cloud_ipstr_;
        }
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

    uint16_t Config::getEdgeBeaconServerRecvreqStartport()
    {
        checkIsValid_();
        return edge_beacon_server_recvreq_startport_;
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

    uint16_t Config::getEdgeInvalidationServerRecvreqStartport()
    {
        checkIsValid_();
        return edge_invalidation_server_recvreq_startport_;
    }

    std::string Config::getEdgeIpstr(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        checkIsValid_();
        if (Param::isSingleNode()) // NOT check edge_idx for single-node mode
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
        oss << "[Static configurations from " << Param::getConfigFilepath() << "]" << std::endl;
        oss << "Client ipstrs: ";
        for (uint32_t i = 0; i < client_ipstrs_.size(); i++)
        {
            oss << client_ipstrs_[i] << " ";
        }
        oss << std::endl;
        oss << "Client recvrsp startport: " << client_recvrsp_startport_ << std::endl;
        oss << "Cloud ipstr: " << cloud_ipstr_ << std::endl;
        oss << "Cloud recvreq startport: " << cloud_recvreq_startport_ << std::endl;
        oss << "Cloud RocksDB base directory: " << cloud_rocksdb_basedir_ << std::endl;
        oss << "Edge beacon server recvreq startport: " << edge_beacon_server_recvreq_startport_ << std::endl;
        oss << "Edge cache server data request buffer size: " << edge_cache_server_data_request_buffer_size_ << std::endl;
        oss << "Edge cache server recvreq startport: " << edge_cache_server_recvreq_startport_ << std::endl;
        oss << "Edge invalidation server recvreq startport: " << edge_invalidation_server_recvreq_startport_ << std::endl;
        oss << "Edge ipstrs: ";
        for (uint32_t i = 0; i < edge_ipstrs_.size(); i++)
        {
            oss << edge_ipstrs_[i] << " ";
        }
        oss << std::endl;
        oss << "Facebook config filepath: " << facebook_config_filepath_ << std::endl;
        oss << "Fine-grained locking size: " << fine_grained_locking_size_ << std::endl;
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
        Util::dumpDebugMsg(kClassName, oss.str());

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
}
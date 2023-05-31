#include "common/config.h"

#include <assert.h> // assert
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string Config::FACEBOOK_CONFIG_FILEPATH_KEYSTR("facebook_config_filepath");
    const std::string Config::GLOBAL_CLOUD_RECVREQ_PORT_KEYSTR("global_cloud_recvreq_port");
    const std::string Config::GLOBAL_CLOUD_ROCKSDB_PATH_KEYSTR("global_cloud_rocksdb_path");
    const std::string Config::GLOBAL_EDGE_RECVREQ_STARTPORT_KEYSTR("global_edge_recvreq_startport");
    const std::string Config::LATENCY_HISTOGRAM_SIZE_KEYSTR("latency_histogram_size");
    const std::string Config::OUTPUT_BASEDIR_KEYSTR("output_basedir");
    const std::string Config::VERSION_KEYSTR("version");

    const std::string Config::kClassName("Config");

    // Initialize config variables by default
    bool Config::is_valid_ = false;
    boost::json::object Config::json_object_ = boost::json::object();
    std::string Config::facebook_config_filepath_("lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json");
    uint16_t Config::global_cloud_recvreq_port_ = 4100; // [4096, 65536]
    std::string Config::global_cloud_rocksdb_path_("/tmp/cloud");
    uint16_t Config::global_edge_recvreq_startport_ = 4200; // [4096, 65536]
    uint32_t Config::latency_histogram_size_ = 1000000; // Track latency up to 1000 ms
    std::string Config::output_basedir_("output");
    std::string Config::version_("1.0");

    void Config::loadConfig()
    {
        std::string config_filepath = Param::getConfigFilepath();
        bool is_exist = Util::isFileExist(config_filepath);

        if (is_exist)
        {
            // parse config file to set json_object_
            parseJsonFile_(config_filepath);

            // Overwrite default values of config variables if any
            boost::json::key_value_pair* kv_ptr = NULL;
            kv_ptr = find_(FACEBOOK_CONFIG_FILEPATH_KEYSTR);
            if (kv_ptr != NULL)
            {
                facebook_config_filepath_ = kv_ptr->value().get_string();
            }
            kv_ptr = find_(GLOBAL_CLOUD_RECVREQ_PORT_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_port = kv_ptr->value().get_int64();
                global_cloud_recvreq_port_ = Util::toUint16(tmp_port);
            }
            kv_ptr = find_(GLOBAL_CLOUD_ROCKSDB_PATH_KEYSTR);
            if (kv_ptr != NULL)
            {
                global_cloud_rocksdb_path_ = kv_ptr->value().get_string();
            }
            kv_ptr = find_(GLOBAL_EDGE_RECVREQ_STARTPORT_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_port = kv_ptr->value().get_int64();
                global_edge_recvreq_startport_ = Util::toUint16(tmp_port);
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

    std::string Config::getFacebookConfigFilepath()
    {
        checkIsValid_();
        return facebook_config_filepath_;
    }

    uint16_t Config::getGlobalCloudRecvreqPort()
    {
        checkIsValid_();
        return global_cloud_recvreq_port_;
    }

    std::string Config::getGlobalCloudRocksdbPath()
    {
        checkIsValid_();
        return global_cloud_rocksdb_path_;
    }

    uint16_t Config::getGlobalEdgeRecvreqStartport()
    {
        checkIsValid_();
        return global_edge_recvreq_startport_;
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
        oss << "Facebook config filepath: " << facebook_config_filepath_ << std::endl;
        oss << "Global cloud recvreq port: " << global_cloud_recvreq_port_ << std::endl;
        oss << "Global edge recvreq startport: " << global_edge_recvreq_startport_ << std::endl;
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
        if (Param::isDebug())
        {
            std::ostringstream oss;
            oss << "read " << config_filepath << " with " << filesize << " bytes";
            Util::dumpDebugMsg(kClassName, oss.str());
        }

        // Parse the bytes
        boost::json::stream_parser boost_json_parser;
        boost::json::error_code boost_json_errcode;
        boost_json_parser.write(bytes.data(), filesize, boost_json_errcode);
        bool is_error = bool(boost_json_errcode);
        if (is_error)
        {
            Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
            exit(-1);
        }

        // Finish byte parsing
        boost_json_parser.finish(boost_json_errcode);
        is_error = bool(boost_json_errcode);
        if (is_error)
        {
            Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
            exit(-1);
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
            exit(-1);
        }
        return;
    }
}
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>
#include <assert.h> // assert

#include "common/util.h"
#include "common/param.h"
#include "common/config.h"

#include <iostream>

namespace covered
{
    const std::string Config::VERSION_KEYSTR("version");
    const std::string Config::GLOBAL_CLIENT_WORKLOAD_STARTPORT_KEYSTR("global_client_workload_startport");
    const std::string Config::FACEBOOK_CONFIG_FILEPATH_KEYSTR("facebook_config_filepath");

    const std::string Config::kClassName("Config");

    // Initialize config variables by default
    bool Config::is_valid_ = false;
    boost::json::object Config::json_object_ = boost::json::object();
    std::string Config::version_("1.0");
    uint16_t Config::global_client_workload_startport_ = 4100; // [4096, 65536]
    std::string Config::facebook_config_filepath_("lib/CacheLib/cachelib/cachebench/test_configs/hit_ratio/cdn/config.json");

    void Config::loadConfig()
    {
        std::string config_filepath = Param::getConfigFilepath();
        bool is_exist = Util::isFileExist(config_filepath);

        if (is_exist)
        {
            // parse config file to set json_object_
            parseJsonFile(config_filepath);

            // Overwrite default values of config variables if any
            boost::json::key_value_pair* kv_ptr = find(VERSION_KEYSTR);
            if (kv_ptr != NULL)
            {
                version_ = kv_ptr->value().get_string();
            }
            kv_ptr = find(GLOBAL_CLIENT_WORKLOAD_STARTPORT_KEYSTR);
            if (kv_ptr != NULL)
            {
                int64_t tmp_port = kv_ptr->value().get_int64();
                global_client_workload_startport_ = Util::toUint16(tmp_port);
            }
            kv_ptr = find(FACEBOOK_CONFIG_FILEPATH_KEYSTR);
            if (kv_ptr != NULL)
            {
                facebook_config_filepath_ = kv_ptr->value().get_string();
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

    std::string Config::getVersion()
    {
        checkIsValid();
        return version_;
    }

    uint16_t Config::getGlobalClientWorkloadStartport()
    {
        checkIsValid();
        return global_client_workload_startport_;
    }

    std::string Config::getFacebookConfigFilepath()
    {
        checkIsValid();
        return facebook_config_filepath_;
    }

    std::string Config::toString()
    {
        checkIsValid();
        std::ostringstream oss;
        oss << "[Static configurations from " << Param::getConfigFilepath() << "]" << std::endl;
        oss << "Version: " << version_;
        return oss.str();
    }

    void Config::parseJsonFile(const std::string& config_filepath)
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

    boost::json::key_value_pair* Config::find(const std::string& key)
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

    void Config::checkIsValid()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid Config (config file has not been loaded)!");
            exit(-1);
        }
        return;
    }
}
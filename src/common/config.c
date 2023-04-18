#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>
#include <assert.h> // assert

#include "common/util.h"
#include "common/param.h"
#include "common/config.h"

const std::string covered::Config::VERSION_KEYSTR = "version";
const std::string covered::Config::GLOBAL_CLIENT_WORKLOAD_STARTPORT_KEYSTR = "global_client_workload_startport"

const std::string covered::Config::kClassName = "Config";

// Initialize config variables by default
bool covered::Config::is_valid_ = false;
static std::string covered::Config::filepath_ = "";
static std::string covered::Config::version_ = "1.0";
static uint16_t covered::Config::client_workload_startport_ = 4100; // [4096, 65536]

void covered::Config::setConfig(const std::string& config_filepath)
{
    filepath_ = config_filepath;
    bool is_exist = covered::Util::isFileExist(config_filepath);

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
            uint64_t tmp_port = kv_ptr->value().get_uint64();
            global_client_workload_startport_ = covered::Util::toUint16(tmp_port);
        }

        is_valid_ = true; // valid Config
    }
    else
    {
        std::ostringstream oss;

        //oss << config_filepath << " does not exist; use default config!";
        //covered::Util::dumpWarnMsg(kClassName, oss.str());
        ////is_valid_ = false; // invalid Config

        oss << config_filepath << " does not exist!";
        covered::Util::dumpErrorMsg(kClassName, oss.str());
        exit(1);
    }
}

std::string covered::Config::getVersion()
{
    checkIsValid();
    return version_;
}

uint16_t covered::Config::getGobalClientWorkloadStartport()
{
    checkIsValid();
    return global_client_workload_startport_;
}

std::string covered::Config::toString()
{
    checkIsValid();
    std::ostringstream oss;
    oss << "[Parsed Config Information]" << std::endl;
    oss << "Config file path: " << filepath_ << std::endl;
    oss << "Version: " << version_;
    return oss.str();
}

void covered::Config::parseJsonFile(const std::string& config_filepath)
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
    if (covered::Param::isDebug())
    {
        std::ostringstream oss;
        oss << "read " << config_filepath << " with " << filesize << " bytes";
        covered::Util::dumpDebugMsg(kClassName, oss.str());
    }

    // Parse the bytes
    boost::json::stream_parser boost_json_parser;
    boost::json::error_code boost_json_errcode;
    boost_json_parser.write(bytes.data(), filesize, boost_json_errcode);
    bool is_error = bool(boost_json_errcode);
    if (is_error)
    {
        covered::Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
        exit(-1);
    }

    // Finish byte parsing
    boost_json_parser.finish(boost_json_errcode);
    is_error = bool(boost_json_errcode);
    if (is_error)
    {
        covered::Util::dumpErrorMsg(kClassName, boost_json_errcode.message());
        exit(-1);
    }

    // Get the json object
    boost::json::value boost_json_value = boost_json_parser.release();
    assert(boost_json_value.kind() == boost::json::kind::object);
    json_object_ = boost_json_value.get_object();
    return;
}

boost::json::key_value_pair* covered::Config::find(const std::string& key)
{
    boost::json::key_value_pair* kv_ptr = json_object_.find(key);
    if (kv_ptr == NULL)
    {
        std::ostringstrem oss;
        oss << "no json entry for " << key << "; use default setting"
        covered::Util::dumpWarnMsg(kClassName, oss.str());
    }
    return kv_ptr;
}

void covered::Config::checkIsValid()
{
    if (!is_valid_)
    {
        covered::Util::dumpErrorMsg(kClassName, "invalid Config (config file has not been loaded)!");
        exit(-1);
    }
    return;
}
#include <fstream> // ifstream
#include <sstream> // ostringstream
#include <vector>
#include <assert.h> // assert

#include "util.h"
#include "param.h"
#include "config.h"

const std::string covered::Config::VERSION_KEYSTR = "version";

const std::string covered::Config::class_name_ = "Config";

covered::Config::Config(const std::string& config_filepath)
{
    filepath_ = config_filepath;
    bool is_exist = covered::Util::isFileExist(config_filepath);

    // Initialize config variables by default
    version_ = "0.0";

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

        //is_valid_ = true; // valid Config
    }
    else
    {
        std::ostringstream oss;
        oss << config_filepath << " does not exist; use default config!";
        covered::Util::dumpWarnMsg(class_name_, oss.str());
        //is_valid_ = false; // invalid Config
    }
}

covered::Config::~Config() {}

std::string covered::Config::getVersion()
{
    //checkIsValid();
    return version_;
}

std::string covered::Config::toString()
{
    //checkIsValid();
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
        covered::Util::dumpDebugMsg(class_name_, oss.str());
    }

    // Parse the bytes
    boost::json::stream_parser boost_json_parser;
    boost::json::error_code boost_json_errcode;
    boost_json_parser.write( bytes.data(), filesize, boost_json_errcode);
    bool is_error = bool(boost_json_errcode);
    if (is_error)
    {
        covered::Util::dumpErrorMsg(class_name_, boost_json_errcode.message());
        exit(-1);
    }

    // Finish byte parsing
    boost_json_parser.finish(boost_json_errcode);
    is_error = bool(boost_json_errcode);
    if (is_error)
    {
        covered::Util::dumpErrorMsg(class_name_, boost_json_errcode.message());
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
    return kv_ptr;
}

/*void covered::Config::checkIsValid()
{
    if (!is_valid_)
    {
        covered::Util::dumpErrorMsg(class_name_, "invalid Config (config file has not been loaded)!");
        exit(-1);
    }
    return;
}*/
#include "common/cli.h"

#include <boost/program_options.hpp>

namespace covered
{
    void CLI::parseAndProcessCliParameters(const bool& is_simulation, const std::string& main_class_name)
    {
        parseCliParameters_(is_simulation);
        processCliParameters_(main_class_name);
        return;
    }

    void CLI::parseCliParameters_(const bool& is_simulation)
    {
        // (1) Create CLI parameter description

        boost::program_options::options_description argument_desc("Allowed arguments:");
        // Static actions
        argument_desc.add_options()
            ("help,h", "dump help information")
        ;
        // Dynamic configurations
        argument_desc.add_options()
            ("cache_name", boost::program_options::value<std::string>()->default_value(covered::CacheWrapperBase::LRU_CACHE_NAME), "cache name")
            ("capacitymb", boost::program_options::value<uint32_t>()->default_value(1000), "cache capacity in units of MB")
            ("clientcnt", boost::program_options::value<uint32_t>()->default_value(1), "the total number of clients")
            ("config_file", boost::program_options::value<std::string>()->default_value("config.json"), "config file path of COVERED")
            ("cloud_storage", boost::program_options::value<std::string>()->default_value(covered::RocksdbWrapper::HDD_NAME), "type of cloud storage")
            ("debug", "enable debug information")
            ("duration", boost::program_options::value<double>()->default_value(10), "benchmark duration")
            ("edgecnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of edge nodes")
            ("keycnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
            ("opcnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
            ("perclient_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each client")
            ("workload_name", boost::program_options::value<std::string>()->default_value(covered::WorkloadWrapperBase::FACEBOOK_WORKLOAD_NAME), "workload name")
        ;
        // Dynamic actions
        argument_desc.add_options()
            ("version,v", "dump version of COVERED")
        ;

        // (2) Parse CLI parameters (static/dynamic actions and dynamic configurations)

        boost::program_options::variables_map argument_info;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc), argument_info);
        boost::program_options::notify(argument_info);

        // (2.1) Get CLI paremters for dynamic configurations

        std::string cache_name = argument_info["cache_name"].as<std::string>();
        uint32_t capacity = argument_info["capacitymb"].as<uint32_t> * 1000; // In units of bytes
        uint32_t clientcnt = argument_info["clientcnt"].as<uint32_t>();
        std::string cloud_storage = argument_info["cloud_storage"].as<std::string>();
        std::string config_filepath = argument_info["config_file"].as<std::string>();
        bool is_debug = false;
        if (argument_info.count("debug"))
        {
            is_debug = true;
        }
        double duration = argument_info["duration"].as<double>();
        uint32_t edgecnt = argument_info["edgecnt"].as<uint32_t>();
        uint32_t keycnt = argument_info["keycnt"].as<uint32_t>();
        uint32_t opcnt = argument_info["opcnt"].as<uint32_t>();
        uint32_t perclient_workercnt = argument_info["perclient_workercnt"].as<uint32_t>();
        std::string workload_name = argument_info["workload_name"].as<std::string>();

        // Store CLI parameters for dynamic configurations and mark covered::Param as valid
        covered::Param::setParameters(is_simulation, cache_name, capacity, clientcnt, cloud_storage, config_filepath, is_debug, duration, edgecnt, keycnt, opcnt, perclient_workercnt, workload_name);

        // (2.2) Load config file for static configurations

        covered::Config::loadConfig();

        return;
    }

    void CLI::processCliParameters_(const std::string& main_class_name)
    {
        // (3) Process CLI parameters

        // (3.1) Process static actions

        if (argument_info.count("help")) // Dump help information
        {
            std::ostringstream oss;
            oss << argument_desc;
            covered::Util::dumpNormalMsg(main_class_name, oss.str());
            return 1;
        }

        // (3.2) Process dynamic actions

        if (argument_info.count("version"))
        {
            std::ostringstream oss;
            oss << "Current version of COVERED: " << covered::Config::getVersion();
            covered::Util::dumpNormalMsg(main_class_name, oss.str());
            return 1;
        }

        // (4) Dump stored CLI parameters and parsed config information if debug

        if (is_debug)
        {
            covered::Util::dumpDebugMsg(main_class_name, covered::Param::toString());
            covered::Util::dumpDebugMsg(main_class_name, covered::Config::toString());
        }

        return;
    }
}
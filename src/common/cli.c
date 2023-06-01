#include "common/cli.h"

#include "cache/cache_wrapper_base.h"
#include "cloud/rocksdb_wrapper.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    std::string CLI::SIMULATOR_NAME("simulator");
    std::string CLI::STATISTICS_AGGREGATOR_NAME("statistics_aggregator");

    std::string CLI::kClassName("CLI");

    boost::program_options::options_description CLI::argument_desc_("Allowed arguments:");
    boost::program_options::variables_map CLI::argument_info_; // Default constructor

    void CLI::parseAndProcessCliParameters(const std::string& main_class_name, int argc, char **argv)
    {
        parseCliParameters_(main_class_name, argc, argv); // Parse CLI parameters based on argument_desc_ to set argument_info_
        setParamAndConfig_(main_class_name); // Set parameters for dynamic configurations and load config file for static configurations
        processCliParameters_(main_class_name); // Process static/dynamic actions
        return;
    }

    void CLI::parseCliParameters_(const std::string& main_class_name, int argc, char **argv)
    {
        // (1) Create CLI parameter description

        // Static actions
        argument_desc_.add_options()
            ("help,h", "dump help information")
        ;
        // Dynamic configurations
        argument_desc_.add_options()
            ("cache_name", boost::program_options::value<std::string>()->default_value(CacheWrapperBase::LRU_CACHE_NAME), "cache name")
            ("capacitymb", boost::program_options::value<uint32_t>()->default_value(1000), "cache capacity in units of MB")
            ("clientcnt", boost::program_options::value<uint32_t>()->default_value(1), "the total number of clients")
            ("config_file", boost::program_options::value<std::string>()->default_value("config.json"), "config file path of COVERED")
            ("cloud_storage", boost::program_options::value<std::string>()->default_value(RocksdbWrapper::HDD_NAME), "type of cloud storage")
            ("debug", "enable debug information")
            ("duration", boost::program_options::value<double>()->default_value(10), "benchmark duration")
            ("edgecnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of edge nodes")
            ("keycnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
            ("opcnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
            ("perclient_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each client")
            ("propagation_latency_clientedge", boost::program_options::value<uint32_t>()->default_value(1000), "the propagation latency between client and edge (in units of us)")
            ("propagation_latency_crossedge", boost::program_options::value<uint32_t>()->default_value(10000), "the propagation latency between edge and neighbor (in units of us)")
            ("propagation_latency_edgecloud", boost::program_options::value<uint32_t>()->default_value(100000), "the propagation latency between edge and cloud (in units of us)")
            ("prototype", "disable simulation mode")
            ("workload_name", boost::program_options::value<std::string>()->default_value(WorkloadWrapperBase::FACEBOOK_WORKLOAD_NAME), "workload name")
        ;
        // Dynamic actions
        argument_desc_.add_options()
            ("version,v", "dump version of COVERED")
        ;

        // (2) Parse CLI parameters (static/dynamic actions and dynamic configurations)

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc_), argument_info_);
        boost::program_options::notify(argument_info_);

        return;
    }

    void CLI::setParamAndConfig_(const std::string& main_class_name)
    {
        // (3) Get CLI parameters for dynamic configurations

        bool is_simulation = true; // Enable simulation mode by default
        if (argument_info_.count("prototype"))
        {
            if (main_class_name == CLI::SIMULATOR_NAME)
            {
                std::ostringstream oss;
                oss << "--prototype does not work for " << main_class_name << " -> still enable simulation mode!";
                Util::dumpWarnMsg(kClassName, oss.str());
            }
            else
            {
                is_simulation = false;
            }
        }
        std::string cache_name = argument_info_["cache_name"].as<std::string>();
        uint32_t capacity = argument_info_["capacitymb"].as<uint32_t>() * 1000; // In units of bytes
        uint32_t clientcnt = argument_info_["clientcnt"].as<uint32_t>();
        std::string cloud_storage = argument_info_["cloud_storage"].as<std::string>();
        std::string config_filepath = argument_info_["config_file"].as<std::string>();
        bool is_debug = false;
        if (argument_info_.count("debug"))
        {
            is_debug = true;
        }
        double duration = argument_info_["duration"].as<double>();
        uint32_t edgecnt = argument_info_["edgecnt"].as<uint32_t>();
        uint32_t keycnt = argument_info_["keycnt"].as<uint32_t>();
        uint32_t opcnt = argument_info_["opcnt"].as<uint32_t>();
        uint32_t perclient_workercnt = argument_info_["perclient_workercnt"].as<uint32_t>();
        uint32_t propagation_latency_clientedge = argument_info_["propagation_latency_clientedge"].as<uint32_t>();
        uint32_t propagation_latency_crossedge = argument_info_["propagation_latency_crossedge"].as<uint32_t>();
        uint32_t propagation_latency_edgecloud = argument_info_["propagation_latency_edgecloud"].as<uint32_t>();
        std::string workload_name = argument_info_["workload_name"].as<std::string>();

        // Store CLI parameters for dynamic configurations and mark Param as valid
        Param::setParameters(is_simulation, cache_name, capacity, clientcnt, cloud_storage, config_filepath, is_debug, duration, edgecnt, keycnt, opcnt, perclient_workercnt, propagation_latency_clientedge, propagation_latency_crossedge, propagation_latency_edgecloud, workload_name);

        // (4) Load config file for static configurations

        Config::loadConfig();
    }

    void CLI::processCliParameters_(const std::string& main_class_name)
    {
        // (5) Process CLI parameters

        // (5.1) Process static actions

        if (argument_info_.count("help")) // Dump help information
        {
            std::ostringstream oss;
            oss << argument_desc_;
            Util::dumpNormalMsg(main_class_name, oss.str());
            exit(0);
        }

        // (5.2) Process dynamic actions

        if (argument_info_.count("version"))
        {
            std::ostringstream oss;
            oss << "Current version of COVERED: " << Config::getVersion();
            Util::dumpNormalMsg(main_class_name, oss.str());
            exit(0);
        }

        // (6) Dump stored CLI parameters and parsed config information if debug

        Util::dumpDebugMsg(main_class_name, Param::toString());
        Util::dumpDebugMsg(main_class_name, Config::toString());

        return;
    }
}
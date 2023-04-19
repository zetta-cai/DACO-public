#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include "common/util.h"
#include "common/param.h"
#include "common/config.h"

int main(int argc, char **argv) {
    std::string main_class_name = "simulator";

    // (1) Create CLI parameter description

    boost::program_options::options_description argument_desc("Allowed arguments:");
    // Static actions
    argument_desc.add_options()
        ("help,h", "dump help information")
    ;
    // Dynamic configurations
    argument_desc.add_options()
        ("config_file,c", boost::program_options::value<std::string>()->default_value("config.json"), "specify config file path of COVERED")
        ("debug", "enable debug information")
        ("keycnt,k", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
        ("opcnt,o", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
        ("clientcnt,p", boost::program_options::value<uint32_t>()->default_value(2), "the total number of physical clients")
        ("perclient_workercnt,p", boost::program_options::value<uint32_t>()->default_value(10), "the number of worker threads for each physical client")
    ;
    // Dynamic actions
    argument_desc.add_options()
        ("version,v", "dump version of COVERED")
    ;

    // (2) Parse CLI parameters (static/dynamic actions and dynamic configurations)

    boost::program_options::variables_map argument_info;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc), argument_info);
    boost::program_options::notify(argument_info);

    // (2.1) Process static actions

    if (argument_info.count("help")) // Dump help information
    {
        std::ostringstream oss;
        oss << argument_desc;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());
        return 1;
    }

    // (2.2) Get CLI paremters for dynamic configurations

    bool is_debug = false;
    if (argument_info.count("debug"))
    {
        is_debug = true;
    }
    std::string config_filepath = argument_info["config_file"].as<std::string>();
    uint32_t keycnt = argument_info["keycnt"].as<uint32_t>();
    uint32_t opcnt = argument_info["opcnt"].as<uint32_t>();
    uint32_t clientcnt = argument_info["clientcnt"].as<uint32_t>();
    uint32_t perclient_workercnt = argument_info["perclient_workercnt"].as<uint32_t>();

    // Store CLI parameters for dynamic configurations and mark covered::Param as valid
    covered::Param::setParameters(config_filepath, is_debug, keycnt, opcnt, clientcnt, perclient_workercnt);

    // (2.3) Load config file for static configurations

    covered::Config::loadConfig();

    // (2.4) Process dynamic actions

    if (argument_info.count("version"))
    {
        std::ostringstream oss;
        oss << "Current version of COVERED: " << covered::Config::getVersion();
        covered::Util::dumpNormalMsg(main_class_name, oss.str());
        return 1;
    }

    // (3) Dump stored CLI parameters and parsed config information if debug
    if (covered::Param::isDebug())
    {
        covered::Util::dumpDebugMsg(main_class_name, covered::Param::toString());
        covered::Util::dumpDebugMsg(main_class_name, covered::Config::toString());
    }
    
    // (4) TODO: simulate multiple physical clients by multi-threading

    return 0;
}
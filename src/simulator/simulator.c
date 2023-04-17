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
    argument_desc.add_options()
        ("help,h", "dump help information")
        ("debug", "enable debug information")
        ("config_file,c", boost::program_options::value<std::string>()->default_value("../../config.json"), "specify config file path")
        ("version,v", "dump version of COVERED")
        ("clientcnt", boost::program_options::value<int>()->default_value(10), "the number of clients")
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
    // (2.2) Store CLI paremters for dynamic configurations
    else if (argument_info.count("debug"))
    {
        covered::Param::setDebug(true);
    }
    covered::Param::setValid(true); // After storing all CLI parameters

    // (2.3) Load config file for static configurations
    covered::Config config(argument_info["config_file"].as<std::string>());

    // (2.4) Process dynamic actions
    if (argument_info.count("version"))
    {
        std::ostringstream oss;
        oss << "Current version of COVERED: " << config.getVersion();
        covered::Util::dumpNormalMsg(main_class_name, oss.str());
        return 1;
    }

    // (3) Dump stored CLI parameters and parsed config information if debug
    if (covered::Param::isDebug())
    {
        covered::Util::dumpDebugMsg(main_class_name, covered::Param::toString());
        covered::Util::dumpDebugMsg(main_class_name, config.toString());
    }
    
    // (4) TODO: simulate multiple physical clients by multi-threading

    return 0;
}
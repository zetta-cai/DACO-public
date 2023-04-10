#include <iostream>

#include <boost/program_options.hpp>

#include "param.h"
#include "config.h"

int main(int argc, char **argv) {
    // Create CLI parameter description
    boost::program_options::options_description argument_desc("Allowed arguments:");
    argument_desc.add_options()
        ("help,h", "dump help information")
        ("debug", "enable debug information")
        ("version,v", "dump version of COVERED")
        ("clientcnt", boost::program_options::value<int>()->default_value(10), "the number of clients")
    ;

    // Parse CLI parameters
    boost::program_options::variables_map argument_info;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc), argument_info);
    boost::program_options::notify(argument_info);

    // Process static parameters
    if (argument_info.count("help")) // Dump help information
    {
        std::cout << argument_desc << std::endl;
        return 1;
    }
    // Store CLI paremters
    else if (argument_info.count("debug"))
    {
        covered::Param::setDebug(true);
    }
    covered::Param::setValid(true); // After storing all CLI parameters

    // Load dynamic config information
    std::string config_filepath = "../../config.json";
    covered::Config config(config_filepath);

    // Process dynamic parameters
    if (argument_info.count("version"))
    {
        std::cout << "Current version of COVERED: " << config.getVersion() << std::endl;
        return 1;
    }

    // Dump stored CLI parameters and parsed config information if debug
    if (covered::Param::isDebug())
    {
        std::cout << covered::Param::toString() << std::endl;
        std::cout << config.toString() << std::endl;
    }
    
    return 1;
}
/*
 * CLI: parse and process CLI parameters for static/dynamic actions and static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.05.26).
 */

#ifndef CLI_H
#define CLI_H

#include <string>

#include <boost/program_options.hpp>

namespace covered
{
    enum CliRole
    {
        kSimulator = 1,
        kStatisticsAggregator,
        kClientPrototype,
        kEdgePrototype,
        kCloudPrototype
    };

    class CLI
    {
    public:
        static std::string cliRoleToString(const CliRole& role);

        static void parseAndProcessCliParameters(const CliRole& role, int argc, char **argv);
    private:
        static const std::string kClassName;

        static void parseCliParameters_(int argc, char **argv);
        static void setParamAndConfig_(const CliRole& role);
        static void processCliParameters_();
        static void createRequiredDirectories_(const CliRole& role);

        static boost::program_options::options_description argument_desc_;
        static boost::program_options::variables_map argument_info_;
    };
}

#endif
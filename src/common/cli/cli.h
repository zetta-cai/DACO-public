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
    class CLI
    {
    public:
        static void parseAndProcessCliParameters(int argc, char **argv);
    private:
        static const std::string kClassName;

        static void parseCliParameters_(int argc, char **argv);
        static void setParamAndConfig_(const std::string& main_class_name);
        static void processCliParameters_();
        static void createRequiredDirectories_(const std::string& main_class_name);

        static boost::program_options::options_description argument_desc_;
        static boost::program_options::variables_map argument_info_;
    };
}

#endif
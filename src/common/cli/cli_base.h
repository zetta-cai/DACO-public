/*
 * CLIBase: the base class to parse and process common CLI parameters for static/dynamic actions and static/dynamic configurations (stored into CommonParam and Config).
 * 
 * NOTE: different CLIs can have overlapped options for the same Params, which will be set at most once.
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLI_BASE_H
#define CLI_BASE_H

#include <string>

#include <boost/program_options.hpp>

namespace covered
{
    class CLIBase
    {
    public:
        CLIBase();
        ~CLIBase();

        void parseAndProcessCliParameters(int argc, char **argv);
    private:
        static const std::string kClassName;

        virtual void addCliParameters_();
        void parseCliParameters_(int argc, char **argv);
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void processCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name);

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_process_cli_parameters_;
        bool is_create_required_directories_;
    protected:
        boost::program_options::options_description argument_desc_;
        boost::program_options::variables_map argument_info_;
    };
}

#endif
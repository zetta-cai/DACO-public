/*
 * CLIBase: the base class to parse and process common CLI parameters for static/dynamic actions and static/dynamic configurations.
 *
 * NOTE: CLIs only track dynamic parameters related with evaluation results.
 * 
 * NOTE: different CLIs should NOT have overlapped parameters, and each virtual function will be invoked at most once.
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
        virtual ~CLIBase();

        std::string getMainClassName() const;
        bool isSingleNode() const;
        std::string getConfigFilepath() const;
        bool isDebug() const;
        bool isTrackEvent() const;

        void parseAndProcessCliParameters(int argc, char **argv);
    private:
        static const std::string kClassName;

        void checkMainClassName_() const;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;

        std::string main_class_name_; // Come from argv[0]
        bool is_single_node_; // Come from --multinode
        std::string config_filepath_; // NO effect on results
        bool is_debug_; // Come from --debug with NO effect on results
        bool track_event_; // NO effect on results
    protected:
        virtual void addCliParameters_();
        void parseCliParameters_(int argc, char **argv);
        virtual void setParamAndConfig_(const std::string& main_class_name);
        void processCliParameters_();
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) = 0;

        boost::program_options::options_description argument_desc_;
        boost::program_options::variables_map argument_info_;
    };
}

#endif
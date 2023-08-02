/*
 * ClientCLI: parse and process client CLI parameters for static/dynamic actions and static/dynamic configurations (stored into ClientParam, EdgeParam::edgecnt_, PropagationParam::propagation_latency_clientedge_us_, and WorkloadParam).
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLIENT_CLI_H
#define CLIENT_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"

namespace covered
{
    class ClientCLI : public CLIBase
    {
    public:
        ClientCLI(int argc, char **argv);
        ~ClientCLI();
    private:
        static const std::string kClassName;

        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void processCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name);

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_process_cli_parameters_;
        bool is_create_required_directories_;
    };
}

#endif
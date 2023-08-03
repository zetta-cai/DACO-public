/*
 * CloudCLI: parse and process cloud CLI parameters for dynamic configurations (stored into CloudParam).
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef CLOUD_CLI_H
#define CLOUD_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"
#include "common/cli/propagation_cli.h"

namespace covered
{
    class CloudCLI : virtual public PropagationCLI
    {
    public:
        CloudCLI();
        CloudCLI(int argc, char **argv);
        ~CloudCLI();
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_create_required_directories_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif
/*
 * PropagationCLI: parse and process propagation CLI parameters for dynamic configurations (stored into PropagationParam).
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef PROPAGATION_CLI_H
#define PROPAGATION_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"

namespace covered
{
    class PropagationCLI : virtual public CLIBase
    {
    public:
        PropagationCLI();
        ~PropagationCLI();
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
    };
}

#endif
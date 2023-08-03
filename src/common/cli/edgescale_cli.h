/*
 * EdgescaleCLI: parse and process edgescale CLI parameters for dynamic configurations (stored into EdgescaleParam).
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGESCALE_CLI_H
#define EDGESCALE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"

namespace covered
{
    class EdgescaleCLI : virtual public CLIBase
    {
    public:
        EdgescaleCLI();
        ~EdgescaleCLI();
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
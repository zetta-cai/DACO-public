/*
 * EdgeCLI: parse and process edge CLI parameters for dynamic configurations (stored into EdgeParam).
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGE_CLI_H
#define EDGE_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"
#include "common/cli/edgescale_cli.h"
#include "common/cli/propagation_cli.h"

namespace covered
{
    class EdgeCLI : virtual public EdgescaleCLI, virtual public PropagationCLI
    {
    public:
        EdgeCLI();
        EdgeCLI(int argc, char **argv);
        ~EdgeCLI();
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_create_required_directories_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void createRequiredDirectories_(const std::string& main_class_name);
    };
}

#endif
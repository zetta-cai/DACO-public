/*
 * ClientCLI: parse and process client CLI parameters dynamic configurations (stored into ClientParam).
 * 
 * By Siyuan Sheng (2023.08.02).
 */

#ifndef CLIENT_CLI_H
#define CLIENT_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "common/cli/cli_base.h"
#include "common/cli/edgescale_cli.h"
#include "common/cli/propagation_cli.h"
#include "common/cli/workload_cli.h"

namespace covered
{
    class ClientCLI : virtual public EdgescaleCLI, virtual public PropagationCLI, virtual public WorkloadCLI
    {
    public:
        ClientCLI();
        ClientCLI(int argc, char **argv);
        ~ClientCLI();
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
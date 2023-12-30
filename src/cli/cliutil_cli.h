/*
 * CliutilCLI: cover all possible CLI parameters of all components such that cliutil can generate CLI string for python exp scripts.
 * 
 * By Siyuan Sheng (2023.12.30).
 */

#ifndef CLIUTIL_CLI_H
#define CLIUTIL_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/dataset_loader_cli.h"
#include "cli/simulator_cli.h"

namespace covered
{
    class CliutilCLI : virtual public DatasetLoaderCLI, virtual public SimulatorCLI
    {
    public:
        CliutilCLI(int argc, char **argv);
        virtual ~CliutilCLI();

        std::string toCliString(); // NOT virtual for cilutil
        virtual void clearIsToCliString(); // Idempotent operation: clear is_to_cli_string_ for the next toCliString()
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;

        bool is_to_cli_string_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif
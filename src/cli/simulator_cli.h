/*
 * SimulatorCLI: parse and process simulator CLI parameters dynamic configurations.
 * 
 * By Siyuan Sheng (2023.08.04).
 */

#ifndef SIMULATOR_CLI_H
#define SIMULATOR_CLI_H

#include <string>

#include <boost/program_options.hpp>

#include "cli/cloud_cli.h"
#include "cli/evaluator_cli.h"

namespace covered
{
    class SimulatorCLI : virtual public CloudCLI, virtual public EvaluatorCLI
    {
    public:
        SimulatorCLI(int argc, char **argv);
        virtual ~SimulatorCLI();
    private:
        static const std::string kClassName;

        bool is_add_cli_parameters_;
        bool is_set_param_and_config_;
        bool is_dump_cli_parameters_;
        bool is_create_required_directories_;
    protected:
        virtual void addCliParameters_();
        virtual void setParamAndConfig_(const std::string& main_class_name);
        virtual void dumpCliParameters_();
        virtual void createRequiredDirectories_(const std::string& main_class_name) override;
    };
}

#endif
#include "cli/simulator_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    const std::string SimulatorCLI::kClassName("SimulatorCLI");

    SimulatorCLI::SimulatorCLI(int argc, char **argv) : CloudCLI(), EvaluatorCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    SimulatorCLI::~SimulatorCLI() {}

    void SimulatorCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CloudCLI::addCliParameters_();
            EvaluatorCLI::addCliParameters_();

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void SimulatorCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CloudCLI::setParamAndConfig_(main_class_name);
            EvaluatorCLI::setParamAndConfig_(main_class_name);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void SimulatorCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            CloudCLI::dumpCliParameters_();
            EvaluatorCLI::dumpCliParameters_();

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void SimulatorCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CloudCLI::createRequiredDirectories_(main_class_name);
            EvaluatorCLI::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }

        return;
    }
}
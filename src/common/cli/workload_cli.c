#include "common/cli/workload_cli.h"

#include "common/config.h"
#include "common/param/workload_param.h"
#include "common/util.h"

namespace covered
{
    const std::string WorkloadCLI::kClassName("WorkloadCLI");

    WorkloadCLI::WorkloadCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false)
    {
    }

    WorkloadCLI::~WorkloadCLI() {}

    void WorkloadCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("keycnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
                ("workload_name", boost::program_options::value<std::string>()->default_value(WorkloadParam::FACEBOOK_WORKLOAD_NAME), "workload name")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void WorkloadCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t keycnt = argument_info_["keycnt"].as<uint32_t>();
            std::string workload_name = argument_info_["workload_name"].as<std::string>();

            // Store workload CLI parameters for dynamic configurations and mark WorkloadParam as valid
            WorkloadParam::setParameters(keycnt, workload_name);

            is_set_param_and_config_ = true;
        }

        return;
    }
}
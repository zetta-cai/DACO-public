#include "common/cli/propagation_cli.h"

#include "common/config.h"
#include "common/param/propagation_param.h"
#include "common/util.h"

namespace covered
{
    const std::string PropagationCLI::kClassName("PropagationCLI");

    PropagationCLI::PropagationCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false)
    {
    }

    PropagationCLI::~PropagationCLI() {}

    void PropagationCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("propagation_latency_clientedge_us", boost::program_options::value<uint32_t>()->default_value(1000), "the propagation latency between client and edge (in units of us)")
                ("propagation_latency_crossedge_us", boost::program_options::value<uint32_t>()->default_value(10000), "the propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_edgecloud_us", boost::program_options::value<uint32_t>()->default_value(100000), "the propagation latency between edge and cloud (in units of us)")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void PropagationCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            CLIBase::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t propagation_latency_clientedge_us = argument_info_["propagation_latency_clientedge_us"].as<uint32_t>();
            uint32_t propagation_latency_crossedge_us = argument_info_["propagation_latency_crossedge_us"].as<uint32_t>();
            uint32_t propagation_latency_edgecloud_us = argument_info_["propagation_latency_edgecloud_us"].as<uint32_t>();

            // Store propagation CLI parameters for dynamic configurations and mark PropagationParam as valid
            PropagationParam::setParameters(propagation_latency_clientedge_us, propagation_latency_crossedge_us, propagation_latency_edgecloud_us);

            is_set_param_and_config_ = true;
        }

        return;
    }
}
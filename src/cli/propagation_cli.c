#include "cli/propagation_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // NOTE: here client-edge, cross-edge, and edge-cloud propagation latency are single-trip latency, which should be counted twice within an RTT -> set default single-trip propagation latency as 0.5/5/50ms such that default propagation RTT is 1/10/100ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US = 500;
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US = 5000;
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US = 50000;

    const std::string PropagationCLI::kClassName("PropagationCLI");

    PropagationCLI::PropagationCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_to_cli_string_(false)
    {
        propagation_latency_clientedge_us_ = 0;
        propagation_latency_crossedge_us_ = 0;
        propagation_latency_edgecloud_us_ = 0;
    }

    PropagationCLI::~PropagationCLI() {}

    uint32_t PropagationCLI::getPropagationLatencyClientedgeUs() const
    {
        return propagation_latency_clientedge_us_;
    }

    uint32_t PropagationCLI::getPropagationLatencyCrossedgeUs() const
    {
        return propagation_latency_crossedge_us_;
    }

    uint32_t PropagationCLI::getPropagationLatencyEdgecloudUs() const
    {
        return propagation_latency_edgecloud_us_;
    }

    std::string PropagationCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);

            oss << CLIBase::toCliString();
            if (propagation_latency_clientedge_us_ != DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US)
            {
                oss << " --propagation_latency_clientedge_us " << propagation_latency_clientedge_us_;
            }
            if (propagation_latency_crossedge_us_ != DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US)
            {
                oss << " --propagation_latency_crossedge_us " << propagation_latency_crossedge_us_;
            }
            if (propagation_latency_edgecloud_us_ != DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US)
            {
                oss << " --propagation_latency_edgecloud_us " << propagation_latency_edgecloud_us_;
            }

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void PropagationCLI::clearIsToCliString()
    {
        CLIBase::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

    void PropagationCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            CLIBase::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("propagation_latency_clientedge_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US), "the propagation latency between client and edge (in units of us)")
                ("propagation_latency_crossedge_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US), "the propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_edgecloud_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US), "the propagation latency between edge and cloud (in units of us)")
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

            // Store propagation CLI parameters for dynamic configurations
            propagation_latency_clientedge_us_ = propagation_latency_clientedge_us;
            propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
            propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;

            is_set_param_and_config_ = true;
        }

        return;
    }

    void PropagationCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            CLIBase::verifyAndDumpCliParameters_(main_class_name);

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "One-way propagation latency between client and edge: " << propagation_latency_clientedge_us_ << "us" << std::endl;
            oss << "One-way propagation latency between edge and edge: " << propagation_latency_crossedge_us_ << "us" << std::endl;
            oss << "One-way propagation latency between edge and cloud: " << propagation_latency_edgecloud_us_ << "us";
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }
}
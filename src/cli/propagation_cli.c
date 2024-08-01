#include "cli/propagation_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // NOTE: here client-edge, cross-edge, and edge-cloud propagation latency are round-trip latency, which should be counted 1/2 when issuing messages (NOTE: still asymmetric due to different random seeds of incoming and outcoming traffic of a link)
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US = 1000; // 1ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US = 10000; // 10ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US = 100000; // 100ms

    const std::string PropagationCLI::kClassName("PropagationCLI");

    PropagationCLI::PropagationCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_to_cli_string_(false)
    {
        propagation_latency_clientedge_avg_us_ = 0;
        propagation_latency_crossedge_avg_us_ = 0;
        propagation_latency_edgecloud_avg_us_ = 0;
    }

    PropagationCLI::~PropagationCLI() {}

    uint32_t PropagationCLI::getPropagationLatencyClientedgeAvgUs() const
    {
        return propagation_latency_clientedge_avg_us_;
    }

    uint32_t PropagationCLI::getPropagationLatencyCrossedgeAvgUs() const
    {
        return propagation_latency_crossedge_avg_us_;
    }

    uint32_t PropagationCLI::getPropagationLatencyEdgecloudAvgUs() const
    {
        return propagation_latency_edgecloud_avg_us_;
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
            if (propagation_latency_clientedge_avg_us_ != DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US)
            {
                oss << " --propagation_latency_clientedge_avg_us " << propagation_latency_clientedge_avg_us_;
            }
            if (propagation_latency_crossedge_avg_us_ != DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US)
            {
                oss << " --propagation_latency_crossedge_avg_us " << propagation_latency_crossedge_avg_us_;
            }
            if (propagation_latency_edgecloud_avg_us_ != DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US)
            {
                oss << " --propagation_latency_edgecloud_avg_us " << propagation_latency_edgecloud_avg_us_;
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
                ("propagation_latency_clientedge_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US), "the average propagation latency between client and edge (in units of us)")
                ("propagation_latency_crossedge_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US), "the average propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_edgecloud_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US), "the average propagation latency between edge and cloud (in units of us)")
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

            uint32_t propagation_latency_clientedge_avg_us = argument_info_["propagation_latency_clientedge_avg_us"].as<uint32_t>();
            uint32_t propagation_latency_crossedge_avg_us = argument_info_["propagation_latency_crossedge_avg_us"].as<uint32_t>();
            uint32_t propagation_latency_edgecloud_avg_us = argument_info_["propagation_latency_edgecloud_avg_us"].as<uint32_t>();

            // Store propagation CLI parameters for dynamic configurations
            propagation_latency_clientedge_avg_us_ = propagation_latency_clientedge_avg_us;
            propagation_latency_crossedge_avg_us_ = propagation_latency_crossedge_avg_us;
            propagation_latency_edgecloud_avg_us_ = propagation_latency_edgecloud_avg_us;

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
            oss << "Average propagation latency between client and edge: " << propagation_latency_clientedge_avg_us_ << "us" << std::endl;
            oss << "Average propagation latency between edge and edge: " << propagation_latency_crossedge_avg_us_ << "us" << std::endl;
            oss << "Average propagation latency between edge and cloud: " << propagation_latency_edgecloud_avg_us_ << "us";
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }
}
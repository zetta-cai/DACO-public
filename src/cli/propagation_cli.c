#include "cli/propagation_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // NOTE: here client-edge, cross-edge, and edge-cloud propagation latency are round-trip latency, which should be counted 1/2 when issuing messages (NOTE: still asymmetric due to different random seeds of incoming and outcoming traffic of a link)
    // NOTE: the default settings refer to realistic edge networks in WWW'22 TMC'24 MagNet
    const std::string PropagationCLI::DEFAULT_PROPAGATION_LATENCY_DISTNAME = "uniform";
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_LBOUND_US = 500; // 0.5ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US = 1000; // 1ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_RBOUND_US = 1500; // 1.5ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_LBOUND_US = 1500; // 1.5ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US = 3000; // 3ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_RBOUND_US = 4500; // 4.5ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_LBOUND_US = 6500; // 6.5ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US = 13000; // 13ms
    const uint32_t PropagationCLI::DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_RBOUND_US = 19500; // 19.5ms
    const std::string PropagationCLI::P2P_LATENCY_MAT_PATH = "";

    const std::string PropagationCLI::kClassName("PropagationCLI");

    PropagationCLI::PropagationCLI() : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_to_cli_string_(false), cli_latency_info_()
    {
    }

    PropagationCLI::~PropagationCLI() {}

    CLILatencyInfo PropagationCLI::getCLILatencyInfo() const
    {
        return cli_latency_info_;
    }

    std::string PropagationCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        { 
            // oss<< "debug for toCliString: ";
            // oss << is_add_cli_parameters_ << " " << is_set_param_and_config_ << " " << is_dump_cli_parameters_ << " " << is_to_cli_string_;
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            
        
            oss << CLIBase::toCliString();
            if (cli_latency_info_.getPropagationLatencyDistname() != DEFAULT_PROPAGATION_LATENCY_DISTNAME)
            {
                oss << " --propagation_latency_distname " << cli_latency_info_.getPropagationLatencyDistname();
            }
            if (cli_latency_info_.getPropagationLatencyClientedgeLboundUs() != DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_LBOUND_US)
            {
                oss << " --propagation_latency_clientedge_lbound_us " << cli_latency_info_.getPropagationLatencyClientedgeLboundUs();
            }
            if (cli_latency_info_.getPropagationLatencyClientedgeAvgUs() != DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US)
            {
                oss << " --propagation_latency_clientedge_avg_us " << cli_latency_info_.getPropagationLatencyClientedgeAvgUs();
            }
            if (cli_latency_info_.getPropagationLatencyClientedgeRboundUs() != DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_RBOUND_US)
            {
                oss << " --propagation_latency_clientedge_rbound_us " << cli_latency_info_.getPropagationLatencyClientedgeRboundUs();
            }
            if (cli_latency_info_.getPropagationLatencyCrossedgeLboundUs() != DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_LBOUND_US)
            {
                oss << " --propagation_latency_crossedge_lbound_us " << cli_latency_info_.getPropagationLatencyCrossedgeLboundUs();
            }
            if (cli_latency_info_.getPropagationLatencyCrossedgeAvgUs() != DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US)
            {
                oss << " --propagation_latency_crossedge_avg_us " << cli_latency_info_.getPropagationLatencyCrossedgeAvgUs();
            }
            if (cli_latency_info_.getPropagationLatencyCrossedgeRboundUs() != DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_RBOUND_US)
            {
                oss << " --propagation_latency_crossedge_rbound_us " << cli_latency_info_.getPropagationLatencyCrossedgeRboundUs();
            }
            if (cli_latency_info_.getPropagationLatencyEdgecloudLboundUs() != DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_LBOUND_US)
            {
                oss << " --propagation_latency_edgecloud_lbound_us " << cli_latency_info_.getPropagationLatencyEdgecloudLboundUs();
            }
            if (cli_latency_info_.getPropagationLatencyEdgecloudAvgUs() != DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US)
            {
                oss << " --propagation_latency_edgecloud_avg_us " << cli_latency_info_.getPropagationLatencyEdgecloudAvgUs();
            }
            if (cli_latency_info_.getPropagationLatencyEdgecloudRboundUs() != DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_RBOUND_US)
            {
                oss << " --propagation_latency_edgecloud_rbound_us " << cli_latency_info_.getPropagationLatencyEdgecloudRboundUs();
            }
            // oss << " --p2p_latency_mat_path " << cli_latency_info_.getPropagationP2PLatencyMatPath();
            if (cli_latency_info_.getPropagationP2PLatencyMatPath() != P2P_LATENCY_MAT_PATH)
            {
                oss << " --p2p_latency_mat_path " << cli_latency_info_.getPropagationP2PLatencyMatPath();
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

            std::string distname_descstr = "the name of the propagation latency distribution (e.g., " + Util::PROPAGATION_SIMULATION_UNIFORM_DISTNAME + " and " + Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME + ")";

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("propagation_latency_distname", boost::program_options::value<std::string>()->default_value(DEFAULT_PROPAGATION_LATENCY_DISTNAME), distname_descstr.c_str())
                ("propagation_latency_clientedge_lbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_LBOUND_US), "the left bound propagation latency between client and edge (in units of us)")
                ("propagation_latency_clientedge_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_AVG_US), "the average propagation latency between client and edge (in units of us)")
                ("propagation_latency_clientedge_rbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_RBOUND_US), "the right bound propagation latency between client and edge (in units of us)")
                ("propagation_latency_crossedge_lbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_LBOUND_US), "the left bound propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_crossedge_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_AVG_US), "the average propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_crossedge_rbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_RBOUND_US), "the right bound propagation latency between edge and neighbor (in units of us)")
                ("propagation_latency_edgecloud_lbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_LBOUND_US), "the left bound propagation latency between edge and cloud (in units of us)")
                ("propagation_latency_edgecloud_avg_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_AVG_US), "the average propagation latency between edge and cloud (in units of us)")
                ("propagation_latency_edgecloud_rbound_us", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_RBOUND_US), "the right bound propagation latency between edge and cloud (in units of us)")
                ("p2p_latency_mat_path", boost::program_options::value<std::string>()->default_value(P2P_LATENCY_MAT_PATH), "the path to the P2P latency matrix (in units of us)");

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

            std::string propagation_latency_distname = argument_info_["propagation_latency_distname"].as<std::string>();
            uint32_t propagation_latency_clientedge_lbound_us = argument_info_["propagation_latency_clientedge_lbound_us"].as<uint32_t>();
            uint32_t propagation_latency_clientedge_avg_us = argument_info_["propagation_latency_clientedge_avg_us"].as<uint32_t>();
            uint32_t propagation_latency_clientedge_rbound_us = argument_info_["propagation_latency_clientedge_rbound_us"].as<uint32_t>();
            uint32_t propagation_latency_crossedge_lbound_us = argument_info_["propagation_latency_crossedge_lbound_us"].as<uint32_t>();
            uint32_t propagation_latency_crossedge_avg_us = argument_info_["propagation_latency_crossedge_avg_us"].as<uint32_t>();
            uint32_t propagation_latency_crossedge_rbound_us = argument_info_["propagation_latency_crossedge_rbound_us"].as<uint32_t>();
            uint32_t propagation_latency_edgecloud_lbound_us = argument_info_["propagation_latency_edgecloud_lbound_us"].as<uint32_t>();
            uint32_t propagation_latency_edgecloud_avg_us = argument_info_["propagation_latency_edgecloud_avg_us"].as<uint32_t>();
            uint32_t propagation_latency_edgecloud_rbound_us = argument_info_["propagation_latency_edgecloud_rbound_us"].as<uint32_t>();
            std::string p2p_latency_mat_path = argument_info_["p2p_latency_mat_path"].as<std::string>();
            

            // Store propagation CLI parameters for dynamic configurations
            cli_latency_info_ = CLILatencyInfo(propagation_latency_distname, propagation_latency_clientedge_lbound_us, propagation_latency_clientedge_avg_us, propagation_latency_clientedge_rbound_us, propagation_latency_crossedge_lbound_us, propagation_latency_crossedge_avg_us, propagation_latency_crossedge_rbound_us, propagation_latency_edgecloud_lbound_us, propagation_latency_edgecloud_avg_us, propagation_latency_edgecloud_rbound_us, p2p_latency_mat_path);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void PropagationCLI::verifyAndDumpCliParameters_(const std::string& main_class_name)
    {
        if (!is_dump_cli_parameters_)
        {
            CLIBase::verifyAndDumpCliParameters_(main_class_name);

            verifyPropagationLatencyDistname_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            oss << "Propagation latency distribution: " << cli_latency_info_.getPropagationLatencyDistname() << std::endl;
            oss << "Left bound propagation latency between client and edge: " << cli_latency_info_.getPropagationLatencyClientedgeLboundUs() << "us" << std::endl;
            oss << "Average propagation latency between client and edge: " << cli_latency_info_.getPropagationLatencyClientedgeAvgUs() << "us" << std::endl;
            oss << "Right bound propagation latency between client and edge: " << cli_latency_info_.getPropagationLatencyClientedgeRboundUs() << "us" << std::endl;
            oss << "Left bound propagation latency between edge and edge: " << cli_latency_info_.getPropagationLatencyCrossedgeLboundUs() << "us" << std::endl;
            oss << "Average propagation latency between edge and edge: " << cli_latency_info_.getPropagationLatencyCrossedgeAvgUs() << "us" << std::endl;
            oss << "Right bound propagation latency between edge and edge: " << cli_latency_info_.getPropagationLatencyCrossedgeRboundUs() << "us" << std::endl;
            oss << "Left bound propagation latency between edge and cloud: " << cli_latency_info_.getPropagationLatencyEdgecloudLboundUs() << "us" << std::endl;
            oss << "Average propagation latency between edge and cloud: " << cli_latency_info_.getPropagationLatencyEdgecloudAvgUs() << "us";
            oss << "Right bound propagation latency between edge and cloud: " << cli_latency_info_.getPropagationLatencyEdgecloudRboundUs() << "us";
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void PropagationCLI::verifyPropagationLatencyDistname_() const
    {
        const std::string distname = cli_latency_info_.getPropagationLatencyDistname();
        if (distname != Util::PROPAGATION_SIMULATION_UNIFORM_DISTNAME && distname != Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME)
        {
            std::ostringstream oss;
            oss << "propagation latency distribution name " << distname << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
    // std::string PropagationCLI::getP2PLatencyMatrixPath() const{
    //     return p2p_latency_mat_path;
    // }
}
#include "common/cli/client_cli.h"

#include "common/config.h"
#include "common/param/client_param.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientCLI::kClassName("ClientCLI");

    ClientCLI::ClientCLI() : EdgescaleCLI(), PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
    }

    ClientCLI::ClientCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    ClientCLI::~ClientCLI() {}

    void ClientCLI::addCliParameters_()
    {
        if (!is_add_cli_parameters_)
        {
            EdgescaleCLI::addCliParameters_();
            PropagationCLI::addCliParameters_();
            WorkloadCLI::addCliParameters_();

            // (1) Create CLI parameter description

            // Dynamic configurations for client
            argument_desc_.add_options()
                ("clientcnt", boost::program_options::value<uint32_t>()->default_value(1), "the total number of clients")
                ("disable_warmup_speedup", "disable speedup mode for warmup phase")
                ("opcnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
                ("perclient_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each client")
            ;

            is_add_cli_parameters_ = true;
        }

        return;
    }

    void ClientCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        if (!is_set_param_and_config_)
        {
            EdgescaleCLI::setParamAndConfig_(main_class_name);
            PropagationCLI::setParamAndConfig_(main_class_name);
            WorkloadCLI::setParamAndConfig_(main_class_name);

            // (3) Get CLI parameters for client dynamic configurations

            uint32_t clientcnt = argument_info_["clientcnt"].as<uint32_t>();
            bool is_warmup_speedup = true;
            if (argument_info_.count("disable_warmup_speedup"))
            {
                is_warmup_speedup = false;
            }
            uint32_t opcnt = argument_info_["opcnt"].as<uint32_t>();
            uint32_t perclient_workercnt = argument_info_["perclient_workercnt"].as<uint32_t>();

            // Store client CLI parameters for dynamic configurations and mark ClientParam as valid
            ClientParam::setParameters(clientcnt, is_warmup_speedup, opcnt, perclient_workercnt);

            is_set_param_and_config_ = true;
        }

        return;
    }

    void ClientCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            CLIBase::createRequiredDirectories_(main_class_name);

            is_create_required_directories_ = true;
        }
        return;
    }
}
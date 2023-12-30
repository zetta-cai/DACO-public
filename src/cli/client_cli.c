#include "cli/client_cli.h"

#include "common/config.h"
#include "common/util.h"

namespace covered
{
    // const uint32_t ClientCLI::DEFAULT_CLIENTCNT = 1;
    const bool ClientCLI::DEFAULT_IS_WARMUP_SPEEDUP = true;
    const uint32_t ClientCLI::DEFAULT_PERCLIENT_OPCNT = 1000000;
    const uint32_t ClientCLI::DEFAULT_PERCLIENT_WORKERCNT = 1;

    const std::string ClientCLI::kClassName("ClientCLI");

    ClientCLI::ClientCLI() : EdgescaleCLI(), PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        // clientcnt_ = 0;
        is_warmup_speedup_ = true;
        perclient_opcnt_ = 0;
        perclient_workercnt_ = 0;
    }

    ClientCLI::ClientCLI(int argc, char **argv) : EdgescaleCLI(), PropagationCLI(), WorkloadCLI(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_dump_cli_parameters_(false), is_create_required_directories_(false), is_to_cli_string_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    ClientCLI::~ClientCLI() {}

    // uint32_t ClientCLI::getClientcnt() const
    // {
    //     return clientcnt_;
    // }

    bool ClientCLI::isWarmupSpeedup() const
    {
        return is_warmup_speedup_;
    }

    uint32_t ClientCLI::getPerclientOpcnt() const
    {
        return perclient_opcnt_;
    }

    uint32_t ClientCLI::getPerclientWorkercnt() const
    {
        return perclient_workercnt_;
    }

    std::string ClientCLI::toCliString()
    {
        std::ostringstream oss;
        if (!is_to_cli_string_)
        {
            // NOTE: MUST already parse and process CLI parameters
            assert(is_add_cli_parameters_);
            assert(is_set_param_and_config_);
            assert(is_dump_cli_parameters_);
            assert(is_create_required_directories_);

            oss << EdgescaleCLI::toCliString();
            oss << PropagationCLI::toCliString();
            oss << WorkloadCLI::toCliString();
            // if (clientcnt_ != DEFAULT_CLIENTCNT)
            // {
            //     oss << " --clientcnt " << clientcnt_;
            // }
            if (is_warmup_speedup_ != DEFAULT_IS_WARMUP_SPEEDUP)
            {
                assert(!is_warmup_speedup_);
                oss << " --disable_warmup_speedup";
            }
            if (perclient_opcnt_ != DEFAULT_PERCLIENT_OPCNT)
            {
                oss << " --perclient_opcnt " << perclient_opcnt_;
            }
            if (perclient_workercnt_ != DEFAULT_PERCLIENT_WORKERCNT)
            {
                oss << " --perclient_workercnt " << perclient_workercnt_;
            }

            is_to_cli_string_ = true;
        }
        
        return oss.str();
    }

    void ClientCLI::clearIsToCliString()
    {
        EdgescaleCLI::clearIsToCliString();
        PropagationCLI::clearIsToCliString();
        WorkloadCLI::clearIsToCliString();
        
        is_to_cli_string_ = false;
        return;
    }

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
                // ("clientcnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_CLIENTCNT), "the total number of clients")
                ("disable_warmup_speedup", "disable speedup mode for warmup phase")
                ("perclient_opcnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PERCLIENT_OPCNT), "the number of operations for each client")
                ("perclient_workercnt", boost::program_options::value<uint32_t>()->default_value(DEFAULT_PERCLIENT_WORKERCNT), "the number of worker threads for each client")
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

            // uint32_t clientcnt = argument_info_["clientcnt"].as<uint32_t>();
            bool is_warmup_speedup = true;
            if (argument_info_.count("disable_warmup_speedup"))
            {
                is_warmup_speedup = false;
            }
            uint32_t perclient_opcnt = argument_info_["perclient_opcnt"].as<uint32_t>();
            uint32_t perclient_workercnt = argument_info_["perclient_workercnt"].as<uint32_t>();

            // Store client CLI parameters for dynamic configurations
            // clientcnt_ = clientcnt;
            is_warmup_speedup_ = is_warmup_speedup;
            perclient_opcnt_ = perclient_opcnt;
            perclient_workercnt_ = perclient_workercnt;
            verifyIntegrity_();

            is_set_param_and_config_ = true;
        }

        return;
    }

    void ClientCLI::dumpCliParameters_()
    {
        if (!is_dump_cli_parameters_)
        {
            EdgescaleCLI::dumpCliParameters_();
            PropagationCLI::dumpCliParameters_();
            WorkloadCLI::dumpCliParameters_();

            // (6) Dump stored CLI parameters and parsed config information if debug

            std::ostringstream oss;
            oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
            // oss << "Client count: " << clientcnt_ << std::endl;
            oss << "Warmup speedup flag: " << (is_warmup_speedup_?"true":"false") << std::endl;
            oss << "Per-client operation count (workload size): " << perclient_opcnt_ << std::endl;
            oss << "Per-client worker count: " << perclient_workercnt_;
            Util::dumpDebugMsg(kClassName, oss.str());

            is_dump_cli_parameters_ = true;
        }

        return;
    }

    void ClientCLI::createRequiredDirectories_(const std::string& main_class_name)
    {
        if (!is_create_required_directories_)
        {
            is_create_required_directories_ = true;
        }
        return;
    }

    void ClientCLI::verifyIntegrity_() const
    {
        // assert(clientcnt_ > 0);
        assert(perclient_opcnt_ > 0);
        assert(perclient_workercnt_ > 0);

        // uint32_t edgecnt = getEdgecnt();
        // if (clientcnt_ < edgecnt)
        // {
        //     std::ostringstream oss;
        //     oss << "clientcnt " << clientcnt_ << " should >= edgecnt " << edgecnt << " for edge-client mapping!";
        //     Util::dumpErrorMsg(kClassName, oss.str());
        //     exit(1);
        // }

        return;
    }
}
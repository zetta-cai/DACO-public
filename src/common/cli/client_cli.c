#include "common/cli/client_cli.h"

#include "common/config.h"
#include "common/param/workload_param.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientCLI::kClassName("ClientCLI");

    ClientCLI::ClientCLI(int argc, char **argv) : CLIBase(), is_add_cli_parameters_(false), is_set_param_and_config_(false), is_process_cli_parameters_(false), is_create_required_directories_(false)
    {
        parseAndProcessCliParameters(argc, argv);
    }

    void ClientCLI::addCliParameters_()
    {
        // (1) Create CLI parameter description

        // Dynamic configurations for client
        argument_desc_.add_options()
            ("clientcnt", boost::program_options::value<uint32_t>()->default_value(1), "the total number of clients")
            ("disable_warmup_speedup", "disable speedup mode for warmup phase")
            ("edgecnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of edge nodes")
            ("keycnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
            ("opcnt", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
            ("perclient_workercnt", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each client")
            ("propagation_latency_clientedge_us", boost::program_options::value<uint32_t>()->default_value(1000), "the propagation latency between client and edge (in units of us)")
            ("workload_name", boost::program_options::value<std::string>()->default_value(WorkloadParam::FACEBOOK_WORKLOAD_NAME), "workload name")
        ;

        return;
    }

    void ClientCLI::setParamAndConfig_(const std::string& main_class_name)
    {
        // (3) Get CLI parameters for client dynamic configurations

        uint32_t clientcnt = argument_info_["clientcnt"].as<uint32_t>();
        bool is_warmup_speedup = true;
        if (argument_info_.count("disable_warmup_speedup"))
        {
            is_warmup_speedup = false;
        }
        uint32_t edgecnt = argument_info_["edgecnt"].as<uint32_t>();
        uint32_t keycnt = argument_info_["keycnt"].as<uint32_t>();
        uint32_t opcnt = argument_info_["opcnt"].as<uint32_t>();
        uint32_t percacheserver_workercnt = argument_info_["percacheserver_workercnt"].as<uint32_t>();
        uint32_t perclient_workercnt = argument_info_["perclient_workercnt"].as<uint32_t>();
        uint32_t propagation_latency_clientedge_us = argument_info_["propagation_latency_clientedge_us"].as<uint32_t>();
        std::string workload_name = argument_info_["workload_name"].as<std::string>();

        // TODO: END HERE

        // Store CLI parameters for dynamic configurations and mark Param as valid
        Param::setParameters(main_class_name, is_single_node, cache_name, capacity_bytes, clientcnt, cloud_storage, config_filepath, is_debug, is_warmup_speedup, edgecnt, hash_name, keycnt, max_warmup_duration_sec, opcnt, percacheserver_workercnt, perclient_workercnt, propagation_latency_clientedge_us, propagation_latency_crossedge_us, propagation_latency_edgecloud_us, stresstest_duration_sec, track_event, workload_name);

        // (4) Load config file for static configurations

        Config::loadConfig();
    }
}
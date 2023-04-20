#include <iostream>
#include <sstream>
#include <pthread.h>

#include <boost/program_options.hpp>

#include "common/util.h"
#include "common/param.h"
#include "common/config.h"
#include "benchmark/benchmark_util.h"
#include "benchmark/client_param.h"
#include "benchmark/client_wrapper.h"

int main(int argc, char **argv) {
    std::string main_class_name = "simulator";

    // (1) Create CLI parameter description

    boost::program_options::options_description argument_desc("Allowed arguments:");
    // Static actions
    argument_desc.add_options()
        ("help,h", "dump help information")
    ;
    // Dynamic configurations
    argument_desc.add_options()
        ("config_file,c", boost::program_options::value<std::string>()->default_value("config.json"), "specify config file path of COVERED")
        ("debug", "enable debug information")
        ("keycnt,k", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
        ("opcnt,o", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
        ("clientcnt,p", boost::program_options::value<uint32_t>()->default_value(2), "the total number of clients")
        ("perclient_workercnt,p", boost::program_options::value<uint32_t>()->default_value(10), "the number of worker threads for each client")
    ;
    // Dynamic actions
    argument_desc.add_options()
        ("version,v", "dump version of COVERED")
    ;

    // (2) Parse CLI parameters (static/dynamic actions and dynamic configurations)

    boost::program_options::variables_map argument_info;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc), argument_info);
    boost::program_options::notify(argument_info);

    // (2.1) Process static actions

    if (argument_info.count("help")) // Dump help information
    {
        std::ostringstream oss;
        oss << argument_desc;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());
        return 1;
    }

    // (2.2) Get CLI paremters for dynamic configurations

    const bool is_simulation = true;
    std::string config_filepath = argument_info["config_file"].as<std::string>();
    bool is_debug = false;
    if (argument_info.count("debug"))
    {
        is_debug = true;
    }
    uint32_t keycnt = argument_info["keycnt"].as<uint32_t>();
    uint32_t opcnt = argument_info["opcnt"].as<uint32_t>();
    uint32_t clientcnt = argument_info["clientcnt"].as<uint32_t>();
    uint32_t perclient_workercnt = argument_info["perclient_workercnt"].as<uint32_t>();

    // Store CLI parameters for dynamic configurations and mark covered::Param as valid
    covered::Param::setParameters(is_simulation, config_filepath, is_debug, keycnt, opcnt, clientcnt, perclient_workercnt);

    // (2.3) Load config file for static configurations

    covered::Config::loadConfig();

    // (2.4) Process dynamic actions

    if (argument_info.count("version"))
    {
        std::ostringstream oss;
        oss << "Current version of COVERED: " << covered::Config::getVersion();
        covered::Util::dumpNormalMsg(main_class_name, oss.str());
        return 1;
    }

    // (3) Dump stored CLI parameters and parsed config information if debug

    if (covered::Param::isDebug())
    {
        covered::Util::dumpDebugMsg(main_class_name, covered::Param::toString());
        covered::Util::dumpDebugMsg(main_class_name, covered::Config::toString());
    }
    
    // (4) Simulate multiple clients by multi-threading

    pthread_t client_threads[clientcnt];
    covered::ClientParam client_params[clientcnt];
    int pthread_returncode;

    // Prepare clientcnt client parameters
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        uint16_t local_client_workload_startport = covered::BenchmarkUtil::getLocalClientWorkloadStartport(global_client_idx);
        std::string local_edge_node_ipstr = covered::BenchmarkUtil::getLocalEdgeNodeIpstr(global_client_idx);
        covered::ClientParam local_client_param(global_client_idx, local_client_workload_startport, local_edge_node_ipstr);
        client_params[global_client_idx] = local_client_param;
    }

    // Launch clientcnt clients
    // NOTE: global_XXX is from the view of entire system, while local_XXX is from the view of each individual client
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        std::ostringstream oss;
        oss << "simulate client " << global_client_idx << " by pthread";
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        pthread_returncode = pthread_create(&client_threads[global_client_idx], NULL, covered::ClientWrapper::launchClient, (void*)(&(client_params[global_client_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch client " << global_client_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // TODO: set local_client_running_ in all clientcnt client parameters to start benchmark

    // TODO: need a class StatisticsTracker (in ClientParam) to collect and process statistics of all workers within each client
        // TODO: StatisticsTracker provides a aggregate method, such that simulator can use an empty StatisticsTracker to aggregate those of all simulated clients
        // TODO: StatisticsTracker also provides serialize/deserialize methods, such that prototype can collect serialized statistics files from all physical clients and deserialize them for aggregation

    // TODO: with the aggregated StatisticsTracker, main thread can dump statistics every 10 seconds if necessary

    // Wait for all clients
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        pthread_returncode = pthread_join(client_threads[global_client_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join client " << global_client_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // TODO: with the aggregated StatisticsTracker, main thread can dump aggregated statistics after joining all sub-threads

    return 0;
}
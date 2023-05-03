#include <iostream>
#include <pthread.h>
#include <sstream>
#include <time.h> // struct timespec
#include <unistd.h> // usleep

#include <boost/program_options.hpp>

#include "benchmark/client_param.h"
#include "benchmark/client_wrapper.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/edge_param.h"
#include "edge/edge_wrapper.h"
#include "workload/workload_base.h"

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
        ("config_file,f", boost::program_options::value<std::string>()->default_value("config.json"), "config file path of COVERED")
        ("debug", "enable debug information")
        ("edgecnt,e", boost::program_options::value<uint32_t>()->default_value(1), "the number of edge nodes")
        ("keycnt,k", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of keys")
        ("opcnt,o", boost::program_options::value<uint32_t>()->default_value(1000000), "the total number of operations")
        ("clientcnt,c", boost::program_options::value<uint32_t>()->default_value(1), "the total number of clients")
        ("perclient_workercnt,p", boost::program_options::value<uint32_t>()->default_value(1), "the number of worker threads for each client")
        ("workload_name,n", boost::program_options::value<std::string>()->default_value(covered::WorkloadBase::FACEBOOK_WORKLOAD_NAME), "workload name")
        ("duration,d", boost::program_options::value<double>()->default_value(10), "benchmark duration")
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
    uint32_t edgecnt = argument_info["edgecnt"].as<uint32_t>();
    uint32_t keycnt = argument_info["keycnt"].as<uint32_t>();
    uint32_t opcnt = argument_info["opcnt"].as<uint32_t>();
    uint32_t clientcnt = argument_info["clientcnt"].as<uint32_t>();
    uint32_t perclient_workercnt = argument_info["perclient_workercnt"].as<uint32_t>();
    std::string workload_name = argument_info["workload_name"].as<std::string>();
    double duration = argument_info["duration"].as<double>();

    // Store CLI parameters for dynamic configurations and mark covered::Param as valid
    covered::Param::setParameters(is_simulation, config_filepath, is_debug, edgecnt, keycnt, opcnt, clientcnt, perclient_workercnt, workload_name, duration);

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

    if (is_debug)
    {
        covered::Util::dumpDebugMsg(main_class_name, covered::Param::toString());
        covered::Util::dumpDebugMsg(main_class_name, covered::Config::toString());
    }

    // (4) TODO: Simulate cloud for backend storage

    // (5) Simulate edge nodes with cooperative caching

    pthread_t edge_threads[edgecnt];
    covered::EdgeParam edge_params[edgecnt];
    int pthread_returncode;

    // (5.1) Prepare edgecnt edge parameters

    for (uint32_t global_edge_idx = 0; global_edge_idx < edgecnt; global_edge_idx++)
    {
        covered::EdgeParam local_edge_param(global_edge_idx);
        edge_params[global_edge_idx] = local_edge_param;
    }

    // (5.2) Launch edgecnt edge nodes

    for (uint32_t global_edge_idx = 0; global_edge_idx < edgecnt; global_edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << global_edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        pthread_returncode = pthread_create(&edge_threads[global_edge_idx], NULL, covered::EdgeWrapper::launchEdge, (void*)(&(edge_params[global_edge_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch edge node " << global_edge_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    
    // (6) Simulate multiple clients by multi-threading

    pthread_t client_threads[clientcnt];
    covered::WorkloadBase* workload_generator_ptrs[clientcnt]; // need delete
    covered::ClientParam client_params[clientcnt];

    // (6.1) Prepare clientcnt client parameters

    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        workload_generator_ptrs[global_client_idx] = covered::WorkloadBase::getWorkloadGenerator(workload_name, global_client_idx);

        covered::ClientParam local_client_param(global_client_idx, workload_generator_ptrs[global_client_idx]);
        client_params[global_client_idx] = local_client_param;
    }

    // (6.2) Launch clientcnt clients

    // NOTE: global_XXX is from the view of entire system, while local_XXX is from the view of each individual client
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << global_client_idx;
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

    // (6.3) Launch clientcnt statistics trackers

    // TODO: need a class StatisticsTracker (in ClientParam) to collect and process statistics of all workers within each client
        // TODO: StatisticsTracker provides a aggregate method, such that simulator can use an empty StatisticsTracker to aggregate those of all simulated clients
        // TODO: StatisticsTracker also provides serialize/deserialize methods, such that prototype can collect serialized statistics files from all physical clients and deserialize them for aggregation

    // (7) Start benchmark and dump intermediate statistics

    // (7.1) Set local_client_running_ = true in all clientcnt client parameters to start benchmark

    covered::Util::dumpNormalMsg(main_class_name, "Start benchmark...");
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        client_params[global_client_idx].setClientRunning();
    }

    // (7.2) Dump intermediate statistics

    struct timespec start_timespec = covered::Util::getCurrentTimespec();
    while (true)
    {
        usleep(covered::Util::SLEEP_INTERVAL_US);

        struct timespec end_timespec = covered::Util::getCurrentTimespec();
        double delta_time = covered::Util::getDeltaTime(end_timespec, start_timespec);
        if (delta_time >= duration)
        {
            break;
        }
        
        // TODO: with the aggregated StatisticsTracker, main thread can dump statistics every 10 seconds if necessary
    }

    // (8) Stop benchmark and dump aggregated statistics

    // (8.1) Reset local_client_running_ = false in clientcnt client parameters to stop benchmark

    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        client_params[global_client_idx].resetClientRunning();
    }
    covered::Util::dumpNormalMsg(main_class_name, "Stop benchmark...");

    // (8.2) Wait for clientcnt clients

    covered::Util::dumpNormalMsg(main_class_name, "wait for all clients...");
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
    covered::Util::dumpNormalMsg(main_class_name, "all clients are done");

    // (8.3) Reset local_edge_running_ = false in edgecnt edge parameters to stop edge nodes

    for (uint32_t global_edge_idx = 0; global_edge_idx < edgecnt; global_edge_idx++)
    {
        edge_params[global_edge_idx].resetEdgeRunning();
    }
    covered::Util::dumpNormalMsg(main_class_name, "Stop edge nodes...");

    // (8.4) Wait for edgecnt edge nodes

    covered::Util::dumpNormalMsg(main_class_name, "wait for all edge nodes...");
    for (uint32_t global_edge_idx = 0; global_edge_idx < edgecnt; global_edge_idx++)
    {
        pthread_returncode = pthread_join(edge_threads[global_edge_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join edge node " << global_edge_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all edge nodes are done");

    // (8.3) Dump aggregated statistics

    // TODO: with the aggregated StatisticsTracker, main thread can dump aggregated statistics after joining all sub-threads

    // (8.4) Delete variables in heap

    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        if (workload_generator_ptrs[global_client_idx] != NULL)
        {
            delete workload_generator_ptrs[global_client_idx];
            workload_generator_ptrs[global_client_idx]= NULL;
        }
    }

    return 0;
}
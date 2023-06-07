#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <time.h> // struct timespec
#include <unistd.h> // usleep

#include "benchmark/client_param.h"
#include "benchmark/client_wrapper.h"
#include "common/cli.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cloud/cloud_param.h"
#include "cloud/cloud_wrapper.h"
#include "edge/edge_param.h"
#include "edge/edge_wrapper.h"
#include "statistics/client_statistics_tracker.h"
#include "workload/workload_wrapper_base.h"

void* launchCloud(void* cloud_param_ptr)
{
    CloudWrapper local_cloud(Param::getCloudStorage(), (CloudParam*)cloud_param_ptr);
    local_cloud.start();
    
    pthread_exit(NULL);
    return NULL;
}

void* launchEdge(void* edge_param_ptr)
{
    EdgeWrapperBase local_edge(Param::getCacheName(), (EdgeParam*)edge_param_ptr);
    local_edge.start();
    
    pthread_exit(NULL);
    return NULL;
}

void* launchClient(void* client_param_ptr)
{
    ClientWrapper local_client((ClientParam*)client_param_ptr);
    local_client.start();
    
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {
    // (1) Parse and process CLI parameters (set configurations in Config and Param)
    covered::CLI::parseAndProcessCliParameters(argc, argv);
    const std::string main_class_name = covered::Param::getMainClassName();

    int pthread_returncode;

    // (2) Simulate a single cloud node for backend storage

    pthread_t cloud_thread;
    covered::CloudParam cloud_param;

    // (2.1) Prepare one cloud parameter

    cloud_param = covered::CloudParam();

    // (2.2) Launch one cloud node

    covered::Util::dumpNormalMsg(main_class_name, "launch cloud node");

    pthread_returncode = pthread_create(&cloud_thread, NULL, launchCloud, (void*)(&(cloud_param)));
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to launch cloud node (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }

    // (3) Simulate edgecnt edge nodes with cooperative caching

    const uint32_t edgecnt = covered::Param::getEdgecnt();
    pthread_t edge_threads[edgecnt];
    covered::EdgeParam edge_params[edgecnt];

    // (3.1) Prepare edgecnt edge parameters

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        covered::EdgeParam edge_param(edge_idx);
        edge_params[edge_idx] = edge_param;
    }

    // (3.2) Launch edgecnt edge nodes

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        pthread_returncode = pthread_create(&edge_threads[edge_idx], NULL, launchEdge, (void*)(&(edge_params[edge_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch edge node " << edge_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    
    // (4) Simulate clientcnt clients by multi-threading

    const uint32_t clientcnt = covered::Param::getClientcnt();
    pthread_t client_threads[clientcnt];
    covered::WorkloadWrapperBase* workload_generator_ptrs[clientcnt]; // Release at the end
    covered::ClientStatisticsTracker* client_statistics_tracker_ptrs[clientcnt]; // Release at the end
    covered::ClientParam client_params[clientcnt];

    // (4.1) Prepare clientcnt client parameters

    const std::string workload_name = covered::Param::getWorkloadName();
    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        // Create workload generator for the client
        workload_generator_ptrs[client_idx] = covered::WorkloadWrapperBase::getWorkloadGenerator(workload_name, client_idx);
        assert(workload_generator_ptrs[client_idx] != NULL);

        // Create statistics tracker for the client
        client_statistics_tracker_ptrs[client_idx] = new covered::ClientStatisticsTracker(covered::Param::getPerclientWorkercnt(), covered::Config::getLatencyHistogramSize());
        assert(client_statistics_tracker_ptrs[client_idx] != NULL);

        covered::ClientParam client_param(client_idx, workload_generator_ptrs[client_idx], client_statistics_tracker_ptrs[client_idx]);
        client_params[client_idx] = client_param;
    }

    // (4.2) Launch clientcnt clients

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        pthread_returncode = pthread_create(&client_threads[client_idx], NULL, launchClient, (void*)(&(client_params[client_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch client " << client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // (5) Start benchmark and dump intermediate statistics

    // (5.1) Set client_running_ = true in all clientcnt client parameters to start benchmark

    covered::Util::dumpNormalMsg(main_class_name, "Start benchmark...");
    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        client_params[client_idx].setClientRunning();
    }

    // (5.2) Dump intermediate statistics

    const double duration = covered::Param::getDuration();
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

    // (6) Stop benchmark

    // (6.1) Reset client_running_ = false in clientcnt client parameters to stop benchmark

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        client_params[client_idx].resetClientRunning();
    }
    covered::Util::dumpNormalMsg(main_class_name, "Stop benchmark...");

    // (6.2) Wait for clientcnt clients

    covered::Util::dumpNormalMsg(main_class_name, "wait for all clients...");
    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        pthread_returncode = pthread_join(client_threads[client_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join client " << client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all clients are done");

    // (6.3) Reset edge_running_ = false in edgecnt edge parameters to stop edge nodes

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        edge_params[edge_idx].resetEdgeRunning();
    }
    covered::Util::dumpNormalMsg(main_class_name, "Stop edge nodes...");

    // (6.4) Wait for edgecnt edge nodes

    covered::Util::dumpNormalMsg(main_class_name, "wait for all edge nodes...");
    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        pthread_returncode = pthread_join(edge_threads[edge_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join edge node " << edge_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all edge nodes are done");

    // (6.5) Reset cloud_running_ = false in a cloud parameter to stop the cloud node

    cloud_param.resetCloudRunning();
    covered::Util::dumpNormalMsg(main_class_name, "Stop the cloud node...");

    // (6.6) Wait for the cloud node

    covered::Util::dumpNormalMsg(main_class_name, "wait for the cloud node...");
    pthread_returncode = pthread_join(cloud_thread, NULL); // void* retval = NULL
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to join cloud node (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }
    covered::Util::dumpNormalMsg(main_class_name, "the cloud node is done");

    // (7) Release variables in heap

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        if (workload_generator_ptrs[client_idx] != NULL)
        {
            delete workload_generator_ptrs[client_idx];
            workload_generator_ptrs[client_idx]= NULL;
        }
        if (client_statistics_tracker_ptrs[client_idx] != NULL)
        {
            delete client_statistics_tracker_ptrs[client_idx];
            client_statistics_tracker_ptrs[client_idx] = NULL;
        }
    }

    return 0;
}
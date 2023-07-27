/*
 * Simulate all componenets including client/edge/cloud nodes and evaluator in a single node.
 * 
 * By Siyuan Sheng (2023.07.27).
 */

#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <time.h> // struct timespec
#include <unistd.h> // usleep

#include "benchmark/client_wrapper.h"
#include "benchmark/evaluator_wrapper.h"
#include "common/cli.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cloud/cloud_wrapper.h"
#include "edge/edge_wrapper.h"
#include "benchmark/evaluator_wrapper.h"
#include "statistics/client_statistics_tracker.h"

int main(int argc, char **argv) {
    // (1) Parse and process CLI parameters (set configurations in Config and Param)
    covered::CLI::parseAndProcessCliParameters(argc, argv);
    const std::string main_class_name = covered::Param::getMainClassName();

    int pthread_returncode;

    // (2) Launch evaluator to control benchmark workflow

    pthread_t evaluator_thread;
    volatile bool is_evaluator_initialized = false;

    covered::Util::dumpNormalMsg(main_class_name, "launch evaluator");

    pthread_returncode = covered::Util::pthreadCreateHighPriority(&evaluator_thread, covered::EvaluatorWrapper::launchEvaluator, (void*)(&is_evaluator_initialized));
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to launch evaluator (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }

    // Block until evaluator is initialized
    while (!is_evaluator_initialized) {}

    // (2) Simulate a single cloud node for backend storage

    pthread_t cloud_thread;
    uint32_t cloud_idx;

    // (2.1) Prepare one cloud parameter

    cloud_idx = 0; // TODO: support 1 cloud node now

    // (2.2) Launch one cloud node

    covered::Util::dumpNormalMsg(main_class_name, "launch cloud node");

    //pthread_returncode = pthread_create(&cloud_thread, NULL, covered::CloudWrapper::launchCloud, (void*)(&(cloud_idx)));
    pthread_returncode = covered::Util::pthreadCreateHighPriority(&cloud_thread, covered::CloudWrapper::launchCloud, (void*)(&(cloud_idx)));
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
    uint32_t edge_idxes[edgecnt];

    // (3.1) Prepare edgecnt edge parameters

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        edge_idxes[edge_idx] = edge_idx;
    }

    // (3.2) Launch edgecnt edge nodes

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&edge_threads[edge_idx], NULL, covered::EdgeWrapper::launchEdge, (void*)(&(edge_idxes[edge_idx])));
        pthread_returncode = covered::Util::pthreadCreateLowPriority(&edge_threads[edge_idx], covered::EdgeWrapper::launchEdge, (void*)(&(edge_idxes[edge_idx])));
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
    uint32_t client_idxes[clientcnt];

    // (4.1) Prepare clientcnt client parameters

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        client_idxes[client_idx] = client_idx;
    }

    // (4.2) Launch clientcnt clients

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&client_threads[client_idx], NULL, covered::ClientWrapper::launchClient, (void*)(&(client_idxes[client_idx])));
        pthread_returncode = covered::Util::pthreadCreateLowPriority(&client_threads[client_idx], covered::ClientWrapper::launchClient, (void*)(&(client_idxes[client_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch client " << client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // (5) Start benchmark

    // (5.3) Start stresstest phase if all clients finish warmup phase

    // (6) Stop benchmark

    // (6.1) Reset client_running_ = false in clientcnt client parameters to stop benchmark

    //for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    //{
    //    client_params[client_idx].resetNodeRunning();
    //}
    //covered::Util::dumpNormalMsg(main_class_name, "Stop benchmark...");

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

    //for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    //{
    //    edge_params[edge_idx].resetNodeRunning();
    //}
    //covered::Util::dumpNormalMsg(main_class_name, "Stop edge nodes...");

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

    //cloud_param.resetNodeRunning();
    //covered::Util::dumpNormalMsg(main_class_name, "Stop the cloud node...");

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

    return 0;
}
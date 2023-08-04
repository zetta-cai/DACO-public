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
#include "common/cli/client_cli.h"
#include "common/cli/cloud_cli.h"
#include "common/cli/edge_cli.h"
#include "common/cli/evaluator_cli.h"
#include "common/config.h"
#include "common/util.h"
#include "cloud/cloud_wrapper.h"
#include "edge/edge_wrapper.h"
#include "benchmark/evaluator_wrapper.h"
#include "statistics/client_statistics_tracker.h"

int main(int argc, char **argv) {

    // (1) Parse and process different CLI parameters for client/edge/cloud/evaluator, and store them into Params

    covered::ClientCLI client_cli(argc, argv);
    covered::EdgeCLI edge_cli(argc, argv);
    covered::CloudCLI cloud_cli(argc, argv);
    covered::EvaluatorCLI evaluator_cli(argc, argv);

    const std::string main_class_name = client_cli.getMainClassName();

    int pthread_returncode;

    // (2) Launch evaluator to control benchmark workflow

    pthread_t evaluator_thread;
    covered::EvaluatorWrapperParam evaluator_param(false, &evaluator_cli);

    covered::Util::dumpNormalMsg(main_class_name, "launch evaluator");

    pthread_returncode = covered::Util::pthreadCreateHighPriority(&evaluator_thread, covered::EvaluatorWrapper::launchEvaluator, (void*)(&evaluator_param));
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to launch evaluator (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }

    // Block until evaluator is initialized
    while (!evaluator_param.isEvaluatorInitialized()) {}

    // (2) Simulate a single cloud node for backend storage

    pthread_t cloud_thread;
    covered::CloudWrapperParam cloud_param;

    // (2.1) Prepare one cloud parameter

    cloud_idx = 0; // TODO: support 1 cloud node now
    cloud_param = CloudWrapperParam(0, &cloud_cli);

    // (2.2) Launch one cloud node

    covered::Util::dumpNormalMsg(main_class_name, "launch cloud node");

    //pthread_returncode = pthread_create(&cloud_thread, NULL, covered::CloudWrapper::launchCloud, (void*)(&(cloud_idx)));
    pthread_returncode = covered::Util::pthreadCreateHighPriority(&cloud_thread, covered::CloudWrapper::launchCloud, (void*)(&(cloud_param)));
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to launch cloud node (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }

    // (3) Simulate edgecnt edge nodes with cooperative caching

    const uint32_t edgecnt = edge_cli.getEdgecnt();
    pthread_t edge_threads[edgecnt];
    covered::EdgeWrapperParam edge_params[edgecnt];

    // (3.1) Prepare edgecnt edge parameters

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        covered::EdgeWrapperParam tmp_edge_param(edge_idx, &edge_cli);
        edge_params[edge_idx] = tmp_edge_param;
    }

    // (3.2) Launch edgecnt edge nodes

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&edge_threads[edge_idx], NULL, covered::EdgeWrapper::launchEdge, (void*)(&(edge_idxes[edge_idx])));
        pthread_returncode = covered::Util::pthreadCreateLowPriority(&edge_threads[edge_idx], covered::EdgeWrapper::launchEdge, (void*)(&(edge_params[edge_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch edge node " << edge_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    
    // (4) Simulate clientcnt clients by multi-threading

    const uint32_t clientcnt = client_cli.getClientcnt();
    pthread_t client_threads[clientcnt];
    covered::ClientWrapperParam client_params[clientcnt];

    // (4.1) Prepare clientcnt client parameters

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        covered::ClientWrapperParam tmp_client_param(client_idx, &client_cli);
        client_params[client_idx] = tmp_client_param;
    }

    // (4.2) Launch clientcnt clients

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&client_threads[client_idx], NULL, covered::ClientWrapper::launchClient, (void*)(&(client_idxes[client_idx])));
        pthread_returncode = covered::Util::pthreadCreateLowPriority(&client_threads[client_idx], covered::ClientWrapper::launchClient, (void*)(&(client_params[client_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch client " << client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // (5) Wait for all threads

    // (5.1) Wait for clientcnt clients

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

    // (5.2) Wait for edgecnt edge nodes

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

    // (5.3) Wait for the cloud node

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
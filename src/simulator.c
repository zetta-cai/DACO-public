/*
 * Simulate all componenets including client/edge/cloud nodes and evaluator in the current physical machine.
 *
 * NOTE: clients/edges/cloud/evaluator MUST be in the same physical machine in config.json.
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
#include "cli/simulator_cli.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "cloud/cloud_wrapper.h"
#include "edge/edge_wrapper.h"
#include "benchmark/evaluator_wrapper.h"
#include "statistics/client_statistics_tracker.h"

int main(int argc, char **argv) {

    // (1) Parse and process different CLI parameters for client/edge/cloud/evaluator

    covered::SimulatorCLI simulator_cli(argc, argv);

    // Bind main thread of simulator to a shared CPU core
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // Validate thread launcher before launching threads
    covered::ThreadLauncher::validate(main_class_name, simulator_cli.getClientcnt(), simulator_cli.getEdgecnt());

    // (2) Launch evaluator to control benchmark workflow

    pthread_t evaluator_thread;
    covered::EvaluatorWrapperParam evaluator_param(false, (covered::EvaluatorCLI*)&simulator_cli);

    covered::Util::dumpNormalMsg(main_class_name, "launch evaluator");

    std::string tmp_thread_name = "evaluator-wrapper";
    covered::ThreadLauncher::pthreadCreateHighPriority(covered::ThreadLauncher::EVALUATOR_THREAD_ROLE, tmp_thread_name, &evaluator_thread, covered::EvaluatorWrapper::launchEvaluator, (void*)(&evaluator_param));
    // if (pthread_returncode != 0)
    // {
    //     std::ostringstream oss;
    //     oss << "failed to launch evaluator (error code: " << pthread_returncode << ")";
    //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
    //     exit(1);
    // }

    // Block until evaluator is initialized
    while (!evaluator_param.isEvaluatorInitialized()) {}

    covered::Util::dumpNormalMsg(main_class_name, "Evaluator initialized"); // NOTE: used by exp scripts to verify whether the evaluator has been initialized

    // (2) Simulate a single cloud node for backend storage

    pthread_t cloud_thread;
    covered::CloudWrapperParam cloud_param;

    // (2.1) Prepare one cloud parameter

    const uint32_t cloud_idx = 0; // TODO: support 1 cloud node now
    cloud_param = covered::CloudWrapperParam(cloud_idx, (covered::CloudCLI*)&simulator_cli);

    // (2.2) Launch one cloud node

    covered::Util::dumpNormalMsg(main_class_name, "launch cloud node");

    //pthread_returncode = pthread_create(&cloud_thread, NULL, covered::CloudWrapper::launchCloud, (void*)(&(cloud_param)));
    // if (pthread_returncode != 0)
    // {
    //     std::ostringstream oss;
    //     oss << "failed to launch cloud node (error code: " << pthread_returncode << ")";
    //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
    //     exit(1);
    // }
    tmp_thread_name = "cloud-wrapper-" + std::to_string(cloud_idx);
    covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cloud_thread, covered::CloudWrapper::launchCloud, (void*)(&(cloud_param)));

    // (3) Simulate edgecnt edge nodes with cooperative caching

    const uint32_t edgecnt = simulator_cli.getEdgecnt();
    pthread_t edge_threads[edgecnt];
    covered::EdgeWrapperParam edge_params[edgecnt];

    // (3.1) Prepare edgecnt edge parameters

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        covered::EdgeWrapperParam tmp_edge_param(edge_idx, (covered::EdgeCLI*)&simulator_cli);
        edge_params[edge_idx] = tmp_edge_param;
    }

    // (3.2) Launch edgecnt edge nodes

    for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&edge_threads[edge_idx], NULL, covered::EdgeWrapper::launchEdge, (void*)(&(edge_params[edge_idx])));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "failed to launch edge node " << edge_idx << " (error code: " << pthread_returncode << ")";
        //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
        //     exit(1);
        // }

        tmp_thread_name = "edge-wrapper-" + std::to_string(edge_idx);
        covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &edge_threads[edge_idx], covered::EdgeWrapper::launchEdge, (void*)(&(edge_params[edge_idx])));
    }
    
    // (4) Simulate clientcnt clients by multi-threading

    const uint32_t clientcnt = simulator_cli.getClientcnt();
    pthread_t client_threads[clientcnt];
    covered::ClientWrapperParam client_params[clientcnt];

    // (4.1) Prepare clientcnt client parameters

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        covered::ClientWrapperParam tmp_client_param(client_idx, (covered::ClientCLI*)&simulator_cli);
        client_params[client_idx] = tmp_client_param;
    }

    // (4.2) Launch clientcnt clients

    for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&client_threads[client_idx], NULL, covered::ClientWrapper::launchClient, (void*)(&(client_params[client_idx])));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "failed to launch client " << client_idx << " (error code: " << pthread_returncode << ")";
        //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "client-wrapper-" + std::to_string(client_idx);
        covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &client_threads[client_idx], covered::ClientWrapper::launchClient, (void*)(&(client_params[client_idx])));
    }

    // (5) Wait for all threads

    int pthread_returncode = 0;

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

    // (5.4) Wait for the evaluator

    covered::Util::dumpNormalMsg(main_class_name, "wait for the evaluator...");
    pthread_returncode = pthread_join(evaluator_thread, NULL); // void* retval = NULL
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to join evaluator (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }
    covered::Util::dumpNormalMsg(main_class_name, "the evaluator is done");

    return 0;
}
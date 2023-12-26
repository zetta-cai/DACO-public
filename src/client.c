/*
 * Launch a client node in a physical machine.
 * 
 * By Siyuan Sheng (2023.12.26).
 */

#include <pthread.h>

#include "benchmark/client_worker_param.h"
#include "benchmark/client_wrapper.h"
#include "cli/client_cli.h"
#include "common/config.h"
#include "common/thread_launcher.h"

int main(int argc, char **argv) {

    // (1) Parse and process client CLI parameters

    covered::ClientCLI client_cli(argc, argv);

    // Bind main thread of simulator to a shared CPU core
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // (2) Simulate current_machine_clientcnt clients by multi-threading

    // Get client idx ranges in the current physical machine
    const uint32_t total_clientcnt = client_cli.getClientcnt();
    uint32_t left_inclusive_client_idx = 0;
    uint32_t right_inclusive_client_idx = 0;
    covered::Config::getCurrentMachineClientIdxRange(total_clientcnt, left_inclusive_client_idx, right_inclusive_client_idx);
    assert(right_inclusive_client_idx >= left_inclusive_client_idx);
    assert(right_inclusive_client_idx < total_clientcnt);

    // Prepare pthread_t and parameters for clients
    uint32_t current_machine_clientcnt = right_inclusive_client_idx - left_inclusive_client_idx + 1;
    assert(current_machine_clientcnt <= total_clientcnt);
    pthread_t current_machine_client_threads[current_machine_clientcnt];
    covered::ClientWrapperParam current_machine_client_params[current_machine_clientcnt];

    // (2.1) Prepare current_machine_clientcnt client parameters

    for (uint32_t client_idx = left_inclusive_client_idx; client_idx <= right_inclusive_client_idx; client_idx++)
    {
        covered::ClientWrapperParam tmp_client_param(client_idx, &client_cli);
        current_machine_client_params[client_idx - left_inclusive_client_idx] = tmp_client_param;
    }

    // (2.2) Launch current_machine_clientcnt clients

    for (uint32_t client_idx = left_inclusive_client_idx; client_idx <= right_inclusive_client_idx; client_idx++)
    {
        std::ostringstream oss;
        oss << "launch client " << client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&current_machine_client_threads[client_idx - left_inclusive_client_idx], NULL, covered::ClientWrapper::launchClient, (void*)(&(current_machine_client_params[client_idx - left_inclusive_client_idx])));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "failed to launch client " << client_idx << " (error code: " << pthread_returncode << ")";
        //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "client-wrapper-" + std::to_string(client_idx);
        covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &current_machine_client_threads[client_idx - left_inclusive_client_idx], covered::ClientWrapper::launchClient, (void*)(&(current_machine_client_params[client_idx - left_inclusive_client_idx])));
    }

    // (3) Wait for current_machine_clientcnt client threads

    int pthread_returncode = 0;

    covered::Util::dumpNormalMsg(main_class_name, "wait for all clients...");
    for (uint32_t client_idx = left_inclusive_client_idx; client_idx <= right_inclusive_client_idx; client_idx++)
    {
        pthread_returncode = pthread_join(current_machine_client_threads[client_idx - left_inclusive_client_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join client " << client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all clients are done");

    return 0;
}
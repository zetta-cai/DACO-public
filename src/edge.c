/*
 * Launch edge node(s) in the current physical machine.
 * 
 * By Siyuan Sheng (2023.12.27).
 */

#include <pthread.h>

#include "cli/edge_cli.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "edge/edge_wrapper.h"

int main(int argc, char **argv) {

    // (1) Parse and process edge CLI parameters

    covered::EdgeCLI edge_cli(argc, argv);

    // Bind main thread of edge(s) to a shared CPU core
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // (2) Simulate current_machine_edgecnt edges by multi-threading

    // Get edge idx ranges in the current physical machine
    const uint32_t total_edgecnt = edge_cli.getEdgecnt();
    uint32_t left_inclusive_edge_idx = 0;
    uint32_t right_inclusive_edge_idx = 0;
    covered::Config::getCurrentMachineEdgeIdxRange(total_edgecnt, left_inclusive_edge_idx, right_inclusive_edge_idx);
    assert(right_inclusive_edge_idx >= left_inclusive_edge_idx);
    assert(right_inclusive_edge_idx < total_edgecnt);

    // Prepare pthread_t and parameters for edges
    uint32_t current_machine_edgecnt = right_inclusive_edge_idx - left_inclusive_edge_idx + 1;
    assert(current_machine_edgecnt <= total_edgecnt);
    pthread_t current_machine_edge_threads[current_machine_edgecnt];
    covered::EdgeWrapperParam current_machine_edge_params[current_machine_edgecnt];

    // (2.1) Prepare current_machine_edgecnt edge parameters

    for (uint32_t edge_idx = left_inclusive_edge_idx; edge_idx <= right_inclusive_edge_idx; edge_idx++)
    {
        covered::EdgeWrapperParam tmp_edge_param(edge_idx, &edge_cli);
        current_machine_edge_params[edge_idx - left_inclusive_edge_idx] = tmp_edge_param;
    }

    // (2.2) Launch current_machine_edgecnt edge nodes

    for (uint32_t edge_idx = left_inclusive_edge_idx; edge_idx <= right_inclusive_edge_idx; edge_idx++)
    {
        std::ostringstream oss;
        oss << "launch edge node " << edge_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        //pthread_returncode = pthread_create(&current_machine_edge_threads[edge_idx - left_inclusive_edge_idx], NULL, covered::EdgeWrapper::launchEdge, (void*)(&(current_machine_edge_params[edge_idx - left_inclusive_edge_idx])));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "failed to launch edge node " << edge_idx << " (error code: " << pthread_returncode << ")";
        //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
        //     exit(1);
        // }

        std::string tmp_thread_name = "edge-wrapper-" + std::to_string(edge_idx);
        covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &current_machine_edge_threads[edge_idx - left_inclusive_edge_idx], covered::EdgeWrapper::launchEdge, (void*)(&(current_machine_edge_params[edge_idx - left_inclusive_edge_idx])));
    }

    // (3) Wait for current_machine_edgecnt edge threads

    int pthread_returncode = 0;

    covered::Util::dumpNormalMsg(main_class_name, "wait for all edges...");
    for (uint32_t edge_idx = left_inclusive_edge_idx; edge_idx <= right_inclusive_edge_idx; edge_idx++)
    {
        pthread_returncode = pthread_join(current_machine_edge_threads[edge_idx - left_inclusive_edge_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join edge " << edge_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all edges are done");

    return 0;
}
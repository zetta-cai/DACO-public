#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <time.h> // struct timespec
#include <unistd.h> // usleep

#include "common/cli.h"
#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "statistics/client_statistics_tracker.h"
#include "statistics/total_statistics_tracker.h"

struct LoaderParam
{
    uint32_t global_client_idx;
    covered::ClientStatisticsTracker* client_statistics_tracker_ptr;
};

void* launchLoader(void* local_loader_param_ptr)
{
    assert(local_loader_param_ptr != NULL);
    LoaderParam& local_loader_param = *((LoaderParam*)local_loader_param_ptr);
    assert(local_loader_param.client_statistics_tracker_ptr == NULL);

    std::string client_statistics_filepath = covered::Util::getClientStatisticsFilepath(local_loader_param.global_client_idx);
    local_loader_param.client_statistics_tracker_ptr = new covered::ClientStatisticsTracker(client_statistics_filepath); // Release outside launchLoader()
    assert(local_loader_param.client_statistics_tracker_ptr != NULL);
    
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {
    std::string main_class_name = "statistics_aggregator";

    // (1) Parse and process CLI parameters (set configurations in Config and Param)
    covered::CLI::parseAndProcessCliParameters(main_class_name, argc, argv);

    int pthread_returncode;

    // TODO: (2) Transfer per-client statistics files from all client machines into the current machine for prototype mode
    if (!covered::Param::isSimulation())
    {
        // TODO
    }

    // (3) Launch clientcnt loaders to load per-client statistics

    const uint32_t clientcnt = covered::Param::getClientcnt();
    pthread_t loader_threads[clientcnt];
    LoaderParam loader_params[clientcnt];

    // (3.1) Prepare loader parameters
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx)
    {
        loader_params[global_client_idx].global_client_idx = global_client_idx;
        loader_params[global_client_idx].client_statistics_tracker_ptr = NULL;
    }

    // (3.2) Launch clientcnt loaders

    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        std::ostringstream oss;
        oss << "launch loader " << global_client_idx;
        covered::Util::dumpNormalMsg(main_class_name, oss.str());

        pthread_returncode = pthread_create(&loader_threads[global_client_idx], NULL, launchLoader, (void*)(&(loader_params[global_client_idx])));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to launch loader " << global_client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // (4) Aggregate and dump per-client statistics

    // (4.1) Prepare parameters for TotalStatisticsTracker
    covered::ClientStatisticsTracker* client_statistics_tracker_ptrs[clientcnt];
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        client_statistics_tracker_ptrs[global_client_idx] = loader_params[global_client_idx].client_statistics_tracker_ptr;
        assert(client_statistics_tracker_ptrs[global_client_idx] != NULL);
    }

    // (4.2) Aggregate and dump per-client statistics
    covered::TotalStatisticsTracker total_statistics_tracker(clientcnt, client_statistics_tracker_ptrs);
    std::string total_statistics_string = total_statistics_tracker.toString();
    covered::Util::dumpDebugMsg(main_class_name, total_statistics_string);

    // (5) Wait for clientcnt loaders
    covered::Util::dumpNormalMsg(main_class_name, "wait for all loaders...");
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        pthread_returncode = pthread_join(loader_threads[global_client_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join loader " << global_client_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all loaders are done");

    // (6) Release variables in heap
    for (uint32_t global_client_idx = 0; global_client_idx < clientcnt; global_client_idx++)
    {
        assert(loader_params[global_client_idx].client_statistics_tracker_ptr != NULL);
        delete loader_params[global_client_idx].client_statistics_tracker_ptr;
        loader_params[global_client_idx].client_statistics_tracker_ptr = NULL;
    }

    return 0;
}
/*
 * Launch a cloud node in the current physical machine.
 * 
 * By Siyuan Sheng (2023.12.27).
 */

#include <pthread.h>

#include "cli/cloud_cli.h"
#include "cloud/cloud_wrapper.h"
#include "common/config.h"
#include "common/thread_launcher.h"

int main(int argc, char **argv) {

    // (1) Parse and process cloud CLI parameters

    covered::CloudCLI cloud_cli(argc, argv);

    // Bind main thread of cloud to a shared CPU core
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // (2) Simulate a single cloud node for backend storage

    pthread_t cloud_thread;
    covered::CloudWrapperParam cloud_param;

    // (2.1) Prepare one cloud parameter

    const uint32_t cloud_idx = 0; // TODO: support 1 cloud node now
    cloud_param = covered::CloudWrapperParam(cloud_idx, &cloud_cli);

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
    std::string tmp_thread_name = "cloud-wrapper-" + std::to_string(cloud_idx);
    covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cloud_thread, covered::CloudWrapper::launchCloud, (void*)(&(cloud_param)));

    // (3) Wait for the cloud node

    int pthread_returncode = 0;

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
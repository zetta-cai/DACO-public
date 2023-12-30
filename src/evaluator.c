/*
 * Launch an evaluator node in the current physical machine.
 * 
 * By Siyuan Sheng (2023.12.27).
 */

#include <pthread.h>

#include "cli/evaluator_cli.h"
#include "benchmark/evaluator_wrapper.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"

int main(int argc, char **argv) {

    // (1) Parse and process evaluator CLI parameters

    covered::EvaluatorCLI evaluator_cli(argc, argv);

    // Validate thread launcher before launching threads
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::validate(main_class_name, evaluator_cli.getClientcnt(), evaluator_cli.getEdgecnt());

    // Bind main thread of evaluator to a shared CPU core
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // (2) Launch evaluator to control benchmark workflow

    pthread_t evaluator_thread;
    covered::EvaluatorWrapperParam evaluator_param(false, &evaluator_cli);

    covered::Util::dumpNormalMsg(main_class_name, "launch evaluator");

    std::string tmp_thread_name = "evaluator-wrapper";
    // covered::ThreadLauncher::pthreadCreateHighPriority(covered::ThreadLauncher::EVALUATOR_THREAD_ROLE, tmp_thread_name, &evaluator_thread, covered::EvaluatorWrapper::launchEvaluator, (void*)(&evaluator_param));
    covered::ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &evaluator_thread, covered::EvaluatorWrapper::launchEvaluator, (void*)(&evaluator_param)); // TMPDEBUG231229
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

    // (3) Wait for the evaluator

    int pthread_returncode = 0;

    covered::Util::dumpNormalMsg(main_class_name, "wait for the evaluator...");
    pthread_returncode = pthread_join(evaluator_thread, NULL); // void* retval = NULL
    if (pthread_returncode != 0)
    {
        std::ostringstream oss;
        oss << "failed to join evaluator (error code: " << pthread_returncode << ")";
        covered::Util::dumpErrorMsg(main_class_name, oss.str());
        exit(1);
    }
    covered::Util::dumpNormalMsg(main_class_name, "Evaluator done"); // NOTE: used by exp scripts to verify whether evaluation has been done

    return 0;
}
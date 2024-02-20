/*
 * Preprocess replayed traces to get trace properties including keycnt, total opcnt, and dataset capacity size (for cache capacity calculation in evaluation).
 *
 * NOTE: you should update corresponding entires in config.json for replayed traces after preprocessing.
 * 
 * By Siyuan Sheng (2024.02.17).
 */

#include <sstream>
#include <unistd.h>

#include "cli/trace_preprocessor_cli.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "workload/workload_wrapper_base.h"

int main(int argc, char **argv) {
    // (1) Parse and process CLI parameters

    covered::TracePreprocessorCLI trace_preprocessor_cli(argc, argv);

    // Validate thread launcher before launching threads
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::validate(main_class_name);

    // Bind main thread of trace preprocessor to a shared CPU core
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    const uint32_t keycnt = trace_preprocessor_cli.getKeycnt();
    assert(keycnt == 0); // keycnt MUST be 0 for trace preprocessor; keycnt will NOT be used by replayed traces
    const std::string workload_name = trace_preprocessor_cli.getWorkloadName();
    assert(covered::Util::isReplayedWorkload(workload_name));

    // (2) Preprocess trace files by workload wrapper

    std::ostringstream oss;
    oss << "preprocess replayed trace files for workload " << workload_name << "...";
    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    // Create a workload wrapper
    // NOTE: NOT need client CLI parameters as we only use dataset key-value pairs instead of workload items
    const uint32_t clientcnt = 1;
    const uint32_t client_idx = 0;
    const uint32_t perclient_workercnt = 1;
    const uint32_t perclient_opcnt = 0; // NOTE: perclient_opcnt will NOT be used by replayed traces
    const bool is_loading_phase = true; // Track dataset items instead of workload items (yet still track total opcnt)
    covered::WorkloadWrapperBase* workload_generator_ptr = covered::WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, perclient_opcnt, perclient_workercnt, workload_name, is_loading_phase);
    assert(workload_generator_ptr != NULL);

    // (3) Dump workload properties

    oss.clear();
    oss.str("");

    const uint32_t dataset_keycnt = workload_generator_ptr->getPracticalKeycnt();
    oss << "unique key count (dataset size): " << dataset_keycnt << "; total opcnt of all clients (workload size): " << workload_generator_ptr->getTotalOpcnt() << std::endl;

    const uint32_t avg_dataset_keysize = workload_generator_ptr->getAvgDatasetKeysize();
    const uint32_t avg_dataset_valuesize = workload_generator_ptr->getAvgDatasetValuesize();
    const uint64_t dataset_capacity_size = (static_cast<uint64_t>(avg_dataset_keysize) + static_cast<uint64_t>(avg_dataset_valuesize)) * static_cast<uint64_t>(dataset_keycnt);
    const uint32_t dataset_capacity_mb = static_cast<uint32_t>(B2MB(dataset_capacity_size));
    oss << "average dataset key size: " << avg_dataset_keysize << "; average dataset value size: " << avg_dataset_valuesize << "; dataset_capacity_mb: " << dataset_capacity_mb << std::endl;

    oss << "min dataset key size: " << workload_generator_ptr->getMinDatasetKeysize() << "; min dataset value size: " << workload_generator_ptr->getMinDatasetValuesize() << std::endl;
    oss << "max dataset key size: " << workload_generator_ptr->getMaxDatasetKeysize() << "; max dataset value size: " << workload_generator_ptr->getMaxDatasetValuesize();

    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    // (6) Release variables

    assert(workload_generator_ptr != NULL);
    delete workload_generator_ptr;
    workload_generator_ptr = NULL;

    // TODO: Uncomment the following code if we launch trace preprocessor by python scripts
    // // NOTE: used by exp scripts to verify whether trace preprocessor has finished -> MUST be the same as scripts/common.py
    // const std::string TRACE_PREPROCESSOR_FINISH_SYMBOL = "Trace preprocessor done";
    // covered::Util::dumpNormalMsg(main_class_name, TRACE_PREPROCESSOR_FINISH_SYMBOL);

    return 0;
}
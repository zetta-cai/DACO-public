/*
 * Load dataset of a given workload into the current cloud node offline.
 *
 * NOTE: due to per-level compression in RocksDB, the actual size of dataset may be far smaller than keycnt * avg_kv_size -> while it does NOT affect the results of cooperative edge caching, as the bottleneck is edge-cloud network communication instead of cloud-side RocksDB accesses.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#include <sstream>
#include <unistd.h>

#include "cloud/rocksdb_wrapper.h"
#include "cli/dataset_loader_cli.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

struct DatasetLoaderParam
{
    uint32_t dataset_loader_idx;
    uint32_t dataset_loadercnt;
    covered::RocksdbWrapper* rocksdb_wrapper_ptr;
    covered::WorkloadWrapperBase* workload_generator_ptr;
};

void* launchDatasetLoader(void* dataset_loader_param_ptr)
{
    assert(dataset_loader_param_ptr != NULL);
    DatasetLoaderParam& dataset_loader_param = *((DatasetLoaderParam*)dataset_loader_param_ptr);

    // Calculate the key index range [start, end) assigned to the current dataset loader
    const uint32_t practical_keycnt = dataset_loader_param.workload_generator_ptr->getPracticalKeycnt();
    assert(dataset_loader_param.dataset_loadercnt != 0);
    const uint32_t perloader_keycnt = practical_keycnt / dataset_loader_param.dataset_loadercnt;
    assert(perloader_keycnt > 0);
    const uint32_t start_keyidx = perloader_keycnt * dataset_loader_param.dataset_loader_idx;
    assert(start_keyidx < practical_keycnt);
    uint32_t end_keyidx = 0;
    if (dataset_loader_param.dataset_loader_idx == dataset_loader_param.dataset_loadercnt - 1) // tail
    {
        end_keyidx = practical_keycnt;
    }
    else
    {
        end_keyidx = start_keyidx + perloader_keycnt;
    }
    assert(end_keyidx <= practical_keycnt);
    assert(end_keyidx > start_keyidx);

    std::ostringstream oss;
    oss << "loader " << dataset_loader_param.dataset_loader_idx << " loads dataset item indexes [" << start_keyidx << ", " << end_keyidx << ") into RocksDB KVS...";
    covered::Util::dumpNormalMsg("dataset loader", oss.str());
    
    // Store partial dataset into RocksDB
    uint64_t tmp_total_key_bytes = 0;
    uint64_t tmp_total_value_bytes = 0;
    for (uint32_t itemidx = start_keyidx; itemidx < end_keyidx; itemidx++)
    {
        covered::WorkloadItem dataset_item = dataset_loader_param.workload_generator_ptr->getDatasetItem(itemidx);
        assert(dataset_item.getItemType() == covered::WorkloadItemType::kWorkloadItemPut);

        dataset_loader_param.rocksdb_wrapper_ptr->put(dataset_item.getKey(), dataset_item.getValue());

        tmp_total_key_bytes += dataset_item.getKey().getKeyLength();
        tmp_total_value_bytes += dataset_item.getValue().getValuesize();
    }
    double tmp_avg_key_bytes = (double)tmp_total_key_bytes / (double)(end_keyidx - start_keyidx);
    double tmp_avg_value_bytes = (double)tmp_total_value_bytes / (double)(end_keyidx - start_keyidx);
    oss.clear();
    oss.str("");
    oss << "loader " << dataset_loader_param.dataset_loader_idx << " finishes loading dataset item indexes [" << start_keyidx << ", " << end_keyidx << ") into RocksDB KVS (average key size: " << tmp_avg_key_bytes << " bytes, average value size: " << tmp_avg_value_bytes << " bytes)";
    covered::Util::dumpNormalMsg("dataset loader", oss.str());
    
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {

    // (1) Parse and process CLI parameters and store them into DatasetLoaderParam

    covered::DatasetLoaderCLI dataset_loader_cli(argc, argv);

    // Bind main thread of dataset loader to a shared CPU core
    const std::string main_class_name = covered::Config::getMainClassName();
    covered::ThreadLauncher::bindMainThreadToSharedCpuCore(main_class_name);

    // Validate thread launcher before launching threads
    covered::ThreadLauncher::validate(main_class_name);

    const uint32_t cloud_idx = dataset_loader_cli.getCloudIdx();
    const uint32_t dataset_loadercnt = dataset_loader_cli.getDatasetLoadercnt();
    const uint32_t keycnt = dataset_loader_cli.getKeycnt();
    const std::string workload_name = dataset_loader_cli.getWorkloadName();

    // (2) Prepare dataset loader parameters

    std::ostringstream oss;
    oss << "launch dataset loaders for cloud " << cloud_idx << "...";
    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    // Create the corresponding RocksDB KVS
    covered::RocksdbWrapper* rocksdb_wrapper_ptr = new covered::RocksdbWrapper(cloud_idx, dataset_loader_cli.getCloudStorage(), covered::Util::getCloudRocksdbDirpath(keycnt, workload_name, cloud_idx));
    assert(rocksdb_wrapper_ptr != NULL);

    // Create a workload generator
    // NOTE: NOT need client CLI parameters as we only use dataset key-value pairs instead of workload items
    const uint32_t clientcnt = 1;
    const uint32_t client_idx = 0;
    const uint32_t perclient_workercnt = dataset_loadercnt;
    const uint32_t opcnt = clientcnt * perclient_workercnt; // dataset_loadercnt
    covered::WorkloadWrapperBase* workload_generator_ptr = covered::WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, opcnt, perclient_workercnt, workload_name);
    covered::Util::dumpVariablesForDebug(main_class_name, 4, "average dataset key size:", std::to_string(workload_generator_ptr->getAvgDatasetKeysize()).c_str(), "average dataset value size:", std::to_string(workload_generator_ptr->getAvgDatasetValuesize()).c_str());
    covered::Util::dumpVariablesForDebug(main_class_name, 4, "min dataset key size:", std::to_string(workload_generator_ptr->getMinDatasetKeysize()).c_str(), "min dataset value size:", std::to_string(workload_generator_ptr->getMinDatasetValuesize()).c_str());
    covered::Util::dumpVariablesForDebug(main_class_name, 4, "max dataset key size:", std::to_string(workload_generator_ptr->getMaxDatasetKeysize()).c_str(), "max dataset value size:", std::to_string(workload_generator_ptr->getMaxDatasetValuesize()).c_str());
    assert(workload_generator_ptr != NULL);

    // Create dataset loader parameters
    struct DatasetLoaderParam dataset_loader_params[dataset_loadercnt];
    for (uint32_t dataset_loader_idx = 0; dataset_loader_idx < dataset_loadercnt; dataset_loader_idx++)
    {
        struct DatasetLoaderParam& tmp_dataset_loader_param = dataset_loader_params[dataset_loader_idx];
        tmp_dataset_loader_param.dataset_loader_idx = dataset_loader_idx;
        tmp_dataset_loader_param.dataset_loadercnt = dataset_loadercnt;
        tmp_dataset_loader_param.rocksdb_wrapper_ptr = rocksdb_wrapper_ptr;
        tmp_dataset_loader_param.workload_generator_ptr = workload_generator_ptr;
    }

    // (3) Launch dataset loaders for loading phase
    covered::Util::dumpNormalMsg(main_class_name, "launch dataset loaders...");
    pthread_t dataset_loader_threads[dataset_loadercnt];
    for (uint32_t dataset_loader_idx = 0; dataset_loader_idx < dataset_loadercnt; dataset_loader_idx++)
    {
        std::string tmp_thread_name = "dataset-loader-" + std::to_string(dataset_loader_idx);
        covered::ThreadLauncher::pthreadCreateHighPriority(covered::ThreadLauncher::DATASET_LOADER_THREAD_ROLE, tmp_thread_name, &dataset_loader_threads[dataset_loader_idx], launchDatasetLoader, (void*)(&dataset_loader_params[dataset_loader_idx]));
        // if (pthread_returncode != 0)
        // {
        //     oss.clear();
        //     oss.str("");
        //     oss << "failed to launch dataset loader (error code: " << pthread_returncode << ")";
        //     covered::Util::dumpErrorMsg(main_class_name, oss.str());
        //     exit(1);
        // }
    }

    // (4) Wait for dataset loaders
    int pthread_returncode = 0;
    covered::Util::dumpNormalMsg(main_class_name, "wait for all dataset loaders...");
    for (uint32_t dataset_loader_idx = 0; dataset_loader_idx < dataset_loadercnt; dataset_loader_idx++)
    {
        pthread_returncode = pthread_join(dataset_loader_threads[dataset_loader_idx], NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "failed to join dataset loader " << dataset_loader_idx << " (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }
    covered::Util::dumpNormalMsg(main_class_name, "all dataset loaders are done");

    // (5) Sleep for compaction

    oss.clear();
    oss.str("");
    uint32_t sleep_for_compaction_sec = covered::Config::getDatasetLoaderSleepForCompactionSec();
    oss << "sleep " << sleep_for_compaction_sec << " seconds to wait for compaction...";
    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    sleep(sleep_for_compaction_sec);

    covered::Util::dumpNormalMsg(main_class_name, "finish sleep for compaction!");

    // (6) Release variables

    assert(workload_generator_ptr != NULL);
    delete workload_generator_ptr;
    workload_generator_ptr = NULL;

    // Close Rocksdb KVS
    assert(rocksdb_wrapper_ptr != NULL);
    delete rocksdb_wrapper_ptr;
    rocksdb_wrapper_ptr = NULL;

    return 0;
}
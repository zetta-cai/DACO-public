/*
 * Load dataset of a given workload into the current cloud node offline.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#include <sstream>

#include "cloud/rocksdb_wrapper.h"
#include "common/cli/dataset_loader_cli.h"
#include "common/param/cloud_param.h"
#include "common/param/common_param.h"
#include "common/param/dataset_loader_param.h"
#include "common/param/workload_param.h"
#include "common/util.h"
#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

struct DatasetLoaderParam
{
    uint32_t dataset_loader_idx;
    uint32_t dataset_loadercnt;
    covered::RocksdbWrapper* rocksdb_wrapper_ptr;
    covered::WorkloadWrapperBase* workload_generator_ptr;
}

void* launchDatasetLoader(void* dataset_loader_param_ptr)
{
    assert(dataset_loader_param_ptr != NULL);
    covered::DatasetLoaderParam& dataset_loader_param = *((covered::DatasetLoaderParam*)dataset_loader_param_ptr);

    // Calculate the key index range [start, end) assigned to the current dataset loader
    const uint32_t keycnt = dataset_loader_param.workload_generator_ptr->getKeycnt();
    assert(dataset_loader_param.dataset_loadercnt != 0);
    const uint32_t perloader_keycnt = keycnt / dataset_loader_param.dataset_loadercnt;
    assert(perloader_keycnt > 0);
    const uint32_t start_keyidx = perloader_keycnt * dataset_loader_param.dataset_loader_idx;
    assert(start_keyidx < keycnt);
    uint32_t end_keyidx = 0;
    if (dataset_loader_param.dataset_loader_idx == dataset_loader_param.dataset_loadercnt - 1) // tail
    {
        end_keyidx = keycnt;
    }
    else
    {
        end_keyidx = start_keyidx + perloader_keycnt;
    }
    assert(end_keyidx <= keycnt);
    assert(end_keyidx > start_keyidx);

    std::ostringstream oss;
    oss << "loader " << dataset_loader_param.dataset_loader_idx << " loads dataset item indexes [" << start_keyidx << ", " << end_keyidx << " ) into RocksDB KVS...";
    covered::Util::dumpNormalMsg("dataset loader", oss.str());
    
    // Store partial dataset into RocksDB
    for (uint32_t itemidx = start_keyidx; itemidx < end_keyidx; itemidx++)
    {
        covered::WorkloadItem dataset_item = dataset_loader_param.workload_generator_ptr->getDatasetItem(itemidx);
        assert(dataset_item.getItemType() == covered::WorkloadItemType::kWorkloadItemPut);

        dataset_loader_param.rocksdb_wrapper_ptr->put(dataset_item.getKey(), dataset_item.getValue());
    }
    
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {

    // (1) Parse and process CLI parameters and store them into DatasetLoaderParam

    covered::DatasetLoaderCLI dataset_loader_cli(argc, argv);

    const std::string main_class_name = covered::CommonParam::getMainClassName();

    const uint32_t cloud_idx = covered::DatasetLoaderParam::getCloudIdx();
    const uint32_t dataset_loadercnt = covered::DatasetLoaderParam::getDatasetLoadercnt();
    const uint32_t keycnt = covered::WorkloadParam::getKeycnt();
    const std::string workload_name = covered::WorkloadParam::getWorkloadName();

    // (2) Prepare dataset loader parameters

    std::ostringstream oss;
    oss << "launch dataset loaders for cloud " << cloud_idx << "...";
    covered::Util::dumpNormalMsg(main_class_name, oss.str());

    // Create the corresponding RocksDB KVS
    covered::RocksdbWrapper rocksdb_wrapper(cloud_idx, covered::CloudParam::getCloudStorage(), covered::Util::getCloudRocksdbDirpath(cloud_idx));

    // Create a workload generator
    // NOTE: NOT need client CLI parameters as we only use dataset key-value pairs instead of workload items
    const uint32_t clientcnt = 1;
    const uint32_t client_idx = 0;
    const uint32_t perclient_workercnt = dataset_loadercnt;
    const uint32_t opcnt = clientcnt * perclient_workercnt; // dataset_loadercnt
    covered::WorkloadWrapperBase* workload_generator_ptr = covered::WorkloadWrapperBase::getWorkloadGeneratorByWorkloadName(clientcnt, client_idx, keycnt, opcnt, perclient_workercnt, workload_name)
    assert(workload_generator_ptr != NULL);

    // Create dataset loader parameters
    struct DatasetLoaderParam dataset_loader_params[dataset_loadercnt];
    for (uint32_t dataset_loader_idx = 0; dataset_loader_idx < dataset_loadercnt; dataset_loader_idx++)
    {
        struct DatasetLoaderParam& tmp_dataset_loader_param = dataset_loader_params[dataset_loader_idx];
        tmp_dataset_loader_param.dataset_loader_idx = dataset_loader_idx;
        tmp_dataset_loader_param.dataset_loadercnt = dataset_loadercnt;
        tmp_dataset_loader_param.rocksdb_wrapper_ptr = &rocksdb_wrapper;
        tmp_dataset_loader_param.workload_generator_ptr = workload_generator_ptr;
    }

    // (4) Launch dataset loaders for loading phase
    covered::Util::dumpNormalMsg(main_class_name, "launch dataset loaders...");
    int pthread_returncode = 0;
    pthread_t dataset_loader_threads[dataset_loadercnt];
    for (uint32_t dataset_loader_idx = 0; dataset_loader_idx < dataset_loadercnt; dataset_loader_idx++)
    {
        pthread_returncode = covered::Util::pthreadCreateHighPriority(&dataset_loader_threads[dataset_loader_idx], launchDatasetLoader, (void*)(&dataset_loader_params[dataset_loader_idx]));
        if (pthread_returncode != 0)
        {
            oss.clear();
            oss.str("");
            oss << "failed to launch dataset loader (error code: " << pthread_returncode << ")";
            covered::Util::dumpErrorMsg(main_class_name, oss.str());
            exit(1);
        }
    }

    // (5) Wait for dataset loaders
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

    // (6) Release variables
    assert(workload_generator_ptr != NULL);
    delete workload_generator_ptr;
    workload_generator_ptr = NULL;

    return 0;
}
/*
 * CloudWrapper: a cloud node to launch data server for receiving global requests and replying responses.
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#ifndef CLOUD_WRAPPER_H
#define CLOUD_WRAPPER_H

#include <string>

#include "cloud/rocksdb_wrapper.h"
#include "cli/cloud_cli.h"
#include "common/node_wrapper_base.h"
#include "network/propagation_simulator_param.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    class CloudWrapperParam
    {
    public:
        CloudWrapperParam();
        CloudWrapperParam(const uint32_t& cloud_idx, CloudCLI* cloud_cli_ptr);
        ~CloudWrapperParam();

        uint32_t getCloudIdx() const;
        CloudCLI* getCloudCLIPtr() const;

        CloudWrapperParam& operator=(const CloudWrapperParam& other);
    private:
        uint32_t cloud_idx_;
        CloudCLI* cloud_cli_ptr_;
    };

    class CloudWrapper : public NodeWrapperBase
    {
    public:
        static void* launchCloud(void* cloud_wrapper_param_ptr);

        CloudWrapper(const uint32_t& cloud_idx, const std::string& cloud_storage, const uint32_t& keycnt, const uint32_t& propagation_latency_edgecloud_us, const std::string& workload_name);
        ~CloudWrapper();

        WorkloadWrapperBase* getWorkloadGeneratorPtr() const;
        RocksdbWrapper* getCloudRocksdbPtr() const;
        PropagationSimulatorParam* getCloudToedgePropagationSimulatorParamPtr() const;
    private:
        static const std::string kClassName;

        virtual void initialize_() override;
        virtual void processFinishrunRequest_() override;
        virtual void processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr) override;
        virtual void cleanup_() override;

        void checkPointers_() const;

        std::string instance_name_;
        WorkloadWrapperBase* workload_generator_ptr_; // for warmup speedup to skip disk I/O latency in cloud (thread safe)
        RocksdbWrapper* cloud_rocksdb_ptr_;

        PropagationSimulatorParam* cloud_toedge_propagation_simulator_param_ptr_;

        // Sub-threads
        pthread_t cloud_toedge_propagation_simulator_thread_;
        pthread_t data_server_thread_; // TODO: only 1 cloud node now
    };
}

#endif
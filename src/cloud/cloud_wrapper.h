/*
 * CloudWrapper: the cloud to receive global requests and send responses.
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#ifndef CLOUD_WRAPPER_H
#define CLOUD_WRAPPER_H

//#define DEBUG_CLOUD_WRAPPER

#include <string>

#include "cloud/rocksdb_wrapper.h"
#include "common/node_wrapper_base.h"
#include "message/message_base.h"
#include "network/network_addr.h"
#include "network/propagation_simulator_param.h"

namespace covered
{
    class CloudWrapper : public NodeWrapperBase
    {
    public:
        static void* launchCloud(void* cloud_idx_ptr);

        CloudWrapper(const uint32_t& cloud_idx, const std::string& cloud_storage, const uint32_t& propagation_latency_edgecloud_us);
        ~CloudWrapper();
    private:
        static const std::string kClassName;

        virtual void initialize_() override;
        virtual void startInternal_() override;

        bool processGlobalRequest_(MessageBase* request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);

        void checkPointers_() const;

        std::string instance_name_;
        RocksdbWrapper* cloud_rocksdb_ptr_;

        // For receiving global requests
        NetworkAddr cloud_recvreq_source_addr_; // The same as that used by edge cache server worker to send global requests (const individual variable)
        UdpMsgSocketServer* cloud_recvreq_socket_server_ptr_; // Used by cloud to receive global requests from edge cache server worker (non-const individual variable)

        PropagationSimulatorParam* cloud_toedge_propagation_simulator_param_ptr_;

        // Sub-threads
        pthread_t cloud_toedge_propagation_simulator_thread_;
    };
}

#endif
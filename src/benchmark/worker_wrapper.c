#include "benchmark/worker_wrapper.h"

#include <assert.h> // assert
#include <sstream>

#include "benchmark/client_param.h"
#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/network_addr.h"
#include "workload/workload_item.h"

namespace covered
{
    const std::string WorkerWrapper::kClassName("Worker");

    void* WorkerWrapper::launchWorker(void* local_worker_param_ptr)
    {
        WorkerWrapper local_worker((WorkerParam*)local_worker_param_ptr);
        local_worker.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    WorkerWrapper::WorkerWrapper(WorkerParam* local_worker_param_ptr)
    {
        if (local_worker_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_worker_param_ptr is NULL!");
            exit(1);
        }
        local_worker_param_ptr_ = local_worker_param_ptr;
        assert(local_worker_param_ptr_ != NULL);

        ClientParam* local_client_param_ptr = local_worker_param_ptr_->getLocalClientParamPtr();
        assert(local_client_param_ptr != NULL);

        // Each per-client worker uses global_worker_idx to create a random generator and get different requests
        const uint32_t global_client_idx = local_client_param_ptr->getGlobalClientIdx();
        const uint32_t local_worker_idx = local_worker_param_ptr_->getLocalWorkerIdx();
        uint32_t global_worker_idx = Util::getGlobalWorkerIdx(global_client_idx, local_worker_idx);
        local_worker_item_randgen_ptr_ = new std::mt19937_64(global_worker_idx);
        if (local_worker_item_randgen_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "failed to create a random generator for requests!");
            exit(1);
        }

        // Prepare a socket client to closest edge recvreq port
        uint32_t global_client_idx = local_client_param_ptr->getGlobalClientIdx();
        std::string closest_edge_ipstr = Util::getClosestEdgeIpstr(global_client_idx);
        uint16_t closest_edge_recvreq_port = Util::getClosestEdgeRecvreqPort(global_client_idx);
        NetworkAddr remote_addr(closest_edge_ipstr, closest_edge_recvreq_port); // Communicate with the closest edge node
        local_worker_sendreq_toedge_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, remote_addr);
        assert(local_worker_sendreq_toedge_socket_client_ptr_ != NULL);
    }
    
    WorkerWrapper::~WorkerWrapper()
    {
        // NOTE: no need to delete local_worker_param_ptr_, as it is maintained outside WorkerWrapper

        assert(local_worker_item_randgen_ptr_ != NULL);
        delete local_worker_item_randgen_ptr_;
        local_worker_item_randgen_ptr_ = NULL;

        assert(local_worker_sendreq_toedge_socket_client_ptr_ != NULL);
        delete local_worker_sendreq_toedge_socket_client_ptr_;
        local_worker_sendreq_toedge_socket_client_ptr_ = NULL;
    }

    void WorkerWrapper::start()
    {
        assert(local_worker_item_randgen_ptr_ != NULL);
        assert(local_worker_sendreq_toedge_socket_client_ptr_ != NULL);

        assert(local_worker_param_ptr_ != NULL);
        ClientParam* local_client_param_ptr = local_worker_param_ptr_->getLocalClientParamPtr();
        assert(local_client_param_ptr != NULL);
        WorkloadWrapperBase* workload_generator_ptr = local_client_param_ptr->getWorkloadGeneratorPtr();
        assert(workload_generator_ptr != NULL);

        // Block until local_client_running_ becomes true
        while (!local_client_param_ptr->isClientRunning()) {}

        // Current worker thread start to issue requests and receive responses
        while (local_client_param_ptr->isClientRunning())
        {
            // Generate key-value request based on a specific workload
            WorkloadItem workload_item = workload_generator_ptr->generateItem(*local_worker_item_randgen_ptr_);

            // Convert workload item into local request message
            MessageBase* local_request_ptr = MessageBase::getLocalRequestFromWorkloadItem(workload_item);
            assert(local_request_ptr != NULL);

            // Convert local request into message payload
            uint32_t local_request_msg_payload_size = local_request_ptr->getMsgPayloadSize();
            DynamicArray local_request_msg_payload(local_request_msg_payload_size);
            uint32_t local_request_serialize_size = local_request_ptr->serialize(local_request_msg_payload);
            assert(local_request_serialize_size == local_request_msg_payload_size);

            // Timeout-and-retry mechanism
            DynamicArray local_response_msg_payload();
            while (true)
            {
                // Send the message payload of local request to the closest edge node
                local_worker_sendreq_toedge_socket_client_ptr_->send(local_request_msg_payload);

                // Receive the message payload of local response from the closest edge node
                bool is_timeout = local_worker_sendreq_toedge_socket_client_ptr_->recv(local_response_msg_payload);
                if (is_timeout)
                {
                    continue; // Resend the local request message
                }
                else
                {
                    break; // Receive the local response message successfully
                }
            }

            // Free local request message
            assert(local_request_ptr != NULL);
            delete local_request_ptr;
            local_request_ptr = NULL;

            // TODO: Process the response message and update statistics

            // TODO: remove later
            std::ostringstream oss;
            oss << "keystr: " << workload_item.getKey().getKeystr() << "; valuesize: " << workload_item.getValue().getValuesize() << std::endl;
            Util::dumpNormalMsg(kClassName, oss.str());
            break;
        }

        return;
    }
}
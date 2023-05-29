#include "benchmark/worker_wrapper.h"

#include <assert.h> // assert
#include <sstream>
#include <time.h> // struct timespec

#include "benchmark/client_param.h"
#include "common/dynamic_array.h"
#include "common/util.h"
#include "message/local_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"
#include "statistics/client_statistics_tracker.h"
#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

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

        // Each per-client worker uses global_worker_idx as deterministic seed to create a random generator and get different requests
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
            struct timespec sendreq_timestamp = Util::getCurrentTimespec();
            DynamicArray local_response_msg_payload;
            bool is_finish = false; // Mark if local client is finished
            while (true)
            {
                // Send the message payload of local request to the closest edge node
                PropagationSimulator::propagateFromClientToEdge(); // Simulate propagation latency
                local_worker_sendreq_toedge_socket_client_ptr_->send(local_request_msg_payload);

                // Receive the message payload of local response from the closest edge node
                bool is_timeout = local_worker_sendreq_toedge_socket_client_ptr_->recv(local_response_msg_payload);
                if (is_timeout)
                {
                    if (!local_client_param_ptr->isClientRunning())
                    {
                        is_finish = true;
                        break; // Client is NOT running
                    }
                    else
                    {
                        continue; // Resend the local request message
                    }
                }
                else
                {
                    break; // Receive the local response message successfully
                }
            }
            struct timespec recvrsp_timestamp = Util::getCurrentTimespec();

            // Release local request message
            assert(local_request_ptr != NULL);
            delete local_request_ptr;
            local_request_ptr = NULL;

            if (is_finish) // Check is_finish
            {
                continue; // Go to check if client is still running
            }

            // Get local response message
            MessageBase* local_response_ptr = MessageBase::getResponseFromMsgPayload(local_response_msg_payload);
            assert(local_response_ptr != NULL && local_response_ptr->isLocalResponse());
            
            // Process local response message to update statistics
            double rtt_us = Util::getDeltaTime(recvrsp_timestamp, sendreq_timestamp);
            processLocalResponse_(local_response_ptr, static_cast<uint32_t>(rtt_us));

            // Release local response message
            assert(local_response_ptr != NULL);
            delete local_response_ptr;
            local_response_ptr = NULL;
            break;
        }

        return;
    }

    void WorkerWrapper::processLocalResponse_(MessageBase* local_response_ptr, const uint32_t& rtt_us)
    {
        assert(local_response_ptr != NULL);

        assert(local_worker_param_ptr_ != NULL);
        ClientParam* local_client_param_ptr = local_worker_param_ptr_->getLocalClientParamPtr();
        assert(local_client_param_ptr != NULL);
        ClientStatisticsTracker* client_statistics_tracker_ptr_ = local_client_param_ptr->getClientStatisticsTrackerPtr();
        assert(client_statistics_tracker_ptr_ != NULL);

        // Process local response message
        Hitflag hitflag = Hitflag::kGlobalMiss;
        MessageType local_response_message_type = local_response_ptr->getMessageType();
        Key tmp_key;
        Value tmp_value;
        switch (local_response_message_type)
        {
            case MessageType::kLocalGetResponse:
            {
                LocalGetResponse* const local_get_response_ptr = static_cast<LocalGetResponse*>(local_response_ptr);
                tmp_key = local_get_response_ptr->getKey();
                tmp_value = local_get_response_ptr->getValue();
                hitflag = local_get_response_ptr->getHitflag();
                break;
            }
            case MessageType::kLocalPutResponse:
            {
                LocalPutResponse* const local_put_response_ptr = static_cast<LocalPutResponse*>(local_response_ptr);
                tmp_key = local_put_response_ptr->getKey();
                hitflag = local_put_response_ptr->getHitflag();
                break;
            }
            case MessageType::kLocalDelResponse:
            {
                LocalDelResponse* const local_del_response_ptr = static_cast<LocalDelResponse*>(local_response_ptr);
                tmp_key = local_del_response_ptr->getKey();
                hitflag = local_del_response_ptr->getHitflag();
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(local_response_message_type) << " for processLocalResponse_()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        // Update hit ratio statistics for the local client
        switch (hitflag)
        {
            case Hitflag::kLocalHit:
            {
                client_statistics_tracker_ptr_->updateLocalHitcnt(local_worker_param_ptr_->getLocalWorkerIdx());
                break;
            }
            case Hitflag::kCooperativeHit:
            {
                client_statistics_tracker_ptr_->updateCooperativeHitcnt(local_worker_param_ptr_->getLocalWorkerIdx());
                break;
            }
            case Hitflag::kGlobalMiss:
            {
                client_statistics_tracker_ptr_->updateReqcnt(local_worker_param_ptr_->getLocalWorkerIdx());
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid hitflag " << MessageBase::hitflagToString(hitflag) << " for processLocalResponse_()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        // Update latency statistics for the local client
        client_statistics_tracker_ptr_->updateLatency(local_worker_param_ptr_->getLocalWorkerIdx(), rtt_us);

        // TODO: remove later
        std::ostringstream oss;
        oss << "type: " << MessageBase::messageTypeToString(local_response_message_type) << "; keystr: " << tmp_key.getKeystr() << "; valuesize: " << tmp_value.getValuesize() << std::endl;
        Util::dumpNormalMsg(kClassName, oss.str());

        return;
    }
}
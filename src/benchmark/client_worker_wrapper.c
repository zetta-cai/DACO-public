#include "benchmark/client_worker_wrapper.h"

#include <assert.h> // assert
#include <sstream>
#include <time.h> // struct timespec

#include <unistd.h> // usleep // TMPDEBUG

#include "benchmark/client_param.h"
#include "common/dynamic_array.h"
#include "common/util.h"
#include "message/data_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"
#include "statistics/client_statistics_tracker.h"
#include "workload/workload_item.h"
#include "workload/workload_wrapper_base.h"

namespace covered
{
    const std::string ClientWorkerWrapper::kClassName("ClientWorkerWrapper");

    void* ClientWorkerWrapper::launchClientWorker(void* client_worker_param_ptr)
    {
        ClientWorkerWrapper local_client_worker((ClientWorkerParam*)client_worker_param_ptr);
        local_client_worker.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    ClientWorkerWrapper::ClientWorkerWrapper(ClientWorkerParam* client_worker_param_ptr)
    {
        if (client_worker_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_worker_param_ptr is NULL!");
            exit(1);
        }
        ClientParam* client_param_ptr = client_worker_param_ptr->getClientParamPtr();
        assert(client_param_ptr != NULL);

        const uint32_t client_idx = client_param_ptr->getClientIdx();
        const uint32_t local_client_worker_idx = client_worker_param_ptr->getLocalClientWorkerIdx();
        global_client_worker_idx_ = Util::getGlobalClientWorkerIdx(client_idx, local_client_worker_idx);

        // Differentiate different workers
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx << "-worker" << local_client_worker_idx << "-global" << global_client_worker_idx_;
        instance_name_ = oss.str();

        client_worker_param_ptr_ = client_worker_param_ptr;
        assert(client_worker_param_ptr_ != NULL);

        // Each per-client worker uses worker_idx as deterministic seed to create a random generator and get different requests
        client_worker_item_randgen_ptr_ = new std::mt19937_64(global_client_worker_idx_);
        if (client_worker_item_randgen_ptr_ == NULL)
        {
            Util::dumpErrorMsg(instance_name_, "failed to create a random generator for requests!");
            exit(1);
        }

        // Prepare a socket client to closest edge recvreq port
        std::string closest_edge_ipstr = Util::getClosestEdgeIpstr(client_idx);
        uint16_t closest_edge_cache_server_recvreq_port = Util::getClosestEdgeCacheServerRecvreqPort(client_idx);
        NetworkAddr remote_addr(closest_edge_ipstr, closest_edge_cache_server_recvreq_port); // Communicate with the closest edge node
        client_worker_sendreq_toedge_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, remote_addr);
        assert(client_worker_sendreq_toedge_socket_client_ptr_ != NULL);
    }
    
    ClientWorkerWrapper::~ClientWorkerWrapper()
    {
        // NOTE: no need to delete client_worker_param_ptr_, as it is maintained outside ClientWorkerWrapper

        assert(client_worker_item_randgen_ptr_ != NULL);
        delete client_worker_item_randgen_ptr_;
        client_worker_item_randgen_ptr_ = NULL;

        assert(client_worker_sendreq_toedge_socket_client_ptr_ != NULL);
        delete client_worker_sendreq_toedge_socket_client_ptr_;
        client_worker_sendreq_toedge_socket_client_ptr_ = NULL;
    }

    void ClientWorkerWrapper::start()
    {
        assert(client_worker_item_randgen_ptr_ != NULL);

        assert(client_worker_param_ptr_ != NULL);
        ClientParam* client_param_ptr = client_worker_param_ptr_->getClientParamPtr();
        assert(client_param_ptr != NULL);
        WorkloadWrapperBase* workload_generator_ptr = client_param_ptr->getWorkloadGeneratorPtr();
        assert(workload_generator_ptr != NULL);

        // Block until client_running_ becomes true
        while (!client_param_ptr->isClientRunning()) {}

        // Current worker thread start to issue requests and receive responses
        while (client_param_ptr->isClientRunning())
        {
            // Generate key-value request based on a specific workload
            WorkloadItem workload_item = workload_generator_ptr->generateItem(*client_worker_item_randgen_ptr_);

            // TMPDEBUG
            //WorkloadItem workload_item(Key("123"), Value(200), WorkloadItemType::kWorkloadItemGet);
            //if (client_param_ptr->getClientIdx() != 0)
            //{
            //    sleep(0.5);
            //}

            DynamicArray local_response_msg_payload;
            uint32_t rtt_us = 0;
            bool is_finish = false;

            // Issue the workload item to the closest edge node
            is_finish = issueItemToEdge_(workload_item, local_response_msg_payload, rtt_us);
            if (is_finish) // Check is_finish
            {
                continue; // Go to check if client is still running
            }

            // Process local response message to update statistics
            processLocalResponse_(local_response_msg_payload, rtt_us);

            // TMPDEBUG
            //is_finish = issueItemToEdge_(workload_item, local_response_msg_payload, rtt_us);
            //if (is_finish) // Check is_finish
            //{
            //    continue; // Go to check if client is still running
            //}
            //processLocalResponse_(local_response_msg_payload, rtt_us);

            break;
        }

        return;
    }

    bool ClientWorkerWrapper::issueItemToEdge_(const WorkloadItem& workload_item, DynamicArray& local_response_msg_payload, uint32_t& rtt_us)
    {
        assert(client_worker_param_ptr_ != NULL);
        ClientParam* client_param_ptr = client_worker_param_ptr_->getClientParamPtr();
        assert(client_param_ptr != NULL);
        assert(client_worker_sendreq_toedge_socket_client_ptr_ != NULL);
        
        bool is_finish = false; // Mark if local client is finished

        // Convert workload item into local request message
        MessageBase* local_request_ptr = MessageBase::getLocalRequestFromWorkloadItem(workload_item, global_client_worker_idx_);
        assert(local_request_ptr != NULL);

        // Convert local request into message payload
        uint32_t local_request_msg_payload_size = local_request_ptr->getMsgPayloadSize();
        DynamicArray local_request_msg_payload(local_request_msg_payload_size);
        uint32_t local_request_serialize_size = local_request_ptr->serialize(local_request_msg_payload);
        assert(local_request_serialize_size == local_request_msg_payload_size);

        // TMPDEBUG
        std::ostringstream oss;
        oss << "issue a local request; type: " << MessageBase::messageTypeToString(local_request_ptr->getMessageType()) << "; keystr: " << workload_item.getKey().getKeystr() << "; valuesize: " << workload_item.getValue().getValuesize();// << std::endl << "Msg payload: " << local_request_msg_payload.getBytesHexstr();
        Util::dumpDebugMsg(instance_name_, oss.str());

        struct timespec sendreq_timestamp = Util::getCurrentTimespec();
        while (true) // Timeout-and-retry mechanism
        {
            // Send the message payload of local request to the closest edge node
            PropagationSimulator::propagateFromClientToEdge(); // Simulate propagation latency
            client_worker_sendreq_toedge_socket_client_ptr_->send(local_request_msg_payload);

            // Receive the message payload of local response from the closest edge node
            bool is_timeout = client_worker_sendreq_toedge_socket_client_ptr_->recv(local_response_msg_payload);
            if (is_timeout)
            {
                if (!client_param_ptr->isClientRunning())
                {
                    is_finish = true;
                    break; // Client is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "client timeout to wait for local response");
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

        double tmp_rtt_us = Util::getDeltaTimeUs(recvrsp_timestamp, sendreq_timestamp);
        rtt_us = static_cast<uint32_t>(tmp_rtt_us);

        return is_finish;
    }

    void ClientWorkerWrapper::processLocalResponse_(const DynamicArray& local_response_msg_payload, const uint32_t& rtt_us)
    {
        // Get local response message
        MessageBase* local_response_ptr = MessageBase::getResponseFromMsgPayload(local_response_msg_payload);
        assert(local_response_ptr != NULL && local_response_ptr->isLocalResponse());

        assert(client_worker_param_ptr_ != NULL);
        ClientParam* client_param_ptr = client_worker_param_ptr_->getClientParamPtr();
        assert(client_param_ptr != NULL);
        ClientStatisticsTracker* client_statistics_tracker_ptr_ = client_param_ptr->getClientStatisticsTrackerPtr();
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
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Update hit ratio statistics for the local client
        switch (hitflag)
        {
            case Hitflag::kLocalHit:
            {
                client_statistics_tracker_ptr_->updateLocalHitcnt(client_worker_param_ptr_->getLocalClientWorkerIdx());
                break;
            }
            case Hitflag::kCooperativeHit:
            {
                client_statistics_tracker_ptr_->updateCooperativeHitcnt(client_worker_param_ptr_->getLocalClientWorkerIdx());
                break;
            }
            case Hitflag::kGlobalMiss:
            {
                client_statistics_tracker_ptr_->updateReqcnt(client_worker_param_ptr_->getLocalClientWorkerIdx());
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid hitflag " << MessageBase::hitflagToString(hitflag) << " for processLocalResponse_()!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Update latency statistics for the local client
        client_statistics_tracker_ptr_->updateLatency(rtt_us);

        // TMPDEBUG
        std::ostringstream oss;
        oss << "receive a local response; type: " << MessageBase::messageTypeToString(local_response_message_type) << "; keystr: " << tmp_key.getKeystr() << "; valuesize: " << tmp_value.getValuesize() << "; hitflag: " << MessageBase::hitflagToString(hitflag);// << std::endl << "Msg payload: " << local_response_msg_payload.getBytesHexstr();
        Util::dumpDebugMsg(instance_name_, oss.str());

        // Release local response message
        assert(local_response_ptr != NULL);
        delete local_response_ptr;
        local_response_ptr = NULL;

        return;
    }
}
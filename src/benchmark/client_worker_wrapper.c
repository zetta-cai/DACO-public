#include "benchmark/client_worker_wrapper.h"

#include <assert.h> // assert
#include <sstream>
#include <time.h> // struct timespec

#include "common/config.h"
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

    ClientWorkerWrapper::ClientWorkerWrapper(ClientWorkerParam* client_worker_param_ptr) : client_worker_param_ptr_(client_worker_param_ptr)
    {
        if (client_worker_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "client_worker_param_ptr is NULL!");
            exit(1);
        }

        // Get client index
        ClientWrapper* client_wrapper_ptr = client_worker_param_ptr->getClientWrapperPtr();
        assert(client_wrapper_ptr != NULL);
        const uint32_t client_idx = client_wrapper_ptr->getNodeIdx();
        const uint32_t clientcnt = client_wrapper_ptr->getNodeCnt();
        const uint32_t edgecnt = client_wrapper_ptr->getEdgeCnt();
        const uint32_t perclient_workercnt = client_wrapper_ptr->getPerclientWorkercnt();
        const uint32_t local_client_worker_idx = client_worker_param_ptr->getLocalClientWorkerIdx();
        uint32_t global_client_worker_idx = Util::getGlobalClientWorkerIdx(client_idx, local_client_worker_idx, perclient_workercnt);

        // Differentiate different workers
        std::ostringstream oss;
        oss << kClassName << " client" << client_idx << "-worker" << local_client_worker_idx << "-global" << global_client_worker_idx;
        instance_name_ = oss.str();

        // (1) For sending local requests

        // Get closest edge network address to send local requests
        std::string closest_edge_ipstr = Util::getClosestEdgeIpstr(client_idx, clientcnt, edgecnt);
        uint16_t closest_edge_cache_server_recvreq_port = Util::getClosestEdgeCacheServerRecvreqPort(client_idx, clientcnt, edgecnt, closest_edge_idx_);
        closest_edge_cache_server_recvreq_dst_addr_ = NetworkAddr(closest_edge_ipstr, closest_edge_cache_server_recvreq_port);

        // (2) For receiving local responses

        // Get source address of client worker to receive local responses
        const bool is_private_client_ipstr = true; // NOTE: client communicates with the closest edge node via private IP address
        const bool is_launch_client = true; // The client worker belones to the logical client node launched in the current physical machine
        std::string client_ipstr = Config::getClientIpstr(client_idx, clientcnt, is_private_client_ipstr, is_launch_client);
        uint16_t client_worker_recvrsp_port = Util::getClientWorkerRecvrspPort(client_idx, clientcnt, local_client_worker_idx, perclient_workercnt);
        client_worker_recvrsp_source_addr_ = NetworkAddr(client_ipstr, client_worker_recvrsp_port);

        // Prepare a socket server to receive local responses
        NetworkAddr host_addr(Util::ANY_IPSTR, client_worker_recvrsp_port);
        client_worker_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(client_worker_recvrsp_socket_server_ptr_ != NULL);

        // (3) For per-client-worker warmup reqcnt limitation

        cur_warmup_reqcnt_ = 0;
        const uint32_t total_warmup_reqcnt = client_wrapper_ptr->getKeycnt() * client_wrapper_ptr->getWarmupReqcntScale();
        const uint32_t total_client_workercnt = client_wrapper_ptr->getNodeCnt() / client_wrapper_ptr->getPerclientWorkercnt();
        warmup_reqcnt_limit_ = (total_warmup_reqcnt - 1) / total_client_workercnt + 1; // Get per-client-worker warmup reqcnt limitation
        assert(warmup_reqcnt_limit_ > 0);
        assert(warmup_reqcnt_limit_ * total_client_workercnt >= total_warmup_reqcnt); // Total # of issued warmup reqs MUST >= total # of required warmup reqs
    }
    
    ClientWorkerWrapper::~ClientWorkerWrapper()
    {
        // NOTE: no need to delete client_worker_param_ptr_, as it is maintained outside ClientWorkerWrapper

        // Release socket server
        assert(client_worker_recvrsp_socket_server_ptr_ != NULL);
        delete client_worker_recvrsp_socket_server_ptr_;
        client_worker_recvrsp_socket_server_ptr_ = NULL;
    }

    void ClientWorkerWrapper::start()
    {
        checkPointers_();
        ClientWrapper* tmp_client_wrapper_ptr = client_worker_param_ptr_->getClientWrapperPtr();
        const uint32_t local_client_worker_idx = client_worker_param_ptr_->getLocalClientWorkerIdx();
        
        WorkloadWrapperBase* workload_generator_ptr = tmp_client_wrapper_ptr->getWorkloadWrapperPtr();

        // Block until client_running_ becomes true
        while (!tmp_client_wrapper_ptr->isNodeRunning()) {}

        // Current worker thread start to issue requests and receive responses
        // uint32_t tmp_i = 0; // TMPDEBUG
        const bool is_warmup_speedup = tmp_client_wrapper_ptr->isWarmupSpeedup(); // NOTE: warmup speedup will skip propagation latency in network and disk I/O latency in cloud
        while (tmp_client_wrapper_ptr->isNodeRunning())
        {
            // Get current phase (warmup or stresstest)
            bool is_warmup_phase = tmp_client_wrapper_ptr->isWarmupPhase();
            bool is_stresstest_phase = !is_warmup_phase;

            // Consider per-client-worker warmup reqcnt limitation to avoid inconsistent warmup progress (under second-level evaluator monitoring) among different caches due to different warmup speed
            if (is_warmup_phase)
            {
                if (cur_warmup_reqcnt_ < warmup_reqcnt_limit_)
                {
                    cur_warmup_reqcnt_++;
                }
                else
                {
                    continue; // Skip issuing workload items to edge unless entering stresstest phase
                }
            }

            // Generate key-value request based on a specific workload
            WorkloadItem workload_item = workload_generator_ptr->generateWorkloadItem(local_client_worker_idx);

            // TMPDEBUG (50% PUT)
            // if (tmp_i % 2 == 0)
            // {
            //     workload_item = WorkloadItem(workload_item.getKey(), workload_item.getValue(), WorkloadItemType::kWorkloadItemPut);
            // }
            // tmp_i++;

            // TMPDEBUG
            //WorkloadItem workload_item(Key("123"), Value(200), WorkloadItemType::kWorkloadItemGet);
            //if (tmp_client_wrapper_ptr->node_idx_ != 0)
            //{
            //    sleep(0.5);
            //}

            DynamicArray local_response_msg_payload;
            uint32_t rtt_us = 0;
            bool is_finish = false;

            // Issue the workload item to the closest edge node
            is_finish = issueItemToEdge_(workload_item, local_response_msg_payload, rtt_us, is_warmup_phase, is_warmup_speedup);
            if (is_finish) // Check is_finish
            {
                continue; // Go to check if client is still running
            }

            // Process local response message to update statistics
            /*if (!is_stresstest_phase && !tmp_client_wrapper_ptr->isWarmupPhase_()) // Detect responses of warmup requests received after warmup phase (e.g., after curslot switch or during stresstest phase)
            {
                // TODO: as there could still exist limited warmup requests are counted in stresstest beginning slots, we can print the start slot idx of stresstest phase to drop the invalid statistics of the beginning slots
                continue; // NOT update to avoid affecting cur-slot raw/aggregated statistics especially for warmup speedup
            }*/
            processLocalResponse_(workload_item, local_response_msg_payload, rtt_us, is_stresstest_phase);

            // TMPDEBUG
            //is_finish = issueItemToEdge_(workload_item, local_response_msg_payload, rtt_us);
            //if (is_finish) // Check is_finish
            //{
            //    continue; // Go to check if client is still running
            //}
            //processLocalResponse_(local_response_msg_payload, rtt_us);

            //break; // TMPDEBUG
        }

        return;
    }

    bool ClientWorkerWrapper::issueItemToEdge_(const WorkloadItem& workload_item, DynamicArray& local_response_msg_payload, uint32_t& rtt_us, const bool& is_warmup_phase, const bool& is_warmup_speedup)
    {
        checkPointers_();
        ClientWrapper* tmp_client_wrapper_ptr = client_worker_param_ptr_->getClientWrapperPtr();
        
        bool is_finish = false; // Mark if local client is finished

        // TMPDEBUG
        //WorkloadItem workload_item(tmp_workload_item.getKey(), Value(200000), WorkloadItemType::kWorkloadItemPut);

        struct timespec sendreq_timestamp = Util::getCurrentTimespec();
        while (true) // Timeout-and-retry mechanism
        {
            // Convert workload item into local request message
            MessageBase* local_request_ptr = MessageBase::getRequestFromWorkloadItem(workload_item, tmp_client_wrapper_ptr->getNodeIdx(), client_worker_recvrsp_source_addr_, is_warmup_phase, is_warmup_speedup);
            assert(local_request_ptr != NULL);

            #ifdef DEBUG_CLIENT_WORKER_WRAPPER
            Util::dumpVariablesForDebug(instance_name_, 7, "issue a local request;", "type:", MessageBase::messageTypeToString(local_request_ptr->getMessageType()).c_str(), "keystr:", workload_item.getKey().getKeystr().c_str(), "valuesize:", std::to_string(workload_item.getValue().getValuesize()).c_str());
            #endif

            // Push local request into client-to-edge propagation simulator to send to closest edge node
            bool is_successful = tmp_client_wrapper_ptr->getClientToedgePropagationSimulatorParamPtr()->push(local_request_ptr, closest_edge_cache_server_recvreq_dst_addr_);
            assert(is_successful);
            
            // NOTE: local_request_ptr will be released by client-to-edge propagation simulator
            local_request_ptr = NULL;

            // Receive the message payload of local response from the closest edge node
            bool is_timeout = client_worker_recvrsp_socket_server_ptr_->recv(local_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_client_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Client is NOT running
                }
                else
                {
                    std::ostringstream oss;
                    oss << "client timeout to wait for local response for key " << workload_item.getKey().getKeystr() << "from edge " << closest_edge_idx_;
                    Util::dumpWarnMsg(instance_name_, oss.str());
                    continue; // Resend the local request message
                }
            }
            else
            {
                break; // Receive the local response message successfully
            }
        }
        struct timespec recvrsp_timestamp = Util::getCurrentTimespec();

        double tmp_rtt_us = Util::getDeltaTimeUs(recvrsp_timestamp, sendreq_timestamp);
        rtt_us = static_cast<uint32_t>(tmp_rtt_us);

        return is_finish;
    }

    void ClientWorkerWrapper::processLocalResponse_(const WorkloadItem& workload_item, const DynamicArray& local_response_msg_payload, const uint32_t& rtt_us, const bool& is_stresstest_phase)
    {
        checkPointers_();
        ClientWrapper* tmp_client_wrapper_ptr = client_worker_param_ptr_->getClientWrapperPtr();
        uint32_t local_client_worker_idx = client_worker_param_ptr_->getLocalClientWorkerIdx();

        // Get local response message
        MessageBase* local_response_ptr = MessageBase::getResponseFromMsgPayload(local_response_msg_payload);
        assert(local_response_ptr != NULL && local_response_ptr->isLocalDataResponse());

        ClientStatisticsTracker* client_statistics_tracker_ptr_ = tmp_client_wrapper_ptr->getClientStatisticsTrackerPtr();

        // Process local response message
        MessageType local_response_message_type = local_response_ptr->getMessageType();
        Key tmp_key;
        Value tmp_value;
        Hitflag hitflag = Hitflag::kGlobalMiss;
        uint64_t closest_edge_cache_size_bytes = 0;
        uint64_t closest_edge_cache_capacity_bytes = 0;
        bool is_write = false;
        switch (local_response_message_type)
        {
            case MessageType::kLocalGetResponse:
            {
                LocalGetResponse* const local_get_response_ptr = static_cast<LocalGetResponse*>(local_response_ptr);
                tmp_key = local_get_response_ptr->getKey();
                tmp_value = local_get_response_ptr->getValue();
                hitflag = local_get_response_ptr->getHitflag();
                closest_edge_cache_size_bytes = local_get_response_ptr->getCacheSizeBytes();
                closest_edge_cache_capacity_bytes = local_get_response_ptr->getCacheCapacityBytes();
                is_write = false;
                break;
            }
            case MessageType::kLocalPutResponse:
            {
                LocalPutResponse* const local_put_response_ptr = static_cast<LocalPutResponse*>(local_response_ptr);
                tmp_key = local_put_response_ptr->getKey();
                hitflag = local_put_response_ptr->getHitflag();
                closest_edge_cache_size_bytes = local_put_response_ptr->getCacheSizeBytes();
                closest_edge_cache_capacity_bytes = local_put_response_ptr->getCacheCapacityBytes();
                is_write = true;
                break;
            }
            case MessageType::kLocalDelResponse:
            {
                LocalDelResponse* const local_del_response_ptr = static_cast<LocalDelResponse*>(local_response_ptr);
                tmp_key = local_del_response_ptr->getKey();
                hitflag = local_del_response_ptr->getHitflag();
                closest_edge_cache_size_bytes = local_del_response_ptr->getCacheSizeBytes();
                closest_edge_cache_capacity_bytes = local_del_response_ptr->getCacheCapacityBytes();
                is_write = true;
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
        const uint32_t tmp_object_size = tmp_key.getKeyLength() + tmp_value.getValuesize();
        switch (hitflag)
        {
            case Hitflag::kLocalHit:
            {
                client_statistics_tracker_ptr_->updateLocalHitcnt(local_client_worker_idx, is_stresstest_phase);
                client_statistics_tracker_ptr_->updateLocalHitbytes(local_client_worker_idx, tmp_object_size, is_stresstest_phase);
                break;
            }
            case Hitflag::kCooperativeHit:
            {
                client_statistics_tracker_ptr_->updateCooperativeHitcnt(local_client_worker_idx, is_stresstest_phase);
                client_statistics_tracker_ptr_->updateCooperativeHitbytes(local_client_worker_idx, tmp_object_size, is_stresstest_phase);
                break;
            }
            case Hitflag::kGlobalMiss:
            {
                client_statistics_tracker_ptr_->updateReqcnt(local_client_worker_idx, is_stresstest_phase);
                client_statistics_tracker_ptr_->updateReqbytes(local_client_worker_idx, tmp_object_size, is_stresstest_phase);
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
        client_statistics_tracker_ptr_->updateLatency(local_client_worker_idx, rtt_us, is_stresstest_phase);

        // Update read-write ratio statistics for the local client
        if (!is_write)
        {
            client_statistics_tracker_ptr_->updateReadcnt(local_client_worker_idx, is_stresstest_phase);
        }
        else
        {
            client_statistics_tracker_ptr_->updateWritecnt(local_client_worker_idx, is_stresstest_phase);
        }

        // Update cache utilization statistics for the local client
        client_statistics_tracker_ptr_->updateCacheUtilization(local_client_worker_idx, closest_edge_cache_size_bytes, closest_edge_cache_capacity_bytes, is_stresstest_phase);

        // Update value size statistics for the local client
        uint32_t value_size = tmp_value.getValuesize();
        if (local_response_message_type == MessageType::kLocalPutResponse)
        {
            value_size = workload_item.getValue().getValuesize();
        }
        client_statistics_tracker_ptr_->updateWorkloadKeyValueSize(local_client_worker_idx, tmp_key.getKeyLength(), value_size, is_stresstest_phase);

        // Update bandwidth usage statistics for the local client
        BandwidthUsage local_response_bandwidth_usage = local_response_ptr->getBandwidthUsageRef();
        uint32_t client_edge_local_rsp_bandwidth_bytes = local_response_ptr->getMsgBandwidthSize();
        local_response_bandwidth_usage.update(BandwidthUsage(client_edge_local_rsp_bandwidth_bytes, 0, 0, 1, 0, 0)); // Get total bandwidth usage for received local response
        client_statistics_tracker_ptr_->updateBandwidthUsage(local_client_worker_idx, local_response_bandwidth_usage, is_stresstest_phase);

        #ifdef DEBUG_CLIENT_WORKER_WRAPPER
        Util::dumpVariablesForDebug(instance_name_, 13, "receive a local response;", "type:", MessageBase::messageTypeToString(local_response_message_type).c_str(), "keystr", tmp_key.getKeystr().c_str(), "valuesize:", std::to_string(tmp_value.getValuesize()).c_str(), "hitflag:", MessageBase::hitflagToString(hitflag).c_str(), "latency:", std::to_string(rtt_us).c_str(), "eventlist:", local_response_ptr->getEventListRef().toString().c_str());
        // "msg payload:", local_response_msg_payload.getBytesHexstr().c_str()
        #endif

        // Release local response message
        assert(local_response_ptr != NULL);
        delete local_response_ptr;
        local_response_ptr = NULL;

        return;
    }

    void ClientWorkerWrapper::checkPointers_() const
    {
        assert(client_worker_param_ptr_ != NULL);
        assert(client_worker_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
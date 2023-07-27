#include "benchmark/evaluator_wrapper.h"

#include <assert.h>
#include <sstream>
#include <unordered_map>

#include "common/config.h"
#include "common/dynamic_array.h"
#include "common/param.h"
#include "common/util.h"
#include "message/message_base.h"
#include "message/control_message.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    const std::string EvaluatorWrapper::kClassName("EvaluatorWrapper");

    void* EvaluatorWrapper::launchEvaluator(void* is_evaluator_initialized_ptr)
    {
        bool& is_evaluator_initialized = *(static_cast<bool*>(is_evaluator_initialized_ptr));

        EvaluatorWrapper evaluator(Param::getClientcnt(), Param::getDurationSec(), Param::getEdgecnt());
        is_evaluator_initialized = true; // Such that simulator or prototype will continue to launch cloud, edge, and client nodes

        evaluator.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EvaluatorWrapper::EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& duration_sec, const uint32_t& edgecnt) : clientcnt_(clientcnt), duration_sec_(duration_sec), edgecnt_(edgecnt)
    {
        is_warmup_phase_ = true;
        cur_slot_idx_ = 0;

        // (1) Manage evaluation phases

        // Prepare evaluator recvmsg source addr
        std::string evaluator_ipstr = Config::getEvaluatorIpstr();
        uint16_t evaluator_recvmsg_port = Config::getEvaluatorRecvmsgPort();
        evaluator_recvmsg_source_addr_ = NetworkAddr(evaluator_ipstr, evaluator_recvmsg_port);

        // Prepare client recvmsg dst addrs
        perclient_recvmsg_dst_addrs_ = new NetworkAddr[clientcnt];
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            std::string tmp_client_ipstr = Config::getClientIpstr(client_idx, clientcnt);
            uint16_t tmp_client_recvmsg_port = Util::getClientRecvmsgPort(client_idx, clientcnt);
            perclient_recvmsg_dst_addrs_[client_idx] = NetworkAddr(tmp_client_ipstr, tmp_client_recvmsg_port);
        }

        // Prepare edge recvmsg dst addrs
        peredge_recvmsg_dst_addrs_ = new NetworkAddr[edgecnt];
        assert(peredge_recvmsg_dst_addrs_ != NULL);
        for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
        {
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
            uint16_t tmp_edge_recvmsg_port = Util::getEdgeRecvmsgPort(edge_idx, edgecnt);
            peredge_recvmsg_dst_addrs_[edge_idx] = NetworkAddr(tmp_edge_ipstr, tmp_edge_recvmsg_port);
        }

        // Prepare cloud recvmsg dst addr
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvmsg_port = Util::getCloudRecvmsgPort(0);
        cloud_recvmsg_dst_addr_ = NetworkAddr(cloud_ipstr, cloud_recvmsg_port);

        // Prepare a socket server to receive benchmark control messages
        NetworkAddr host_addr(Util::ANY_IPSTR, evaluator_recvmsg_port);
        evaluator_recvmsg_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);

        // Prepare a socket client to send benchmark control messages
        evaluator_sendmsg_socket_client_ptr_ = new UdpMsgSocketClient();
        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);

        // (2) Track total aggregated statistics

        total_statistics_tracker_ptr_ = new TotalStatisticsTracker();
        assert(total_statistics_tracker_ptr_ != NULL);
    }

    EvaluatorWrapper::~EvaluatorWrapper()
    {
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        delete[] perclient_recvmsg_dst_addrs_;
        perclient_recvmsg_dst_addrs_ = NULL;

        assert(peredge_recvmsg_dst_addrs_ != NULL);
        delete[] peredge_recvmsg_dst_addrs_;
        peredge_recvmsg_dst_addrs_ = NULL;

        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);
        delete evaluator_recvmsg_socket_server_ptr_;
        evaluator_recvmsg_socket_server_ptr_ = NULL;

        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);
        delete evaluator_sendmsg_socket_client_ptr_;
        evaluator_sendmsg_socket_client_ptr_ = NULL;

        assert(total_statistics_tracker_ptr_ != NULL);
        delete total_statistics_tracker_ptr_;
        total_statistics_tracker_ptr_ = NULL;
    }

    void EvaluatorWrapper::start()
    {
        checkPointers_();
        
        // Wait all componenets to finish initiailization
        blockForInitialization_();

        Util::dumpNormalMsg(kClassName, "Start benchmark...");

        // Notify all clients to start running
        notifyClientsToStartrun_();

        // Monitor cache hit ratio for warmup and stresstest phases
        const uint32_t client_raw_statistics_slot_interval_sec = Config::getClientRawStatisticsSlotIntervalSec();
        bool is_stable = false;
        double stable_hit_ratio = 0.0d;
        struct timespec start_timestamp; // For duration of stresstest phase
        struct timespec prev_timestamp = Util::getCurrentTimespec(); // For switch slot
        while (true)
        {
            usleep(Util::SLEEP_INTERVAL_US);

            struct timespec cur_timestamp = Util::getCurrentTimespec();

            // Count down duration for stresstest phase to judge if benchmark is finished
            if (!is_warmup_phase_) // Stresstest phase
            {
                double delta_us_for_finishrun = Util::getDeltaTimeUs(cur_timestamp, start_timestamp);
                if (delta_us_for_finishrun >= duration_sec_ * 1000 * 1000)
                {
                    Util::dumpNormalMsg(kClassName, "Stop benchmark...");

                    // Notify client/edge/cloud to finish run
                    notifyAllToFinishrun_();

                    break;
                }
            }

            // Switch cur-slot client raw statistics to track per-slot aggregated statistics
            double delta_us_for_switch_slot = Util::getDeltaTimeUs(cur_timestamp, prev_timestamp);
            if (delta_us_for_switch_slot >= static_cast<double>(client_raw_statistics_slot_interval_sec * 1000 * 1000))
            {
                // Notify clients to switch cur-slot client raw statistics
                notifyClientsToSwitchSlot_(); // Increase cur_slot_idx_ by one

                // TOOD: Update total_statistics_tracker_ptr_

                // Update prev_timestamp for the next slot
                prev_timestamp = cur_timestamp;
            }

            // Monitor if cache hit ratio is stable to finish the warmup phase if any
            if (is_warmup_phase_) // Warmup phase
            {
                is_stable = total_statistics_tracker_ptr_->isPerSlotTotalAggregatedStatisticsStable(stable_hit_ratio);

                // Cache hit ratio becomes stable
                if (is_stable)
                {
                    std::ostringstream oss;
                    oss << "cache hit ratio becomes stable at " << stable_hit_ratio << " -> finish warmup phase";
                    Util::dumpNormalMsg(kClassName, oss.str());

                    // Notify all clients to finish warmup phase (i.e., enter stresstest phase)
                    notifyClientsToFinishWarmup_(); // Mark is_warmup_phase_ as false

                    // Duration starts from the end of warmup phase
                    is_warmup_phase_ = false;
                    start_timestamp = Util::getCurrentTimespec();
                }
            }
        }

        // Print per-slot/stable total aggregated statistics
        std::ostringstream oss;
        oss << total_statistics_tracker_ptr_->toString();
        Util::dumpNormalMsg(kClassName, oss.str());

        // Dump per-slot/stable total aggregated statistics
        std::string evaluator_statistics_filepath = Util::getEvaluatorStatisticsFilepath();
        oss.clear();
        oss.str("");
        oss << "dump per-slot/stable total aggregated statistics into " << evaluator_statistics_filepath;
        Util::dumpNormalMsg(kClassName, oss.str());
        uint32_t dump_size = total_statistics_tracker_ptr_->dump(evaluator_statistics_filepath);
        oss.clear();
        oss.str("");
        oss << "finish dump for " << dump_size << " bytes";
        Util::dumpNormalMsg(kClassName, oss.str());

        return;
    }

    void EvaluatorWrapper::blockForInitialization_()
    {
        checkPointers_();

        // Client/edge/cloud ack flags
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> initialization_acked_flags;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(perclient_recvmsg_dst_addrs_[client_idx], false));
        }
        for (uint32_t edge_idx = 0; edge_idx < edgecnt_; edge_idx++)
        {
            initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(peredge_recvmsg_dst_addrs_[edge_idx], false));
        }
        initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(cloud_recvmsg_dst_addr_, false));

        Util::dumpNormalMsg(kClassName, "Wait for intialization of all clients, edges, and clouds...");

        // Wait for InitializeRequests from client/edge/cloud
        uint32_t acked_cnt = 0;
        while (acked_cnt < initialization_acked_flags.size())
        {
            DynamicArray control_request_msg_payload;
            bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                continue; // Wait until all client/edge/cloud finish initialization
            }
            else
            {
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);
                assert(control_request_ptr->getMessageType() == MessageType::kInitializationRequest);

                NetworkAddr tmp_dst_addr = control_request_ptr->getSourceAddr();
                std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::iterator iter = initialization_acked_flags.find(tmp_dst_addr);
                if (iter == initialization_acked_flags.end())
                {
                    std::ostringstream oss;
                    oss << "receive InitializationRequest from network addr " << tmp_dst_addr.toString() << ", which is NOT in the to-be-acked list!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
                else
                {
                    if (iter->second == false)
                    {
                        // Send back InitializationResponse to client/edge/cloud
                        InitializationResponse initialization_response(0, evaluator_recvmsg_source_addr_, EventList());
                        evaluator_sendmsg_socket_client_ptr_->send((MessageBase*)&initialization_response, tmp_dst_addr);

                        iter->second = true;
                        acked_cnt++;
                    }
                }

                delete control_request_ptr;
                control_request_ptr = NULL;
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients, edges, and clouds finish initialization!");

        return;
    }

    void EvaluatorWrapper::notifyClientsToStartrun_()
    {
        checkPointers_();

        // Client ack flags
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> startrun_acked_flags;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            startrun_acked_flags.insert(std::pair<NetworkAddr, bool>(perclient_recvmsg_dst_addrs_[client_idx], false));
        }

        Util::dumpNormalMsg(kClassName, "Notify all clients to start running...");

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < startrun_acked_flags.size())
        {
            // Issue StartrunRequests to unacked clients simultaneously
            for (std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::const_iterator iter_for_req = startrun_acked_flags.begin(); iter_for_req != startrun_acked_flags.end(); iter_for_req++)
            {
                if (iter_for_req->second == false)
                {
                    StartrunRequest tmp_startrun_request(0, evaluator_recvmsg_source_addr_);
                    evaluator_sendmsg_socket_client_ptr_->send((MessageBase*)&tmp_startrun_request, iter_for_req->first);
                }
            }

            // Receive StartrunResponses for unacked clients
            for (uint32_t i = 0; i < (startrun_acked_flags.size() - acked_cnt); i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for StartrunResponse!");
                    break; // Wait until all clients are running
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getRequestFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kStartrunResponse);

                    NetworkAddr tmp_dst_addr = control_response_ptr->getSourceAddr();
                    std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::iterator iter = startrun_acked_flags.find(tmp_dst_addr);
                    if (iter == startrun_acked_flags.end())
                    {
                        std::ostringstream oss;
                        oss << "receive StartrunResponse from network addr " << tmp_dst_addr.toString() << ", which is NOT in the to-be-acked list!";
                        Util::dumpWarnMsg(kClassName, oss.str());
                    }
                    else
                    {
                        if (iter->second == false)
                        {
                            iter->second = true;
                            acked_cnt++;
                        }
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients are running!");

        return;
    }

    void EvaluatorWrapper::notifyClientsToSwitchSlot_()
    {
        checkPointers_();

        // Client ack flags
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> switchslot_acked_flags;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            switchslot_acked_flags.insert(std::pair<NetworkAddr, bool>(perclient_recvmsg_dst_addrs_[client_idx], false));
        }

        // Prepare for cur-slot per-client aggregated statistics
        std::vector<ClientAggregatedStatistics> curslot_perclient_aggregated_statistics;
        curslot_perclient_aggregated_statistics.resize(clientcnt_);

        cur_slot_idx_++;
        std::ostringstream oss;
        oss << "Notify all clients to switch slot into target slot " << cur_slot_idx_;
        Util::dumpNormalMsg(kClassName, oss.str());

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < switchslot_acked_flags.size())
        {
            // Issue SwitchSlotRequests to unacked clients simultaneously
            for (std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::const_iterator iter_for_req = switchslot_acked_flags.begin(); iter_for_req != switchslot_acked_flags.end(); iter_for_req++)
            {
                if (iter_for_req->second == false)
                {
                    SwitchSlotRequest tmp_switch_slot_request(cur_slot_idx_, 0, evaluator_recvmsg_source_addr_);
                    evaluator_sendmsg_socket_client_ptr_->send((MessageBase*)&tmp_switch_slot_request, iter_for_req->first);
                }
            }

            // Receive SwitchSlotResponses for unacked clients
            for (uint32_t i = 0; i < (switchslot_acked_flags.size() - acked_cnt); i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for SwitchSlotResponse!");
                    break; // Wait until all clients are running
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getRequestFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kSwitchSlotResponse);

                    NetworkAddr tmp_dst_addr = control_response_ptr->getSourceAddr();
                    std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::iterator iter = switchslot_acked_flags.find(tmp_dst_addr);
                    if (iter == switchslot_acked_flags.end())
                    {
                        oss.clear();
                        oss.str("");
                        oss << "receive SwitchSlotResponse from network addr " << tmp_dst_addr.toString() << ", which is NOT in the to-be-acked list!";
                        Util::dumpWarnMsg(kClassName, oss.str());
                    }
                    else
                    {
                        if (iter->second == false)
                        {
                            uint32_t client_idx = iter - switchslot_acked_flags.begin();
                            assert(client_idx == control_response_ptr->getSourceIndex());
                            assert(client_idx < clientcnt_);

                            // Add into cur-slot per-client aggregated statistics
                            const SwitchSlotResponse* const switch_slot_response_ptr = static_cast<const SwitchSlotResponse*>(control_response_ptr);
                            assert(cur_slot_idx_ == switch_slot_response_ptr->getTargetSlotIdx());
                            curslot_perclient_aggregated_statistics[client_idx] = switch_slot_response_ptr->getAggregatedStatistics();

                            iter->second = true;
                            acked_cnt++;
                        }
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        oss.clear();
        oss.str("");
        oss << "All clients are switched to target slot " << cur_slot_idx_;
        Util::dumpNormalMsg(kClassName, oss.str());

        // Update per-slot total aggregated statistics
        total_statistics_tracker_ptr_->updatePerslotTotalAggregatedStatistics(curslot_perclient_aggregated_statistics);

        return;
    }

    void EvaluatorWrapper::checkPointers_() const
    {
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        assert(peredge_recvmsg_dst_addrs_ != NULL);
        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);
        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);
        assert(total_statistics_tracker_ptr_ != NULL);
        return;
    }
}
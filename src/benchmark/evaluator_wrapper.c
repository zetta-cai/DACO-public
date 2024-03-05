#include "benchmark/evaluator_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/dynamic_array.h"
#include "common/util.h"
#include "message/message_base.h"
#include "message/control_message.h"
#include "statistics/client_aggregated_statistics.h"

namespace covered
{
    // NOTE: used by exp scripts to verify whether the evaluator has been initialized or done -> MUST be the same as scripts/common.py
    const std::string EvaluatorWrapper::EVALUATOR_FINISH_INITIALIZATION_SYMBOL("Evaluator initialized");
    const std::string EvaluatorWrapper::EVALUATOR_FINISH_BENCHMARK_SYMBOL("Evaluator done");

    // NOTE: do NOT change the following const variables unless you know what you are doing
    // NOTE: you should guarantee the correctness of dedicated corecnt settings in config.json
    //const bool EvaluatorWrapper::IS_HIGH_PRIORITY_FOR_EVALUATOR = true;
    const bool EvaluatorWrapper::IS_HIGH_PRIORITY_FOR_EVALUATOR = false; // TMPDEBUG

    const std::string EvaluatorWrapper::kClassName("EvaluatorWrapper");

    EvaluatorWrapperParam::EvaluatorWrapperParam() : SubthreadParamBase()
    {
        evaluator_cli_ptr_ = NULL;
    }

    EvaluatorWrapperParam::EvaluatorWrapperParam(EvaluatorCLI* evaluator_cli_ptr) : SubthreadParamBase()
    {
        assert(evaluator_cli_ptr != NULL);

        evaluator_cli_ptr_ = evaluator_cli_ptr;
    }

    EvaluatorWrapperParam::~EvaluatorWrapperParam()
    {
        // NOTE: NO need to release evaluator_cli_ptr_, which is maintained outside EvaluatorWrapperParam
    }
    
    EvaluatorCLI* EvaluatorWrapperParam::getEvaluatorCLIPtr() const
    {
        assert(evaluator_cli_ptr_ != NULL);
        return evaluator_cli_ptr_;
    }

    EvaluatorWrapperParam& EvaluatorWrapperParam::operator=(const EvaluatorWrapperParam& other)
    {
        SubthreadParamBase::operator=(other);
        
        evaluator_cli_ptr_ = other.evaluator_cli_ptr_;
        return *this;
    }

    void* EvaluatorWrapper::launchEvaluator(void* evaluator_wrapper_param_ptr)
    {
        assert(evaluator_wrapper_param_ptr != NULL);
        EvaluatorWrapperParam& evaluator_wrapper_param = *((EvaluatorWrapperParam*)evaluator_wrapper_param_ptr);

        EvaluatorCLI* evaluator_cli_ptr = evaluator_wrapper_param.getEvaluatorCLIPtr();
        std::string evaluator_statistics_filepath = Util::getEvaluatorStatisticsFilepath(evaluator_cli_ptr);

        EvaluatorWrapper evaluator(evaluator_cli_ptr->getClientcnt(), evaluator_cli_ptr->getEdgecnt(), evaluator_cli_ptr->getKeycnt(), evaluator_cli_ptr->getWarmupReqcntScale(), evaluator_cli_ptr->getWarmupMaxDurationSec(), evaluator_cli_ptr->getStresstestDurationSec(), evaluator_statistics_filepath);
        evaluator_wrapper_param.markFinishInitialization(); // Such that simulator or prototype will continue to launch cloud, edge, and client nodes

        evaluator.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EvaluatorWrapper::EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt, const uint32_t& keycnt, const uint32_t& warmup_reqcnt_scale, const uint32_t& warmup_max_duration_sec, const uint32_t& stresstest_duration_sec, const std::string& evaluator_statistics_filepath) : clientcnt_(clientcnt), edgecnt_(edgecnt), warmup_reqcnt_(keycnt * warmup_reqcnt_scale), warmup_max_duration_sec_(warmup_max_duration_sec), stresstest_duration_sec_(stresstest_duration_sec), evaluator_statistics_filepath_(evaluator_statistics_filepath)
    {
        is_warmup_phase_ = true;
        target_slot_idx_ = 0;

        // (1) Manage evaluation phases

        // Prepare evaluator recvmsg source addr
        const bool is_private_evaluator_ipstr = false; // NOTE: evaluator communicates with client/edge/cloud via public IP address
        const bool is_launch_evaluator = true; // We are launching logical evaluator node in the current physical machine
        std::string evaluator_ipstr = Config::getEvaluatorIpstr(is_private_evaluator_ipstr, is_launch_evaluator);
        uint16_t evaluator_recvmsg_port = Config::getEvaluatorRecvmsgPort();
        evaluator_recvmsg_source_addr_ = NetworkAddr(evaluator_ipstr, evaluator_recvmsg_port);

        // Prepare client recvmsg dst addrs
        perclient_recvmsg_dst_addrs_ = new NetworkAddr[clientcnt];
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            const bool is_private_client_ipstr = false; // NOTE: evaluator communicates with client via public IP address
            const bool is_launch_client = false; // Just connect client by evaluator instead of launching the client
            std::string tmp_client_ipstr = Config::getClientIpstr(client_idx, clientcnt, is_private_client_ipstr, is_launch_client);
            uint16_t tmp_client_recvmsg_port = Util::getClientRecvmsgPort(client_idx, clientcnt);
            perclient_recvmsg_dst_addrs_[client_idx] = NetworkAddr(tmp_client_ipstr, tmp_client_recvmsg_port);
        }

        // Prepare edge recvmsg dst addrs
        peredge_recvmsg_dst_addrs_ = new NetworkAddr[edgecnt];
        assert(peredge_recvmsg_dst_addrs_ != NULL);
        for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
        {
            const bool is_private_edge_ipstr = false; // NOTE: evaluator communicates with edge via public IP address
            const bool is_launch_edge = false; // Just connect edge by evaluator instead of launching the edge
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_private_edge_ipstr, is_launch_edge);
            uint16_t tmp_edge_recvmsg_port = Util::getEdgeRecvmsgPort(edge_idx, edgecnt);
            peredge_recvmsg_dst_addrs_[edge_idx] = NetworkAddr(tmp_edge_ipstr, tmp_edge_recvmsg_port);
        }

        // Prepare cloud recvmsg dst addr
        const bool is_private_cloud_ipstr = false; // NOTE: evaluator communicates with cloud via public IP address
        const bool is_launch_cloud = false; // Just connect cloud by evaluator instead of launching the cloud
        std::string cloud_ipstr = Config::getCloudIpstr(is_private_cloud_ipstr, is_launch_cloud);
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

        // Monitor cache object hit ratio for warmup and stresstest phases
        const uint32_t client_raw_statistics_slot_interval_sec = Config::getClientRawStatisticsSlotIntervalSec();
        int stable_condition = -1; // 0: achieve warmup reqcnt; 1: achieve warmup max duration
        struct timespec start_timestamp = Util::getCurrentTimespec(); // For max duration of warmup phase and duration of stresstest phase
        struct timespec prev_timestamp = start_timestamp; // For switch slot
        bool is_monitored = false; // Whether to monitor messages for debugging
        while (true)
        {
            usleep(Util::SLEEP_INTERVAL_US);

            struct timespec cur_timestamp = Util::getCurrentTimespec();
            bool with_new_slot_statistics = false;

            // Count down duration for stresstest phase to judge if benchmark is finished
            if (!is_warmup_phase_) // Stresstest phase
            {
                double delta_us_for_finishrun = Util::getDeltaTimeUs(cur_timestamp, start_timestamp);
                if (delta_us_for_finishrun >= SEC2US(stresstest_duration_sec_))
                {
                    Util::dumpNormalMsg(kClassName, "Stop benchmark...");

                    // Notify client/edge/cloud to finish run
                    notifyAllToFinishrun_(); // Update per-slot/stable total aggregated statistics

                    break;
                }
            }

            // Switch cur-slot client raw statistics to track per-slot aggregated statistics
            double delta_us_for_switch_slot = Util::getDeltaTimeUs(cur_timestamp, prev_timestamp);
            if (delta_us_for_switch_slot >= static_cast<double>(SEC2US(client_raw_statistics_slot_interval_sec)))
            {
                // Notify clients to switch cur-slot client raw statistics
                notifyClientsToSwitchSlot_(is_monitored); // Increase target_slot_idx_ by one, update per-slot total aggregated statistics, and update total_reqcnt in TotalStatisticsTracker

                with_new_slot_statistics = true;

                // Update prev_timestamp for the next slot
                prev_timestamp = cur_timestamp;
            }

            // Monitor if cache object hit ratio is stable to finish the warmup phase if any
            if (is_warmup_phase_) // Warmup phase
            {
                // (1) First stable condition: warmup_reqcnt requests have been processed during warmup phase

                const uint64_t total_reqcnt = total_statistics_tracker_ptr_->getTotalReqcnt();
                bool is_achieve_warmup_reqcnt = (total_reqcnt >= static_cast<uint64_t>(warmup_reqcnt_));

                // (2) Second stable condition (if ENABLE_WARMUP_MAX_DURATION is defined): monitor stable global hit ratio (i.e., cache is filled up and total object hit ratio converges) within warmup max duration

                #ifdef ENABLE_WARMUP_MAX_DURATION
                bool is_achieve_warmup_max_duration = false;
                if (is_achieve_warmup_reqcnt) // Achieve warmup_reqcnt requests
                {
                    double delta_us_for_finishwarmup = Util::getDeltaTimeUs(cur_timestamp, start_timestamp);
                    if (delta_us_for_finishwarmup >= SEC2US(warmup_max_duration_sec_))
                    {
                        if (stable_condition == 1)
                        {
                            std::ostringstream oss;
                            oss << "achieve warmup maximum duration of " << warmup_max_duration_sec_ << " seconds with cache object hit ratio of " << total_statistics_tracker_ptr_->getCurslotTotalAggregatedStatistics().getTotalObjectHitRatio() << " -> finish warmup phase";
                            Util::dumpNormalMsg(kClassName, oss.str());
                        }

                        is_achieve_warmup_max_duration = true;
                    } // End of achieving warmup_max_duration_sec_
                    else
                    {
                        is_achieve_warmup_max_duration = total_statistics_tracker_ptr_->isPerSlotTotalAggregatedStatisticsStable();
                    } // End of within warmup_max_duration_sec_
                } // End of achieve warmup_reqcnt requests
                #else
                bool is_achieve_warmup_max_duration = true;
                #endif

                // Find the condition determines the end of warmup phase
                if (stable_condition == -1)
                {
                    if (is_achieve_warmup_reqcnt && !is_achieve_warmup_max_duration)
                    {
                        stable_condition = 1; // Achieve warmup max duration
                    }
                    else if (!is_achieve_warmup_reqcnt && is_achieve_warmup_max_duration)
                    {
                        stable_condition = 0; // Achieve warmup reqcnt
                    }
                    else if (is_achieve_warmup_reqcnt && is_achieve_warmup_max_duration)
                    {
                        stable_condition = 0; // Achieve warmup reqcnt
                    }
                }

                // Finish warmup phase
                if (is_achieve_warmup_reqcnt && is_achieve_warmup_max_duration)
                {
                    // Cache object hit ratio becomes "stable"
                    double stable_object_hit_ratio = total_statistics_tracker_ptr_->getCurslotTotalAggregatedStatistics().getTotalObjectHitRatio();
                    std::ostringstream oss;
                    oss << "cache object hit ratio at the end of warmup phase is " << stable_object_hit_ratio << " (NOT use this cache hit ratio, as some clients already stop issuing requests at the end of warmup phase to keep strict warmup reqcnt limit; will use cache stable results after warmup) -> finish warmup phase";
                    Util::dumpNormalMsg(kClassName, oss.str());

                    // Notify all clients to finish warmup phase (i.e., enter stresstest phase)
                    notifyClientsToFinishWarmup_(); // Mark is_warmup_phase_ as false

                    // Duration starts from the end of warmup phase
                    is_warmup_phase_ = false;

                    // Reset start_timestamp for duration of stresstest phase
                    start_timestamp = Util::getCurrentTimespec();
                } // End of finish warmup phase
            } // End if (is_warmup_phase == true)

            if (with_new_slot_statistics)
            {
                // Print latest-slot total aggregated statistics
                std::ostringstream oss;
                oss << total_statistics_tracker_ptr_->curslotToString(target_slot_idx_ - 1);
                Util::dumpNormalMsg(kClassName, oss.str());
            }
        } // End of while loop


        // Print last-slot/stable total aggregated statistics
        std::ostringstream oss;
        oss << total_statistics_tracker_ptr_->curslotToString(target_slot_idx_) << std::endl;
        oss << total_statistics_tracker_ptr_->stableToString() << std::endl;
        Util::dumpNormalMsg(kClassName, oss.str());

        // Dump per-slot/stable total aggregated statistics
        oss.clear();
        oss.str("");
        oss << "dump per-slot/stable total aggregated statistics into " << evaluator_statistics_filepath_;
        Util::dumpNormalMsg(kClassName, oss.str());
        uint32_t dump_size = total_statistics_tracker_ptr_->dump(evaluator_statistics_filepath_);
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
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> initialization_acked_flags = getAckedFlagsForAll_();

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

                bool is_first_req_to_be_acked = processMsgForAck_(control_request_ptr, initialization_acked_flags);
                if (is_first_req_to_be_acked)
                {
                    // Send back InitializationResponse to client/edge/cloud
                    InitializationResponse initialization_response(0, evaluator_recvmsg_source_addr_, EventList());
                    evaluator_sendmsg_socket_client_ptr_->send((MessageBase*)&initialization_response, control_request_ptr->getSourceAddr());

                    acked_cnt++;
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
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> startrun_acked_flags = getAckedFlagsForClients_();

        Util::dumpNormalMsg(kClassName, "Notify all clients to start running...");

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < startrun_acked_flags.size())
        {
            // Issue StartrunRequests to unacked clients simultaneously
            StartrunRequest tmp_startrun_request(0, evaluator_recvmsg_source_addr_);
            issueMsgToUnackedNodes_((MessageBase*)&tmp_startrun_request, startrun_acked_flags);

            // Receive StartrunResponses for unacked clients
            const uint32_t expected_rspcnt = startrun_acked_flags.size() - acked_cnt;
            for (uint32_t i = 0; i < expected_rspcnt; i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for StartrunResponse from " + Util::getAckedStatusStr(startrun_acked_flags) + "!");
                    break; // Wait until all clients are running
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kStartrunResponse);

                    bool is_first_rsp_for_ack = processMsgForAck_(control_response_ptr, startrun_acked_flags);
                    if (is_first_rsp_for_ack)
                    {
                        acked_cnt++;
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients are running!");

        return;
    }

    void EvaluatorWrapper::notifyClientsToSwitchSlot_(const bool& is_monitored)
    {
        checkPointers_();

        // Client ack flags
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> switchslot_acked_flags = getAckedFlagsForClients_();

        // Prepare for cur-slot per-client aggregated statistics
        std::vector<ClientAggregatedStatistics> curslot_perclient_aggregated_statistics;
        curslot_perclient_aggregated_statistics.resize(clientcnt_);

        target_slot_idx_++;
        std::ostringstream oss;
        oss << "Notify all clients to switch slot into target slot " << target_slot_idx_ << "...";
        Util::dumpNormalMsg(kClassName, oss.str());

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < switchslot_acked_flags.size())
        {
            // Issue SwitchSlotRequests to unacked clients simultaneously
            SwitchSlotRequest tmp_switch_slot_request(target_slot_idx_, 0, evaluator_recvmsg_source_addr_, is_monitored);
            issueMsgToUnackedNodes_((MessageBase*)&tmp_switch_slot_request, switchslot_acked_flags);

            // Receive SwitchSlotResponses for unacked clients    
            const uint32_t expected_rspcnt = switchslot_acked_flags.size() - acked_cnt;
            for (uint32_t i = 0; i < expected_rspcnt; i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for SwitchSlotResponse from " + Util::getAckedStatusStr(switchslot_acked_flags) + "!");
                    break; // Wait until all clients have switched slot
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kSwitchSlotResponse);

                    bool is_first_rsp_for_ack = processMsgForAck_(control_response_ptr, switchslot_acked_flags);
                    if (is_first_rsp_for_ack)
                    {
                        /*// (OBSOLETE as std::unordered_map does NOT follow insertion order) Calculate client idx
                        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>::iterator iter = switchslot_acked_flags.find(control_response_ptr->getSourceAddr());
                        assert(iter != switchslot_acked_flags.end());
                        uint32_t client_idx = static_cast<uint32_t>(std::distance(switchslot_acked_flags.begin(), iter));*/

                        // Calculate client idx
                        uint32_t client_idx = clientcnt_;
                        for (uint32_t i = 0; i < clientcnt_; i++)
                        {
                            if (perclient_recvmsg_dst_addrs_[i] == control_response_ptr->getSourceAddr())
                            {
                                client_idx = i;
                                break;
                            }
                        }

                        // Check client idx
                        assert(client_idx == control_response_ptr->getSourceIndex());
                        assert(client_idx < clientcnt_);

                        // Add into cur-slot per-client aggregated statistics
                        const SwitchSlotResponse* const switch_slot_response_ptr = static_cast<const SwitchSlotResponse*>(control_response_ptr);
                        assert(target_slot_idx_ == switch_slot_response_ptr->getTargetSlotIdx());
                        curslot_perclient_aggregated_statistics[client_idx] = switch_slot_response_ptr->getAggregatedStatistics();

                        acked_cnt++;
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        oss.clear();
        oss.str("");
        oss << "All clients are switched to target slot " << target_slot_idx_;
        Util::dumpNormalMsg(kClassName, oss.str());

        // Update per-slot total aggregated statistics
        total_statistics_tracker_ptr_->updatePerslotTotalAggregatedStatistics(curslot_perclient_aggregated_statistics, Config::getClientRawStatisticsSlotIntervalSec());

        #ifdef DEBUG_EVALUATOR_WRAPPER
        TotalAggregatedStatistics tmp_curslot_total_aggregated_statistics = total_statistics_tracker_ptr_->getCurslotTotalAggregatedStatistics();
        Util::dumpVariablesForDebug(kClassName, 4, "slot idx:", std::to_string(target_slot_idx_ - 1).c_str(), "\n", tmp_curslot_total_aggregated_statistics.toString().c_str());
        #endif

        return;
    }

    void EvaluatorWrapper::notifyClientsToFinishWarmup_()
    {
        checkPointers_();

        // Client ack flags
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> finish_warmup_acked_flags = getAckedFlagsForClients_();

        Util::dumpNormalMsg(kClassName, "Notify all clients to finish warmup and start stresstest...");

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < finish_warmup_acked_flags.size())
        {
            // Issue FinishWarmupRequests to unacked clients simultaneously
            FinishWarmupRequest tmp_finish_warmup_request(0, evaluator_recvmsg_source_addr_);
            issueMsgToUnackedNodes_((MessageBase*)&tmp_finish_warmup_request, finish_warmup_acked_flags);

            // Receive FinishWarmupResponses for unacked clients
            const uint32_t expected_rspcnt = finish_warmup_acked_flags.size() - acked_cnt;
            for (uint32_t i = 0; i < expected_rspcnt; i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for FinishWarmupResponse from " + Util::getAckedStatusStr(finish_warmup_acked_flags) + "!");
                    break; // Wait until all clients have finished warmup phase
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kFinishWarmupResponse);

                    bool is_first_rsp_for_ack = processMsgForAck_(control_response_ptr, finish_warmup_acked_flags);
                    if (is_first_rsp_for_ack)
                    {
                        acked_cnt++;
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients have finished warmup phase!");

        is_warmup_phase_ = false;

        return;
    }

    void EvaluatorWrapper::notifyAllToFinishrun_()
    {
        // Notify all clients to finish running, and update per-slot/stable total aggregated statistics
        notifyClientsToFinishrun_();

        // After clients finish running, notify edge/cloud nodes to finish running
        notifyEdgeCloudToFinishrun_();
    }

    void EvaluatorWrapper::notifyClientsToFinishrun_()
    {
        checkPointers_();

        // Client ack flags
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> finishrun_acked_flags = getAckedFlagsForClients_();

        // Prepare for last-slot/stable per-client aggregated statistics
        std::vector<ClientAggregatedStatistics> lastslot_perclient_aggregated_statistics;
        lastslot_perclient_aggregated_statistics.resize(clientcnt_);
        std::vector<ClientAggregatedStatistics> stable_perclient_aggregated_statistics;
        stable_perclient_aggregated_statistics.resize(clientcnt_);

        Util::dumpNormalMsg(kClassName, "Notify all clients to finish run...");

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < finishrun_acked_flags.size())
        {
            // Issue FinishrunRequests to unacked clients simultaneously
            FinishrunRequest tmp_finishrun_request(0, evaluator_recvmsg_source_addr_);
            issueMsgToUnackedNodes_((MessageBase*)&tmp_finishrun_request, finishrun_acked_flags);

            // Receive FinishrunResponses for unacked clients
            const uint32_t expected_rspcnt = finishrun_acked_flags.size() - acked_cnt;
            for (uint32_t i = 0; i < expected_rspcnt; i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for FinishrunResponse from " + Util::getAckedStatusStr(finishrun_acked_flags) + "!");
                    break; // Wait until all clients have finished running
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kFinishrunResponse);

                    bool is_first_rsp_for_ack = processMsgForAck_(control_response_ptr, finishrun_acked_flags);
                    if (is_first_rsp_for_ack)
                    {
                        /*// (OBSOLETE as std::unordered_map does NOT follow insertion order) Calculate client idx
                        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>::iterator iter = finishrun_acked_flags.find(control_response_ptr->getSourceAddr());
                        assert(iter != finishrun_acked_flags.end());
                        uint32_t client_idx = static_cast<uint32_t>(std::distance(finishrun_acked_flags.begin(), iter));*/

                        // Calculate client idx
                        uint32_t client_idx = clientcnt_;
                        for (uint32_t i = 0; i < clientcnt_; i++)
                        {
                            if (perclient_recvmsg_dst_addrs_[i] == control_response_ptr->getSourceAddr())
                            {
                                client_idx = i;
                                break;
                            }
                        }

                        // Check client idx
                        assert(client_idx == control_response_ptr->getSourceIndex());
                        assert(client_idx < clientcnt_);

                        // Add into last-slot/stable per-client aggregated statistics
                        const FinishrunResponse* const finishrun_response_ptr = static_cast<const FinishrunResponse*>(control_response_ptr);
                        assert(target_slot_idx_ == finishrun_response_ptr->getLastSlotIdx());
                        lastslot_perclient_aggregated_statistics[client_idx] = static_cast<ClientAggregatedStatistics>(finishrun_response_ptr->getLastSlotAggregatedStatistics());
                        stable_perclient_aggregated_statistics[client_idx] = finishrun_response_ptr->getStableAggregatedStatistics();

                        acked_cnt++;
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients have finished running!");

        // Update per-slot total aggregated statistics
        total_statistics_tracker_ptr_->updatePerslotTotalAggregatedStatistics(lastslot_perclient_aggregated_statistics, Config::getClientRawStatisticsSlotIntervalSec());

        // Update stable total aggregated statistics
        total_statistics_tracker_ptr_->updateStableTotalAggregatedStatistics(stable_perclient_aggregated_statistics, stresstest_duration_sec_);

        return;
    }

    void EvaluatorWrapper::notifyEdgeCloudToFinishrun_()
    {
        checkPointers_();

        // Edge/cloud ack flags
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> finishrun_acked_flags = getAckedFlagsForEdgeCloud_();

        Util::dumpNormalMsg(kClassName, "Notify all edge/cloud nodes to finish run...");

        // Timeout-and-retry mechanism
        uint32_t acked_cnt = 0;
        while (acked_cnt < finishrun_acked_flags.size())
        {
            // Issue FinishrunRequests to unacked edge/cloud nodes simultaneously
            FinishrunRequest tmp_finishrun_request(0, evaluator_recvmsg_source_addr_);
            issueMsgToUnackedNodes_((MessageBase*)&tmp_finishrun_request, finishrun_acked_flags);

            // Receive SimpleFinishrunResponses for unacked edge/cloud nodes
            const uint32_t expected_rspcnt = finishrun_acked_flags.size() - acked_cnt;
            for (uint32_t i = 0; i < expected_rspcnt; i++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    Util::dumpWarnMsg(kClassName, "timeout to wait for SimpleFinishrunResponse from " + Util::getAckedStatusStr(finishrun_acked_flags) + "!");
                    break; // Wait until all edge/cloud nodes have finished running
                }
                else
                {
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    assert(control_response_ptr->getMessageType() == MessageType::kSimpleFinishrunResponse);

                    bool is_first_rsp_for_ack = processMsgForAck_(control_response_ptr, finishrun_acked_flags);
                    if (is_first_rsp_for_ack)
                    {
                        acked_cnt++;
                    }

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                }
            }
        }

        Util::dumpNormalMsg(kClassName, "All edge/cloud nodes have finished running!");

        return;
    }

    std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> EvaluatorWrapper::getAckedFlagsForAll_() const
    {
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> acked_flags_for_clients = getAckedFlagsForClients_();
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> acked_flags_for_edgeclout = getAckedFlagsForEdgeCloud_();

        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> acked_flags_for_all;
        acked_flags_for_all.insert(acked_flags_for_clients.begin(), acked_flags_for_clients.end());
        acked_flags_for_all.insert(acked_flags_for_edgeclout.begin(), acked_flags_for_edgeclout.end());

        return acked_flags_for_all;
    }

    std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> EvaluatorWrapper::getAckedFlagsForClients_() const
    {
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> acked_flags_for_clients;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            acked_flags_for_clients.insert(std::pair<NetworkAddr, std::pair<bool, std::string>>(perclient_recvmsg_dst_addrs_[client_idx], std::pair(false, "client " + std::to_string(client_idx))));
        }
        return acked_flags_for_clients;
    }

    std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> EvaluatorWrapper::getAckedFlagsForEdgeCloud_() const
    {
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> acked_flags_for_edgeclout;
        for (uint32_t edge_idx = 0; edge_idx < edgecnt_; edge_idx++)
        {
            acked_flags_for_edgeclout.insert(std::pair<NetworkAddr, std::pair<bool, std::string>>(peredge_recvmsg_dst_addrs_[edge_idx], std::pair(false, "edge " + std::to_string(edge_idx))));
        }
        acked_flags_for_edgeclout.insert(std::pair<NetworkAddr, std::pair<bool, std::string>>(cloud_recvmsg_dst_addr_, std::pair(false, "cloud")));
        return acked_flags_for_edgeclout;
    }

    void EvaluatorWrapper::issueMsgToUnackedNodes_(MessageBase* message_ptr, const std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>& acked_flags) const
    {
        assert(message_ptr != NULL);

        for (std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>::const_iterator iter_for_req = acked_flags.begin(); iter_for_req != acked_flags.end(); iter_for_req++)
        {
            if (iter_for_req->second.first == false)
            {
                evaluator_sendmsg_socket_client_ptr_->send(message_ptr, iter_for_req->first);
            }
        }

        return;
    }

    bool EvaluatorWrapper::processMsgForAck_(MessageBase* message_ptr, std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>& acked_flags)
    {
        assert(message_ptr != NULL);

        bool is_first_msg = false;
        NetworkAddr tmp_dst_addr = message_ptr->getSourceAddr();
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>::iterator iter = acked_flags.find(tmp_dst_addr);
        if (iter == acked_flags.end())
        {
            std::ostringstream oss;
            oss << "receive " << MessageBase::messageTypeToString(message_ptr->getMessageType()) << " from network addr " << tmp_dst_addr.toString() << ", which is NOT in the to-be-acked list!";
            Util::dumpWarnMsg(kClassName, oss.str());
        }
        else
        {
            if (iter->second.first == false)
            {
                iter->second.first = true;
                is_first_msg = true;
            }
        }

        return is_first_msg;
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
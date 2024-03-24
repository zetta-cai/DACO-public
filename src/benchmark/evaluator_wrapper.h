/*
 * EvaluatorWrapper: an evaluation controller to control the evaluation phases during benchmark.
 *
 * NOTE: evaluator notifies each client wrapper to switch and update cur-slot client raw statistics, monitors per-slot total aggregated statistics to finish warmup phase for stresstest.
 * 
 * By Siyuan Sheng (2023.07.25).
 */

#ifndef EVALUATOR_WRAPPER_H
#define EVALUATOR_WRAPPER_H

//#define DEBUG_EVALUATOR_WRAPPER

#include <string>
#include <unordered_map>

#include "common/subthread_param_base.h"
#include "cli/evaluator_cli.h"
#include "network/network_addr.h"
#include "network/udp_msg_socket_client.h"
#include "network/udp_msg_socket_server.h"
#include "statistics/total_statistics_tracker.h"

namespace covered
{
    class EvaluatorWrapperParam : public SubthreadParamBase
    {
    public:
        EvaluatorWrapperParam();
        EvaluatorWrapperParam(EvaluatorCLI* evaluator_cli_ptr);
        ~EvaluatorWrapperParam();

        EvaluatorCLI* getEvaluatorCLIPtr() const;

        EvaluatorWrapperParam& operator=(const EvaluatorWrapperParam& other);
    private:
        EvaluatorCLI* evaluator_cli_ptr_;
    };

    class EvaluatorWrapper
    {
    public:
        // NOTE: used by exp scripts to verify whether the evaluator has been initialized or done -> MUST be the same as scripts/common.py
        static const std::string EVALUATOR_FINISH_INITIALIZATION_SYMBOL;
        static const std::string EVALUATOR_FINISH_BENCHMARK_SYMBOL;

        static const bool IS_HIGH_PRIORITY_FOR_EVALUATOR;
        
        static void* launchEvaluator(void* evaluator_wrapper_param_ptr);

        EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt, const uint32_t& keycnt, const uint32_t& warmup_reqcnt_scale, const uint32_t& warmup_max_duration_sec, const uint32_t& stresstest_duration_sec, const std::string& evaluator_statistics_filepath, const std::string& realnet_option);
        ~EvaluatorWrapper();

        void start();
    private:
        static const std::string kClassName;

        // (1) Before evaluation
        void blockForInitialization_();
        void notifyClientsToStartrun_();

        // (2) Different evaluation cases
        void normalEval_(); // Normal evaluation for most cases
        void realnetDumpEval_(); // Real-net dump evaluation for real-network exp warmup
        void realnetLoadEval_(); // Real-net load evaluation for real-network exp stresstest

        // (3) Evaluation helpers
        void notifyClientsToSwitchSlot_(const bool& is_monitored);
        bool checkWarmupStatus_(const struct timespec& start_timestamp, const struct timespec& cur_timestamp);
        void notifyClientsToFinishWarmup_();
        void notifyEdgesToDumpSnapshot_(); // Only for realnet dump evaluation
        void notifyAllToFinishrun_(); // Finish clients first, and then edge and cloud
        void notifyClientsToFinishrun_(); // Update per-slot/stable total aggregated statistics
        void notifyEdgeCloudToFinishrun_();

        // NOTE: for the pair of bool and std::string, bool refers to whether receiving the ACK flag, while std::string refers to the node identification information for debugging
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> getAckedFlagsForAll_() const;
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> getAckedFlagsForClients_() const;
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> getAckedFlagsForEdgeCloud_() const;
        std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher> getAckedFlagsForEdges_() const;
        void issueMsgToUnackedNodes_(MessageBase* message_ptr, const std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>& acked_flags) const;
        bool processMsgForAck_(MessageBase* message_ptr, std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>& acked_flags);

        void checkPointers_() const;

        // Const variables
        const uint32_t clientcnt_; // Come from CLI
        const uint32_t edgecnt_; // Come from CLI
        const uint32_t warmup_reqcnt_; // Calculated from CLI::keycnt and CLI::warmup_reqcnt_scale
        const uint32_t warmup_max_duration_sec_; // Come from CLI
        const uint32_t stresstest_duration_sec_; // Come from CLI
        const std::string evaluator_statistics_filepath_; // Calculated based on CLI
        const std::string realnet_option_; // Come from CLI

        // (1) Manage evaluation phases

        bool is_warmup_phase_;
        uint32_t target_slot_idx_;

        NetworkAddr evaluator_recvmsg_source_addr_;
        NetworkAddr* perclient_recvmsg_dst_addrs_;
        NetworkAddr* peredge_recvmsg_dst_addrs_;
        NetworkAddr cloud_recvmsg_dst_addr_; // TODO: only support one cloud node now

        // NOTE: NOT simulator propagation latency for benchmark control messages
        UdpMsgSocketServer* evaluator_recvmsg_socket_server_ptr_;
        UdpMsgSocketClient* evaluator_sendmsg_socket_client_ptr_;

        // (2) Track total aggregated statistics

        TotalStatisticsTracker* total_statistics_tracker_ptr_;

        // To fix duplicate response issue
        std::atomic<uint64_t> evaluator_msg_seqnum_;
    };
}

#endif
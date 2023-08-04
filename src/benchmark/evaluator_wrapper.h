/*
 * EvaluatorWrapper: an evaluation controller to control the evaluation phases during benchmark.
 *
 * NOTE: evaluator notifies each client wrapper to switch and update cur-slot client raw statistics, monitors per-slot total aggregated statistics to finish warmup phase for stresstest.
 * 
 * By Siyuan Sheng (2023.07.25).
 */

#ifndef EVALUATOR_WRAPPER_H
#define EVALUATOR_WRAPPER_H

#define DEBUG_EVALUATOR_WRAPPER

#include <string>
#include <unordered_map>

#include "common/cli/evaluator_cli.h"
#include "network/network_addr.h"
#include "network/udp_msg_socket_client.h"
#include "network/udp_msg_socket_server.h"
#include "statistics/total_statistics_tracker.h"

namespace covered
{
    class EvaluatorWrapperParam
    {
    public:
        EvaluatorWrapperParam();
        EvaluatorWrapperParam(const bool& is_evaluator_initialized, EvaluatorCLI* evaluator_cli_ptr);
        ~EvaluatorWrapperParam();

        bool isEvaluatorInitialized() const;
        EvaluatorCLI* getEvaluatorCLIPtr() const;

        void setEvaluatorInitialized();

        EvaluatorWrapperParam& operator=(const EvaluatorWrapperParam& other);
    private:
        volatile bool is_evaluator_initialized_;
        EvaluatorCLI* evaluator_cli_ptr_;
    };

    class EvaluatorWrapper
    {
    public:
        static void* launchEvaluator(void* evaluator_wrapper_param_ptr);

        EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt, const uint32_t& max_warmup_duration_sec, const uint32_t& stresstest_duration_sec, const std::string& evaluator_statistics_filepath);
        ~EvaluatorWrapper();

        void start();
    private:
        static const std::string kClassName;

        void blockForInitialization_();
        void notifyClientsToStartrun_();
        void notifyClientsToSwitchSlot_();
        void notifyClientsToFinishWarmup_();
        void notifyAllToFinishrun_(); // Finish clients first, and then edge and cloud
        void notifyClientsToFinishrun_(); // Update per-slot/stable total aggregated statistics
        void notifyEdgeCloudToFinishrun_();

        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> getAckedFlagsForAll_() const;
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> getAckedFlagsForClients_() const;
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> getAckedFlagsForEdgeCloud_() const;
        void issueMsgToUnackedNodes_(MessageBase* message_ptr, const std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>& acked_flags) const;
        bool processMsgForAck_(MessageBase* message_ptr, std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>& acked_flags);

        void checkPointers_() const;

        // Const variables
        const uint32_t clientcnt_; // Come from CLI
        const uint32_t edgecnt_; // Come from CLI
        const uint32_t max_warmup_duration_sec_; // Come from CLI
        const uint32_t stresstest_duration_sec_; // Come from CLI
        const std::string evaluator_statistics_filepath_; // Calculated based on CLI

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
    };
}

#endif
/*
 * EvaluatorWrapper: an evaluation controller to control the evaluation phases during benchmark.
 * 
 * By Siyuan Sheng (2023.07.25).
 */

#ifndef EVALUATOR_WRAPPER_H
#define EVALUATOR_WRAPPER_H

#include <string>

#include "network/network_addr.h"
#include "network/udp_msg_socket_client.h"
#include "network/udp_msg_socket_server.h"
#include "statistics/total_statistics_tracker.h"

namespace covered
{
    class EvaluatorWrapper
    {
    public:
        static void* launchEvaluator(void* is_evaluator_initialized_ptr);

        EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt);
        ~EvaluatorWrapper();

        void start();
    private:
        static const std::string kClassName;

        void blockForInitialization_();

        void checkPointers_() const;

        // Const variables
        const uint32_t clientcnt_;
        const uint32_t edgecnt_;

        // (1) Manage evaluation phases

        bool is_warmup_phase_;

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
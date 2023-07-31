/*
 * NodeWrapperBase: the base class for client/edge/cloud wrapper (thread safe).
 * 
 * By Siyuan Sheng (2023.07.26).
 */

#ifndef NODE_WRAPPER_BASE_H
#define NODE_WRAPPER_BASE_H

#include <atomic>
#include <string>

#include "network/network_addr.h"
#include "network/udp_msg_socket_client.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class NodeWrapperBase
    {
    public:
        static const std::string CLIENT_NODE_ROLE;
        static const std::string EDGE_NODE_ROLE;
        static const std::string CLOUD_NODE_ROLE;

        NodeWrapperBase(const std::string& node_role, const uint32_t& node_idx, const uint32_t& node_cnt, const bool& is_running);
        virtual ~NodeWrapperBase();

        void start();

        uint32_t getNodeIdx() const;
        uint32_t getNodeCnt() const;
        std::string getNodeRoleIdxStr() const;

        bool isNodeRunning() const;
    private:
        static const std::string kClassName;

        virtual void initialize_() = 0;
        void finishInitialization_() const;
        void blockForStartrun_();
        void blockForFinishrun_();
        virtual void processFinishrunRequest_() = 0;
        virtual void processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr) = 0;
        virtual void cleanup_() = 0;

        // Const individual variable
        std::string base_instance_name_;
    protected:
        void setNodeRunning_();
        void resetNodeRunning_();

        void checkPointers_() const;

        // Const shared variable
        std::string node_role_;
        uint32_t node_idx_;
        uint32_t node_cnt_;
        std::string node_role_idx_str_;

        // Atomicity: atomic load/store.
        // Concurrency control: acquire-release ordering/consistency.
        // CPU cache coherence: MSI protocol.
        // CPU cache consistency: volatile.
        volatile std::atomic<bool> node_running_; // thread safe

        // Non-const individual variables for benchmark control messages
        NetworkAddr node_recvmsg_source_addr_;
        NetworkAddr evaluator_recvmsg_dst_addr_;
        UdpMsgSocketServer* node_recvmsg_socket_server_ptr_;
        UdpMsgSocketClient* node_sendmsg_socket_client_ptr_;
    };
}

#endif
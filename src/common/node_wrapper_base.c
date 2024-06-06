#include "common/node_wrapper_base.h"

#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string NodeWrapperBase::CLIENT_NODE_ROLE("client");
    const std::string NodeWrapperBase::EDGE_NODE_ROLE("edge");
    const std::string NodeWrapperBase::CLOUD_NODE_ROLE("cloud");

    const std::string NodeWrapperBase::kClassName("NodeWrapperBase");

    NodeWrapperBase::NodeWrapperBase(const std::string& node_role, const uint32_t& node_idx, const uint32_t& node_cnt, const bool& is_running) : node_role_(node_role), node_idx_(node_idx), node_cnt_(node_cnt), node_running_(is_running), node_msg_seqnum_(0)
    {
        std::ostringstream oss;
        oss << node_role << node_idx;
        node_role_idx_str_ = oss.str();

        // Differentiate different NodeWrapperBases
        oss.clear();
        oss.str("");
        oss << kClassName << " " << node_role_idx_str_;
        base_instance_name_ = oss.str();

        // For benchmark control messages

        std::string node_ipstr = "";
        uint16_t node_recvmsg_port = 0;
        const bool is_private_node_ipstr = false; // NOTE: client/edge/cloud communicates with evaluator via public IP addresses
        const bool is_launch_node = true; // We are launching a logical node in the current physical machine
        if (node_role == CLIENT_NODE_ROLE)
        {
            node_ipstr = Config::getClientIpstr(node_idx, node_cnt, is_private_node_ipstr, is_launch_node);
            node_recvmsg_port = Util::getClientRecvmsgPort(node_idx, node_cnt);
        }
        else if (node_role == EDGE_NODE_ROLE)
        {
            node_ipstr = Config::getEdgeIpstr(node_idx, node_cnt, is_private_node_ipstr, is_launch_node);
            node_recvmsg_port = Util::getEdgeRecvmsgPort(node_idx, node_cnt);
        }
        else if (node_role == CLOUD_NODE_ROLE)
        {
            node_ipstr = Config::getCloudIpstr(is_private_node_ipstr, is_launch_node);
            node_recvmsg_port = Util::getCloudRecvmsgPort(node_idx);
        }
        else
        {
            oss.clear();
            oss.str("");
            oss << "invalid node role: " << node_role << "!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        node_recvmsg_source_addr_ = NetworkAddr(node_ipstr, node_recvmsg_port);

        const bool is_private_evaluator_ipstr = false; // NOTE: client/edge/cloud communicates with evaluator via public IP addresses
        const bool is_launch_evaluator = false;
        std::string evaluator_ipstr = Config::getEvaluatorIpstr(is_private_evaluator_ipstr, is_launch_evaluator);
        uint16_t evaluator_recvmsg_port = Config::getEvaluatorRecvmsgPort();
        evaluator_recvmsg_dst_addr_ = NetworkAddr(evaluator_ipstr, evaluator_recvmsg_port);
        
        NetworkAddr host_addr(Util::ANY_IPSTR, node_recvmsg_port);
        node_recvmsg_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(node_recvmsg_socket_server_ptr_ != NULL);

        node_sendmsg_socket_client_ptr_ = new UdpMsgSocketClient();
        assert(node_sendmsg_socket_client_ptr_ != NULL);
    }

    NodeWrapperBase::~NodeWrapperBase()
    {
        // For benchmark control messages

        assert(node_recvmsg_socket_server_ptr_ != NULL);
        delete node_recvmsg_socket_server_ptr_;
        node_recvmsg_socket_server_ptr_ = NULL;

        assert(node_sendmsg_socket_client_ptr_ != NULL);
        delete node_sendmsg_socket_client_ptr_;
        node_sendmsg_socket_client_ptr_ = NULL;
    }

    void NodeWrapperBase::start()
    {
        initialize_();

        // After all time-consuming initialization
        finishInitialization_();

        if (node_role_ == CLIENT_NODE_ROLE)
        {
            // Block until receiving startrun request
            blockForStartrun_(); // Set node_running_ as true
        }

        blockForFinishrun_();

        cleanup_();
    }

    uint32_t NodeWrapperBase::getNodeIdx() const
    {
        return node_idx_;
    }

    uint32_t NodeWrapperBase::getNodeCnt() const
    {
        return node_cnt_;
    }

    std::string NodeWrapperBase::getNodeRoleIdxStr() const
    {
        return node_role_idx_str_;
    }

    uint64_t NodeWrapperBase::getAndIncrNodeMsgSeqnum() const
    {
        uint64_t original_node_msg_seqnum = node_msg_seqnum_.fetch_add(1, Util::RMW_CONCURRENCY_ORDER);
        return original_node_msg_seqnum;
    }

    void NodeWrapperBase::finishInitialization_() const
    {
        Util::dumpNormalMsg(base_instance_name_, "issue initialization request to finish initialization...");

        // Prepare InitializationRequest
        const uint64_t cur_msg_seqnum = getAndIncrNodeMsgSeqnum();
        InitializationRequest initialization_request(node_idx_, node_recvmsg_source_addr_, cur_msg_seqnum);

        // Timeout-and-retry mechanism
        bool is_stale_response = false; // Only recv again instead of send if with a stale response
        while (true)
        {
            if (!is_stale_response)
            {
                // Issue InitializationRequest to evaluator
                node_sendmsg_socket_client_ptr_->send((MessageBase*)&initialization_request, evaluator_recvmsg_dst_addr_);
            }
        
            DynamicArray control_response_msg_payload;
            bool is_timeout = node_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                Util::dumpWarnMsg(base_instance_name_, "timeout to wait for InitializationResponse from evaluator");
                is_stale_response = false; // Reset to re-send request
                continue; // Wait until receiving InitializationResponse from evaluator
            }
            else
            {
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                // Check if the received message is a stale response
                if (control_response_ptr->getExtraCommonMsghdr().getMsgSeqnum() != cur_msg_seqnum)
                {
                    is_stale_response = true; // ONLY recv again instead of send if with a stale response

                    std::ostringstream oss_for_stable_response;
                    oss_for_stable_response << "stale response " << MessageBase::messageTypeToString(control_response_ptr->getMessageType()) << " with seqnum " << control_response_ptr->getExtraCommonMsghdr().getMsgSeqnum() << " != " << cur_msg_seqnum;
                    Util::dumpWarnMsg(base_instance_name_, oss_for_stable_response.str());

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                    continue; // Jump to while loop
                }

                assert(control_response_ptr->getMessageType() == MessageType::kInitializationResponse);

                delete control_response_ptr;
                control_response_ptr = NULL;
                
                break;
            }
        }

        Util::dumpNormalMsg(base_instance_name_, "receive initialization response!");

        return;
    }

    void NodeWrapperBase::blockForStartrun_()
    {
        assert(node_role_ == CLIENT_NODE_ROLE);

        // Wait for StartrunRequest from evaluator
        while (true)
        {
            DynamicArray control_request_msg_payload;
            bool is_timeout = node_recvmsg_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                continue; // Wait until receiving StartrunRequest from evaluator
            }
            else
            {
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);
                assert(control_request_ptr->getMessageType() == MessageType::kStartrunRequest);

                // Send back StartrunResponse to evaluator
                StartrunResponse startrun_response(node_idx_, node_recvmsg_source_addr_, EventList(), control_request_ptr->getExtraCommonMsghdr().getMsgSeqnum());
                node_sendmsg_socket_client_ptr_->send((MessageBase*)&startrun_response, evaluator_recvmsg_dst_addr_);

                delete control_request_ptr;
                control_request_ptr = NULL;
                
                break;
            }
        }

        // Mark the current node as running
        setNodeRunning_();

        return;
    }

    void NodeWrapperBase::blockForFinishrun_()
    {
        // Wait for FinishRunRequest from evaluator
        while (isNodeRunning())
        {
            DynamicArray control_request_msg_payload;
            bool is_timeout = node_recvmsg_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                continue; // Continue to wait for FinishRunRequest if client is still running
            }
            else
            {
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);
                
                MessageType control_request_msg_type = control_request_ptr->getMessageType();
                if (control_request_msg_type == MessageType::kFinishrunRequest)
                {
                    processFinishrunRequest_(control_request_ptr); // Mark node_running_ as false
                }
                else
                {
                    processOtherBenchmarkControlRequest_(control_request_ptr);
                }

                delete control_request_ptr;
                control_request_ptr = NULL;
            }
        }
    }

    bool NodeWrapperBase::isNodeRunning() const
    {
        return node_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void NodeWrapperBase::setNodeRunning_()
    {
        node_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
        return;
    }

    void NodeWrapperBase::resetNodeRunning_()
    {
        node_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
        return;
    }

    void NodeWrapperBase::checkPointers_() const
    {
        assert(node_recvmsg_socket_server_ptr_ != NULL);
        assert(node_sendmsg_socket_client_ptr_ != NULL);
        return;
    }
}
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

    NodeWrapperBase::NodeWrapperBase(const std::string& node_role, const uint32_t& node_idx, const uint32_t& node_cnt, const bool& is_running) : node_role_(node_role), node_idx_(node_idx), node_cnt_(node_cnt), node_running_(is_running)
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
        if (node_role == CLIENT_NODE_ROLE)
        {
            node_ipstr = Config::getClientIpstr(node_idx, node_cnt);
            node_recvmsg_port = Util::getClientRecvmsgPort(node_idx, node_cnt);
        }
        else if (node_role == EDGE_NODE_ROLE)
        {
            node_ipstr = Config::getEdgeIpstr(node_idx, node_cnt);
            node_recvmsg_port = Util::getEdgeRecvmsgPort(node_idx, node_cnt);
        }
        else if (node_role == CLOUD_NODE_ROLE)
        {
            node_ipstr = Config::getCloudIpstr();
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

        std::string evaluator_ipstr = Config::getEvaluatorIpstr();
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

        if (node_running_ == false)
        {
            assert(node_role_ == CLIENT_NODE_ROLE);
            
            // Block until receiving startrun request
            blockForStartrun_();
        }

        startInternal_();
    }

    void NodeWrapperBase::finishInitialization_() const
    {
        // Issue a InitializationRequest to evaluator
        InitializationRequest initialization_request(node_idx_, node_recvmsg_source_addr_);
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&initialization_request, evaluator_recvmsg_dst_addr_);

        // Wait for InitializationResponse
        while (true)
        {
            DynamicArray control_response_msg_payload;
            bool is_timeout = node_recvmsg_socket_server_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                continue; // Wait until receiving InitializationResponse
            }
            else
            {
                MessageBase* control_response_ptr = MessageBase::getRequestFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);
                assert(control_response_ptr->getMessageType() == MessageType::kInitializationResponse);

                delete control_response_ptr;
                control_response_ptr = NULL;
                
                break;
            }
        }
        return;
    }

    void NodeWrapperBase::blockForStartrun_()
    {
        assert(node_role_ == CLIENT_NODE_ROLE);

        // TODO

        // setNodeRunning()
    }

    bool NodeWrapperBase::isNodeRunning_() const
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
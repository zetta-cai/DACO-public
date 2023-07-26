#include "edge/invalidation_server/invalidation_server_base.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/invalidation_server/basic_invalidation_server.h"
#include "edge/invalidation_server/covered_invalidation_server.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string InvalidationServerBase::kClassName("InvalidationServerBase");

    InvalidationServerBase* InvalidationServerBase::getInvalidationServerByCacheName(EdgeWrapper* edge_wrapper_ptr)
    {
        InvalidationServerBase* invalidation_server_ptr = NULL;

        assert(edge_wrapper_ptr != NULL);
        std::string cache_name = edge_wrapper_ptr->cache_name_;
        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            invalidation_server_ptr = new CoveredInvalidationServer(edge_wrapper_ptr);
        }
        else
        {
            invalidation_server_ptr = new BasicInvalidationServer(edge_wrapper_ptr);
        }

        assert(invalidation_server_ptr != NULL);
        return invalidation_server_ptr;
    }

    InvalidationServerBase::InvalidationServerBase(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        const uint32_t edge_idx = edge_wrapper_ptr->node_idx_;
        const uint32_t edgecnt = edge_wrapper_ptr->edgecnt_;

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        // For receiving invalidation requests

        // Get source address of invalidation server recvreq
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_invalidation_server_recvreq_port = Util::getEdgeInvalidationServerRecvreqPort(edge_idx, edgecnt);
        edge_invalidation_server_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_invalidation_server_recvreq_port);

        // Prepare a socket server on recvreq port for invalidation server
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, edge_invalidation_server_recvreq_port);
        edge_invalidation_server_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(edge_invalidation_server_recvreq_socket_server_ptr_ != NULL);
    }

    InvalidationServerBase::~InvalidationServerBase()
    {
        // NOTE: no need to release edge_wrapper_ptr_, which will be released outside InvalidationServerBase (e.g., simulator)

        // Release the socket server on recvreq port
        assert(edge_invalidation_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_invalidation_server_recvreq_socket_server_ptr_;
        edge_invalidation_server_recvreq_socket_server_ptr_ = NULL;
    }

    void InvalidationServerBase::start()
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_invalidation_server_recvreq_socket_server_ptr_ != NULL);
        
        bool is_finish = false; // Mark if edge node is finished
        while (edge_wrapper_ptr_->isNodeRunning_()) // edge_running_ is set as true by default
        {
            // Receive the message payload of control requests
            DynamicArray control_request_msg_payload;
            bool is_timeout = edge_invalidation_server_recvreq_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {   
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);

                if (control_request_ptr->getMessageType() == MessageType::kInvalidationRequest)
                {
                    NetworkAddr recvrsp_dst_addr = control_request_ptr->getSourceAddr(); // cache server worker or beacon server
                    is_finish = processInvalidationRequest_(control_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Release messages
                assert(control_request_ptr != NULL);
                delete control_request_ptr;
                control_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_timeout == false)
        } // End of while loop

        return;
    }

    void InvalidationServerBase::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);

        edge_wrapper_ptr_->checkPointers_();
        
        assert(edge_invalidation_server_recvreq_socket_server_ptr_ != NULL);
    }
}
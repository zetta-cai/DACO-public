#include "edge/beacon_server/beacon_server_base.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/beacon_server/basic_beacon_server.h"
#include "edge/beacon_server/covered_beacon_server.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BeaconServerBase::kClassName("BeaconServerBase");

    BeaconServerBase* BeaconServerBase::getBeaconServer(EdgeWrapper* edge_wrapper_ptr)
    {
        BeaconServerBase* beacon_server_ptr = NULL;

        assert(edge_wrapper_ptr != NULL);
        std::string cache_name = edge_wrapper_ptr->cache_name_;
        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            beacon_server_ptr = new CoveredBeaconServer(edge_wrapper_ptr);
        }
        else
        {
            beacon_server_ptr = new BasicBeaconServer(edge_wrapper_ptr);
        }

        assert(beacon_server_ptr != NULL);
        return beacon_server_ptr;
    }

    BeaconServerBase::BeaconServerBase(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        // Prepare a socket server on recvreq port for beacon server
        uint16_t edge_beacon_server_recvreq_port = Util::getEdgeBeaconServerRecvreqPort(edge_idx);
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_beacon_server_recvreq_port);
        edge_beacon_server_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);

        // NOTE: we use edge0 as default remote address, but CooperationWrapper will reset remote address of the socket clients based on BlockTracker later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_cache_server_port = Util::getEdgeCacheServerRecvreqPort(0);
        NetworkAddr edge0_cache_server_addr(edge0_ipstr, edge0_cache_server_port);

        // Prepare a socket client to blocked edge nodes for beacon server
        edge_beacon_server_sendreq_toblocked_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_cache_server_addr);
        assert(edge_beacon_server_sendreq_toblocked_socket_client_ptr_ != NULL);
    }

    BeaconServerBase::~BeaconServerBase()
    {
        // NOTE: no need to release edge_wrapper_ptr_, which will be released outside BeaconServerBase (e.g., simulator)

        // Release the socket server on recvreq port
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_beacon_server_recvreq_socket_server_ptr_;
        edge_beacon_server_recvreq_socket_server_ptr_ = NULL;

        // Release the socket client to blocked edge nodes
        assert(edge_beacon_server_sendreq_toblocked_socket_client_ptr_ != NULL);
        delete edge_beacon_server_sendreq_toblocked_socket_client_ptr_;
        edge_beacon_server_sendreq_toblocked_socket_client_ptr_ = NULL;
    }

    void BeaconServerBase::start()
    {
        checkPointers_();

        bool is_finish = false; // Mark if edge node is finished
        while (edge_wrapper_ptr_->edge_param_ptr_->isEdgeRunning()) // edge_running_ is set as true by default
        {
            // Receive the message payload of control requests
            DynamicArray control_request_msg_payload;
            NetworkAddr control_request_network_addr;
            bool is_timeout = edge_beacon_server_recvreq_socket_server_ptr_->recv(control_request_msg_payload, control_request_network_addr);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {
                assert(control_request_network_addr.isValidAddr());
                
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);

                if (control_request_ptr->isControlRequest()) // Control requests (e.g., invalidation and cache admission/eviction requests)
                {
                    is_finish = processControlRequest_(control_request_ptr, control_request_network_addr);
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

    // (1) Access content directory information

    bool BeaconServerBase::processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr)
    {
        assert(control_request_ptr != NULL && control_request_ptr->isControlRequest());

        bool is_finish = false; // Mark if edge node is finished

        MessageType message_type = control_request_ptr->getMessageType();
        if (message_type == MessageType::kDirectoryLookupRequest)
        {
            is_finish = processDirectoryLookupRequest_(control_request_ptr, closest_edge_addr);
        }
        else if (message_type == MessageType::kDirectoryUpdateRequest)
        {
            is_finish = processDirectoryUpdateRequest_(control_request_ptr);
        }
        else if (message_type == MessageType::kAcquireWritelockRequest)
        {
            is_finish = processAcquireWritelockRequest_(control_request_ptr, closest_edge_addr);
        }
        else if (message_type == MessageType::kReleaseWritelockRequest)
        {
            is_finish = processReleaseWritelockRequest_(control_request_ptr);
        }
        else
        {
            // NOTE: only COVERED has other control requests to process
            is_finish = processOtherControlRequest_(control_request_ptr);
        }
        // TODO: invalidation and cache admission/eviction requests for control message
        // TODO: reply control response message to a beacon node
        return is_finish;
    }

    // (4) Utility functions

    void BeaconServerBase::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_param_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_cache_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cooperation_wrapper_ptr_ != NULL);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);
        assert(edge_beacon_server_sendreq_toblocked_socket_client_ptr_ != NULL);
    }
}
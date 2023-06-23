#include "edge/beacon_server/beacon_server_base.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
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
        // NOTE: no need to release edge_wrapper_ptr_, which is maintained outside BeaconServerBase

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
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_param_ptr_ != NULL);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);
        
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
                assert(control_request_network_addr.isValid());
                
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
        if (message_type == MessageType::kDirectoryLookupRequest) // TODO: control_request_ptr->isDirectoryLookupRequest() for kCoveredDirectoryLookupRequest
        {
            is_finish = processDirectoryLookupRequest_(control_request_ptr, closest_edge_addr);
        }
        else if (message_type == MessageType::kDirectoryUpdateRequest) // TODO: control_request_ptr->isDirectoryUpdateRequest() for kCoveredDirectoryUpdateRequest
        {
            is_finish = processDirectoryUpdateRequest_(control_request_ptr);
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

    // (2) Unblock for MSI protocol

    bool BeaconServerBase::tryToNotifyEdgesFromBlocklist_(const Key& key) const
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cooperation_wrapper_ptr_ != NULL);
        assert(edge_beacon_server_sendreq_toblocked_socket_client_ptr_ != NULL);

        // The current edge node must be the beacon node for the key
        bool current_is_beacon = edge_wrapper_ptr_->currentIsBeacon_(key);
        assert(current_is_beacon);

        // Pop all blocked edge nodes
        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = edge_wrapper_ptr_->cooperation_wrapper_ptr_->getBlocklistIfNoWrite(key);

        // Notify all blocked edge nodes simultaneously to finish blocking for writes
        bool is_finish = notifyEdgesToFinishBlock_(key, blocked_edges);

        return is_finish;
    }

    bool BeaconServerBase::notifyEdgesToFinishBlock_(const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges) const
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_param_ptr_ != NULL);
        assert(edge_beacon_server_sendreq_toblocked_socket_client_ptr_ != NULL);

        bool is_finish = false;

        uint32_t blocked_edgecnt = blocked_edges.size();
        if (blocked_edgecnt == 0)
        {
            return is_finish;
        }
        assert(blocked_edgecnt > 0);

        // Track whether notifictionas to all closest edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> acked_flags;
        for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_ackflag = blocked_edges.begin(); iter_for_ackflag != blocked_edges.end(); iter_for_ackflag++)
        {
            acked_flags.insert(std::pair<NetworkAddr, bool>(*iter_for_ackflag, false));
        }

        while (acked_edgecnt != blocked_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (blocked_edgecnt - acked_edgecnt) control requests to the closest edge nodes that have not acknowledged notifications
            for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_request = blocked_edges.begin(); iter_for_request != blocked_edges.end(); iter_for_request++)
            {
                if (acked_flags[*iter_for_request]) // Skip the closest edge node that has acknowledged the notification
                {
                    continue;
                }

                const NetworkAddr& tmp_network_addr = *iter_for_request; // cache server address of a blocked closest edge node          
                sendFinishBlockRequest_(key, tmp_network_addr);     
            } // End of edgeidx_for_request

            // Receive (blocked_edgecnt - acked_edgecnt) control repsonses from the beacon node
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < blocked_edgecnt - acked_edgecnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                NetworkAddr control_response_addr;
                bool is_timeout = edge_beacon_server_sendreq_toblocked_socket_client_ptr_->recv(control_response_msg_payload, control_response_addr);
                if (is_timeout)
                {
                    if (!edge_wrapper_ptr_->edge_param_ptr_->isEdgeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for FinishBlockResponse");
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    assert(control_response_addr.isValid());

                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kFinishBlockResponse);

                    // Mark the closest edge node has acknowledged the FinishBlockRequest
                    bool is_match = false;
                    for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_response = blocked_edges.begin(); iter_for_response != blocked_edges.end(); iter_for_response++)
                    {
                        if (*iter_for_response == control_response_addr) // Match a closest edge node
                        {
                            assert(acked_flags[*iter_for_response] == false); // Original ack flag should be false

                            // Update ack information
                            acked_flags[*iter_for_response] = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive FinishBlockResponse from an edge node " << control_response_addr.getIpstr() << ":" << control_response_addr.getPort() << ", which is NOT in the block list!";
                        Util::dumpWarnMsg(base_instance_name_, oss.str());
                    } // End of !is_match

                    // Release the control response message
                    delete control_response_ptr;
                    control_response_ptr = NULL;
                } // End of (is_timeout == false)
            } // End of edgeidx_for_response

            if (is_finish) // Edge node is NOT running now
            {
                break;
            }
        } // End of while(acked_edgecnt != blocked_edgecnt)

        return is_finish;
    }
}
#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"
#include "message/control_message.h"
#include "message/data_message.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr) : CooperationWrapperBase(hash_name, edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}

    // (1) Get data from target edge node

    bool BasicCooperationWrapper::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // NOTE: no need to acquire a read lock for cooperation metadata due to accessing const shared variables and non-const individual variables

        // The current edge node must NOT be the beacon node for the key
        verifyCurrentIsNotBeacon_(key);

        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);

        bool is_finish = false;

        // Prepare directory lookup request to check directory information in beacon node
        DirectoryLookupRequest directory_lookup_request(key);
        DynamicArray control_request_msg_payload(directory_lookup_request.getMsgPayloadSize());
        directory_lookup_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryLookupResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kDirectoryLookupResponse);

                // Get directory information from the control response message
                const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
                is_being_written = directory_lookup_response_ptr->isBeingWritten();
                is_valid_directory_exist = directory_lookup_response_ptr->isValidDirectoryExist();
                directory_info = directory_lookup_response_ptr->getDirectoryInfo();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    bool BasicCooperationWrapper::redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const
    {
        // No need to acquire a read lock for cooperation metadata due to accessing const shared variables and non-const individual variables

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);

        bool is_finish = false;

        // Prepare redirected get request to get data from target edge node if any
        RedirectedGetRequest redirected_get_request(key);
        DynamicArray redirected_request_msg_payload(redirected_get_request.getMsgPayloadSize());
        redirected_get_request.serialize(redirected_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the redirected request to the target edge node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_sendreq_totarget_socket_client_ptr_->send(redirected_request_msg_payload);

            // Receive the control repsonse from the target node
            DynamicArray redirected_response_msg_payload;
            bool is_timeout = edge_sendreq_totarget_socket_client_ptr_->recv(redirected_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for RedirectedGetResponse");
                    continue; // Resend the redirected request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the redirected response message successfully
                MessageBase* redirected_response_ptr = MessageBase::getResponseFromMsgPayload(redirected_response_msg_payload);
                assert(redirected_response_ptr != NULL && redirected_response_ptr->getMessageType() == MessageType::kRedirectedGetResponse);

                // Get value from redirected response message
                const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(redirected_response_ptr);
                value = redirected_get_response_ptr->getValue();
                Hitflag hitflag = redirected_get_response_ptr->getHitflag();
                if (hitflag == Hitflag::kCooperativeHit)
                {
                    is_cooperative_cached = true;
                    is_valid = true;
                }
                else if (hitflag == Hitflag::kCooperativeInvalid)
                {
                    is_cooperative_cached = true;
                    is_valid = false;
                }
                else if (hitflag == Hitflag::kGlobalMiss)
                {
                    std::ostringstream oss;
                    oss << "target edge node does not cache the key " << key.getKeystr() << " in redirectGetToTarget_()!";
                    Util::dumpWarnMsg(instance_name_, oss.str());

                    is_cooperative_cached = false;
                    is_valid = false;
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid hitflag " << MessageBase::hitflagToString(hitflag) << " for redirectGetToTarget_()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release the redirected response message
                delete redirected_response_ptr;
                redirected_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    // (2) Update content directory information

    bool BasicCooperationWrapper::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written)
    {
        // NOTE: no need to acquire a read lock for cooperation metadata due to accessing const shared variables and non-const individual variables

        // The current edge node must NOT be the beacon node for the key
        verifyCurrentIsNotBeacon_(key);

        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);

        bool is_finish = false;

        // Prepare directory update request to check directory information in beacon node
        DirectoryUpdateRequest directory_update_request(key, is_admit, directory_info);
        DynamicArray control_request_msg_payload(directory_update_request.getMsgPayloadSize());
        directory_update_request.serialize(control_request_msg_payload);

        while (true) // Timeout-and-retry mechanism
        {
            // Send the control request to the beacon node
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryUpdateResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

                // Get is_being_written from control response message
                const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = directory_update_response_ptr->isBeingWritten();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }

    // (3) Blocking for MSI protocols

    void BasicCooperationWrapper::sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const
    {
        // Prepare finish block request to finish blocking for writes in all closest edge nodes
        FinishBlockRequest finish_block_request(key);
        DynamicArray control_request_msg_payload(finish_block_request.getMsgPayloadSize());
        finish_block_request.serialize(control_request_msg_payload);

        // Set remote address to the closest edge node
        edge_sendreq_toclosest_cache_server_socket_client_ptr_->setRemoteAddrForClient(closest_edge_addr);

        // Send FinishBlockRequest to the closest edge node
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_sendreq_toclosest_cache_server_socket_client_ptr_->send(control_request_msg_payload);

        return;
    }
}
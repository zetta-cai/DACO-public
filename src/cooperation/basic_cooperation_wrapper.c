#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr) : CooperationWrapperBase(hash_name, edge_param_ptr)
    {
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}

    bool BasicCooperationWrapper::lookupBeaconDirectory_(const Key& key, bool& is_directory_exist, uint32_t& neighbor_edge_idx)
    {
        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);
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
            edge_sendreq_toneighbor_socket_client_ptr_->send(control_request_msg_payload);

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_sendreq_toneighbor_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(kClassName, "edge timeout to wait for control response");
                    continue; // Resend the global request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_request_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kDirectoryLookupResponse);

                // Get directory information from the control response message
                const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
                is_directory_exist = directory_lookup_response_ptr->isDirectoryExist();
                neighbor_edge_idx = directory_lookup_response_ptr->getNeighborEdgeIdx();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }
}
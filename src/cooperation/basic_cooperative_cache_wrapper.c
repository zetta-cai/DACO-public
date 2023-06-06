#include "cooperation/basic_cooperative_cache_wrapper.h"

#include <assert.h>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicCooperativeCacheWrapper::kClassName("BasicCooperativeCacheWrapper");

    BasicCooperativeCacheWrapper::BasicCooperativeCacheWrapper(EdgeParam* edge_param_ptr) : CooperativeCacheWrapperBase(edge_param_ptr)
    {
    }

    BasicCooperativeCacheWrapper::~BasicCooperativeCacheWrapper() {}

    bool BasicCooperativeCacheWrapper::get(const Key& key, Value& value, bool& is_cooperative_cached)
    {
        assert(dht_wrapper_ptr_ != NULL);
        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);

        bool is_finish = false;

        // Update remote address of edge_sendreq_toneighbor_socket_client_ptr_ as the beacon node for the key
        locateBeaconNode_(key);

        // Lookup directory information at the beacon node
        uint32_t neighbor_edge_idx = 0;
        is_finish = directoryLookup_(key, neighbor_edge_idx);
        if (is_finish) // Edge is NOT running
        {
            return is_finish;
        }

        // TODO: Get data from the neighbor node based on the directory information and set is_cooperative_cached = true
        // TODO: Or set is_cooperative_cached = false and return is_finish

        return is_finish;
    }

    bool BasicCooperativeCacheWrapper::directoryLookup_(const Key& key, uint32_t& neighbor_edge_idx)
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

                // Get value from global response message
                const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
                neighbor_edge_idx = directory_lookup_response_ptr->getNeighborEdgeIdx();

                // Release global response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        return is_finish;
    }
}
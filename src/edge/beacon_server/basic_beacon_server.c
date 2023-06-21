#include "edge/beacon_server/basic_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicBeaconServer::kClassName("BasicBeaconServer");

    BasicBeaconServer::BasicBeaconServer(EdgeWrapper* edge_wrapper_ptr) : BeaconServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();

        // Differentiate BasicBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicBeaconServer::~BasicBeaconServer() {}

    // Control requests

    bool BasicBeaconServer::processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const
    {
        // Get key and value from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();

        assert(closest_edge_addr.isValid());

        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cooperation_wrapper_ptr_ != NULL);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;

        // Lookup local directory information and randomly select a target edge index
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        edge_wrapper_ptr_->cooperation_wrapper_ptr_->lookupLocalDirectory(tmp_key, is_being_written, is_valid_directory_exist, directory_info);

        // Send back a directory lookup response
        DirectoryLookupResponse directory_lookup_response(tmp_key, is_being_written, is_valid_directory_exist, directory_info);
        DynamicArray control_response_msg_payload(directory_lookup_response.getMsgPayloadSize());
        directory_lookup_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_beacon_server_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        if (is_being_written)
        {
            // Add the closest edge node into the CooperationWrapperBase::perkey_edge_blocklist_
            edge_wrapper_ptr_->cooperation_wrapper_ptr_->addEdgeIntoBlocklist(tmp_key, closest_edge_addr);

            // Try to notify blocked edge nodes if without writes, in case that writes have finished before adding the closest edge node
            edge_wrapper_ptr_->cooperation_wrapper_ptr_->tryToNotifyEdgesFromBlocklist(tmp_key);
        }

        return is_finish;
    }

    bool BasicBeaconServer::processDirectoryUpdateRequest_(MessageBase* control_request_ptr)
    {
        // Get key and value from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
        Key tmp_key = directory_update_request_ptr->getKey();
        bool is_admit = directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = directory_update_request_ptr->getDirectoryInfo();

        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cooperation_wrapper_ptr_ != NULL);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;        

        // Update local directory information
        bool is_being_written = false;
        edge_wrapper_ptr_->cooperation_wrapper_ptr_->updateLocalDirectory(tmp_key, is_admit, directory_info, is_being_written);

        // Send back a directory update response
        DirectoryUpdateResponse directory_update_response(tmp_key, is_being_written);
        DynamicArray control_response_msg_payload(directory_update_response.getMsgPayloadSize());
        directory_update_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_beacon_server_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        return is_finish;
    }
    
    bool BasicBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL);

        bool is_finish = false;

        std::ostringstream oss;
        oss << "control request " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " is not supported!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return is_finish;
    }
}
#include "edge/basic_edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/key.h"
#include "common/param.h"
#include "common/util.h"
#include "message/control_message.h"
#include "message/message_base.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicEdgeWrapper::kClassName("BasicEdgeWrapper");

    BasicEdgeWrapper::BasicEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr) : EdgeWrapperBase(cache_name, hash_name, edge_param_ptr)
    {
        assert(cache_name != Param::COVERED_CACHE_NAME);
    }

    BasicEdgeWrapper::~BasicEdgeWrapper() {}

    bool BasicEdgeWrapper::processDirectoryLookupRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;

        // Get key from directory lookup request
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();

        // Lookup local directory information and randomly select a target edge index
        bool is_directory_exist = false;
        uint32_t target_edge_idx = 0;
        cooperation_wrapper_ptr_->getTargetEdgeIdxForDirectoryLookupRequest(tmp_key, is_directory_exist, target_edge_idx);

        // Send back a directory lookup response
        DirectoryLookupResponse directory_lookup_response(tmp_key, is_directory_exist, target_edge_idx);
        DynamicArray control_response_msg_payload(directory_lookup_response.getMsgPayloadSize());
        directory_lookup_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        return is_finish;
    }
    
    bool BasicEdgeWrapper::processOtherControlRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL);

        bool is_finish = false;

        std::ostringstream oss;
        oss << "control request " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " is not supported!";
        Util::dumpErrorMsg(kClassName, oss.str());
        exit(1);

        return is_finish;
    }
}
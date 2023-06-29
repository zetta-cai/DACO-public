#include "edge/invalidation_server/basic_invalidation_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicInvalidationServer::kClassName("BasicInvalidationServer");

    BasicInvalidationServer::BasicInvalidationServer(EdgeWrapper* edge_wrapper_ptr) : InvalidationServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();

        // Differentiate BasicInvalidationServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicInvalidationServer::~BasicInvalidationServer() {}

    bool BasicInvalidationServer::processInvalidationRequest_(MessageBase* control_request_ptr)
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest);
        const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
        Key tmp_key = invalidation_request_ptr->getKey();

        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_param_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_cache_ptr_ != NULL);
        assert(edge_invalidation_server_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;

        // Invalidate cached object in local edge cache
        bool is_local_cached = edge_wrapper_ptr_->edge_cache_ptr_->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            edge_wrapper_ptr_->edge_cache_ptr_->invalidateKeyForLocalCachedObject(tmp_key);
        }

        // Send back a invalidation response
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();
        InvalidationResponse invalidation_response(tmp_key, edge_idx);
        DynamicArray control_response_msg_payload(invalidation_response.getMsgPayloadSize());
        invalidation_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_invalidation_server_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        return is_finish;
    }
}
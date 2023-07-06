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
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getNodeIdx();

        // Differentiate BasicInvalidationServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicInvalidationServer::~BasicInvalidationServer() {}

    bool BasicInvalidationServer::processInvalidationRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest);
        const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
        Key tmp_key = invalidation_request_ptr->getKey();

        checkPointers_();

        bool is_finish = false;

        // Invalidate cached object in local edge cache
        bool is_local_cached = edge_wrapper_ptr_->edge_cache_ptr_->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            edge_wrapper_ptr_->edge_cache_ptr_->invalidateKeyForLocalCachedObject(tmp_key);
        }

        // Prepare a invalidation response
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getNodeIdx();
        MessageBase* invalidation_response_ptr = new InvalidationResponse(tmp_key, edge_idx, edge_invalidation_server_recvreq_source_addr_);
        assert(invalidation_response_ptr != NULL);

        // Push the invalidation response into edge-to-edge propagation simulator to cache server worker or beacon server
        bool is_successful = edge_wrapper_ptr_->edge_toedge_propagation_simulator_param_ptr_->push(invalidation_response_ptr, recvrsp_dst_addr);
        assert(is_successful);
        
        // NOTE: invalidation_response_ptr will be released by edge-to-edge propagation simulator

        return is_finish;
    }
}
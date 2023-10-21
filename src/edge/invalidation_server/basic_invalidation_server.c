#include "edge/invalidation_server/basic_invalidation_server.h"

#include <assert.h>
#include <sstream>

#include "common/bandwidth_usage.h"
#include "common/util.h"
#include "event/event.h"
#include "event/event_list.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicInvalidationServer::kClassName("BasicInvalidationServer");

    BasicInvalidationServer::BasicInvalidationServer(EdgeWrapper* edge_wrapper_ptr) : InvalidationServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->getCacheName() != Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate BasicInvalidationServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicInvalidationServer::~BasicInvalidationServer() {}

    void BasicInvalidationServer::processReqForInvalidation_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest);

        const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
        Key tmp_key = invalidation_request_ptr->getKey();

        // Invalidate cached object in local edge cache
        bool is_local_cached = edge_wrapper_ptr_->getEdgeCachePtr()->isLocalCached(tmp_key);
        if (is_local_cached)
        {
            edge_wrapper_ptr_->getEdgeCachePtr()->invalidateKeyForLocalCachedObject(tmp_key);
        }

        return;
    }

    MessageBase* BasicInvalidationServer::getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list)
    {
        assert(control_request_ptr->getMessageType() == MessageType::kInvalidationRequest);

        const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(control_request_ptr);
        Key tmp_key = invalidation_request_ptr->getKey();
        const bool skip_propagation_latency = invalidation_request_ptr->isSkipPropagationLatency();

        // Prepare invalidation response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* invalidation_response_ptr = new InvalidationResponse(tmp_key, edge_idx, edge_invalidation_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(invalidation_response_ptr != NULL);

        return invalidation_response_ptr;
    }
}
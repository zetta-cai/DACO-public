#include "edge/invalidation_server/covered_invalidation_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredInvalidationServer::kClassName("CoveredInvalidationServer");

    CoveredInvalidationServer::CoveredInvalidationServer(EdgeWrapper* edge_wrapper_ptr) : InvalidationServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ == Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->node_idx_;

        // Differentiate CoveredInvalidationServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredInvalidationServer::~CoveredInvalidationServer() {}

    bool CoveredInvalidationServer::processInvalidationRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        return false;
    }
}
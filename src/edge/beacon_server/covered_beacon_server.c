#include "edge/beacon_server/covered_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "event/event.h"
#include "event/event_list.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredBeaconServer::kClassName("CoveredBeaconServer");

    CoveredBeaconServer::CoveredBeaconServer(EdgeWrapper* edge_wrapper_ptr) : BeaconServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ == Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->node_idx_;

        // Differentiate CoveredBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredBeaconServer::~CoveredBeaconServer() {}

    // (1) Access content directory information

    bool CoveredBeaconServer::processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) const
    {
        // NOTE: For COVERED, beacon node will tell the closest edge node if to admit, w/o independent decision

        return false;
    }

    bool CoveredBeaconServer::processDirectoryUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        // NOTE: For COVERED, beacon node will tell the closest edge node if to admit, w/o independent decision

        return false;
    }

    // (2) Process writes and unblock for MSI protocol

    bool CoveredBeaconServer::processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        return false;
    }

    bool CoveredBeaconServer::processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        return false;
    }

    // (3) Process other control requests

    bool CoveredBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        return false;
    }
}
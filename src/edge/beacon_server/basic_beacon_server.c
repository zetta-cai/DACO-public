#include "edge/beacon_server/basic_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "event/event.h"
#include "event/event_list.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicBeaconServer::kClassName("BasicBeaconServer");

    BasicBeaconServer::BasicBeaconServer(EdgeWrapper* edge_wrapper_ptr) : BeaconServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->getCacheName() != Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate BasicBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicBeaconServer::~BasicBeaconServer() {}

    // (1) Access content directory information

    bool BasicBeaconServer::processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const
    {
        bool is_finish = false;

        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();

        // Lookup local content directory
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();
        is_being_written = false;
        bool is_source_cached = false;
        edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupDirectoryTableByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToLookupLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        // Get key and skip_propagation_latency from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();
        const bool skip_propagation_latency = directory_lookup_request_ptr->isSkipPropagationLatency();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* directory_lookup_response_ptr = new DirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(directory_lookup_response_ptr != NULL);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return directory_lookup_response_ptr;
    }

    bool BasicBeaconServer::processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        // Foreground directory updates for baselines
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
        Key tmp_key = directory_update_request_ptr->getKey();
        bool is_admit = directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = directory_update_request_ptr->getDirectoryInfo();

        bool is_finish = false;

        // Update local directory information in cooperation wrapper
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();
        is_being_written = false;
        bool is_source_cached = false;
        edge_wrapper_ptr_->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, source_edge_idx, is_admit, directory_info, is_being_written, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);

        const Key tmp_key = directory_update_request_ptr->getKey();
        const bool skip_propagation_latency = directory_update_request_ptr->isSkipPropagationLatency();

        checkPointers_();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* directory_update_response_ptr = new DirectoryUpdateResponse(tmp_key, is_being_written, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(directory_update_response_ptr != NULL);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return directory_update_response_ptr;
    }

    // (2) Process writes and unblock for MSI protocol

    bool BasicBeaconServer::processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kAcquireWritelockRequest);
        const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = acquire_writelock_request_ptr->getKey();

        bool is_finish = false;

        // Get result of acquiring local write lock
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();
        bool is_source_cached = false;
        lock_result = edge_wrapper_ptr_->getCooperationWrapperPtr()->acquireLocalWritelockByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, all_dirinfo, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToAcquireLocalWritelock_(MessageBase* control_request_ptr, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kAcquireWritelockRequest);
        const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = acquire_writelock_request_ptr->getKey();
        bool skip_propagation_latency = acquire_writelock_request_ptr->isSkipPropagationLatency();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* acquire_writelock_response_ptr = new AcquireWritelockResponse(tmp_key, lock_result, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(acquire_writelock_response_ptr != NULL);

        return acquire_writelock_response_ptr;
    }

    bool BasicBeaconServer::processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kReleaseWritelockRequest);
        const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = release_writelock_request_ptr->getKey();

        bool is_finish = false;

        // Release local write lock and validate sender directory info if any
        uint32_t sender_edge_idx = release_writelock_request_ptr->getSourceIndex();
        DirectoryInfo sender_directory_info(sender_edge_idx);
        bool is_source_cached = false;
        blocked_edges = edge_wrapper_ptr_->getCooperationWrapperPtr()->releaseLocalWritelock(tmp_key, sender_edge_idx, sender_directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToReleaseLocalWritelock_(MessageBase* control_request_ptr, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kReleaseWritelockRequest);
        const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = release_writelock_request_ptr->getKey();
        bool skip_propagation_latency = release_writelock_request_ptr->isSkipPropagationLatency();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* release_writelock_response_ptr = new ReleaseWritelockResponse(tmp_key, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(release_writelock_response_ptr != NULL);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return release_writelock_response_ptr;
    }

    // (3) Process other control requests
    
    bool BasicBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);

        bool is_finish = false;

        std::ostringstream oss;
        oss << "control request " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " is not supported!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return is_finish;
    }

    // (4) Process redirected/global get response for non-blocking placement deployment (ONLY for COVERED)

    void BasicBeaconServer::processRspToRedirectGetForPlacement_(MessageBase* redirected_get_response_ptr)
    {
        checkPointers_();
        
        std::ostringstream oss;
        oss << edge_wrapper_ptr_->getCacheName() << " should NOT receive CoveredPlacementRedirectedGetResponse which is ONLY for COVERED!!!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }

    void BasicBeaconServer::processRspToAccessCloudForPlacement_(MessageBase* cloud_get_response_ptr)
    {
        checkPointers_();
        
        std::ostringstream oss;
        oss << edge_wrapper_ptr_->getCacheName() << " should NOT receive CoveredPlacementGlobalGetResponse which is ONLY for COVERED!!!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }
}
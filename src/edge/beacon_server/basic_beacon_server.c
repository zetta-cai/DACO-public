#include "edge/beacon_server/basic_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
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
        assert(edge_wrapper_ptr_->getCacheName() != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate BasicBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicBeaconServer::~BasicBeaconServer() {}

    // (1) Access content directory information

    bool BasicBeaconServer::processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) const
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();
        const bool skip_propagation_latency = directory_lookup_request_ptr->isSkipPropagationLatency();

        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());

        checkPointers_();

        bool is_finish = false;

        EventList event_list;
        struct timespec lookup_local_directory_start_timestamp = Util::getCurrentTimespec();

        // Calculate cache server worker recvreq destination address
        NetworkAddr edge_cache_server_worker_recvreq_source_addr = Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(edge_cache_server_worker_recvrsp_dst_addr);

        // Lookup local directory information and randomly select a target edge index
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupLocalDirectoryByBeaconServer(tmp_key, edge_cache_server_worker_recvreq_source_addr, is_being_written, is_valid_directory_exist, directory_info);

        // Add intermediate event if with event tracking
        struct timespec lookup_local_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t lookup_local_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(lookup_local_directory_end_timestamp, lookup_local_directory_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME, lookup_local_directory_latency_us);

        // Prepare a directory lookup response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* directory_lookup_response_ptr = new DirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(directory_lookup_response_ptr != NULL);
        
        // Push the directory lookup response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_lookup_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: directory_lookup_response_ptr will be released by edge-to-edge propagation simulator
        directory_lookup_response_ptr = NULL;

        return is_finish;
    }

    bool BasicBeaconServer::processDirectoryUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        // Get key, admit/evict,and directory info from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
        Key tmp_key = directory_update_request_ptr->getKey();
        const bool skip_propagation_latency = directory_update_request_ptr->isSkipPropagationLatency();
        bool is_admit = directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = directory_update_request_ptr->getDirectoryInfo();

        checkPointers_();

        bool is_finish = false;

        EventList event_list;
        struct timespec update_local_directory_start_timestamp = Util::getCurrentTimespec();

        // Update local directory information
        bool is_being_written = false;
        edge_wrapper_ptr_->getCooperationWrapperPtr()->updateLocalDirectory(tmp_key, is_admit, directory_info, is_being_written);

        // Add intermediate event if with event tracking
        struct timespec update_local_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_local_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_local_directory_end_timestamp, update_local_directory_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME, update_local_directory_latency_us);

        // Prepare a directory update response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* directory_update_response_ptr = new DirectoryUpdateResponse(tmp_key, is_being_written, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(directory_update_response_ptr != NULL);

        // Push the directory update response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: directory_update_response_ptr will be released by edge-to-edge propagation simulator
        directory_update_response_ptr = NULL;

        return is_finish;
    }

    // (2) Process writes and unblock for MSI protocol

    bool BasicBeaconServer::processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kAcquireWritelockRequest);
        const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = acquire_writelock_request_ptr->getKey();
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        checkPointers_();

        bool is_finish = false;

        EventList event_list;
        struct timespec acquire_local_writelock_start_timestamp = Util::getCurrentTimespec();

        // Calculate cache server worker recvreq destination address
        NetworkAddr edge_cache_server_worker_recvreq_dst_addr = Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(edge_cache_server_worker_recvrsp_dst_addr);
        
        // Try to acquire permission for the write
        LockResult lock_result = LockResult::kFailure;
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> all_dirinfo;
        lock_result = edge_wrapper_ptr_->getCooperationWrapperPtr()->acquireLocalWritelockByBeaconServer(tmp_key, edge_cache_server_worker_recvreq_dst_addr, all_dirinfo);

        // Add intermediate event if with event tracking
        struct timespec acquire_local_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_local_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_local_writelock_end_timestamp, acquire_local_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME, acquire_local_writelock_latency_us);

        // NOTE: we invalidate cache copies by beacon code to avoid transmitting all_dirinfo to cache server of the closest edge node
        if (lock_result == LockResult::kSuccess) // If acquiring write permission successfully
        {
            // Invalidate all cache copies
            edge_wrapper_ptr_->invalidateCacheCopies(edge_beacon_server_recvrsp_socket_server_ptr_, edge_beacon_server_recvrsp_source_addr_, tmp_key, all_dirinfo, event_list, skip_propagation_latency); // Add events of intermedate responses if with event tracking
        }

        // Prepare a acquire writelock response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* acquire_writelock_response_ptr = new AcquireWritelockResponse(tmp_key, lock_result, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(acquire_writelock_response_ptr != NULL);

        // Push acquire writelock response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(acquire_writelock_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: acquire_writelock_response_ptr will be released by edge-to-edge propagation simulator
        acquire_writelock_response_ptr = NULL;

        return is_finish;
    }

    bool BasicBeaconServer::processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kReleaseWritelockRequest);
        const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = release_writelock_request_ptr->getKey();
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        checkPointers_();

        bool is_finish = false;

        EventList event_list;
        struct timespec release_local_writelock_start_timestamp = Util::getCurrentTimespec();

        // Release permission for the write
        uint32_t sender_edge_idx = release_writelock_request_ptr->getSourceIndex();
        DirectoryInfo sender_directory_info(sender_edge_idx);
        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = edge_wrapper_ptr_->getCooperationWrapperPtr()->releaseLocalWritelock(tmp_key, sender_directory_info);

        // Add intermediate event if with event tracking
        struct timespec release_local_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t release_local_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_local_writelock_end_timestamp, release_local_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME, release_local_writelock_latency_us);

        // NOTE: notify blocked edge nodes if any after finishing writes, to avoid transmitting blocked_edges to cache server of the closest edge node
        is_finish = edge_wrapper_ptr_->notifyEdgesToFinishBlock_(edge_beacon_server_recvrsp_socket_server_ptr_, edge_beacon_server_recvrsp_source_addr_, tmp_key, blocked_edges, event_list, skip_propagation_latency); // Add events of intermedate responses if with event tracking

        // Prepare a release writelock response
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* release_writelock_response_ptr = new ReleaseWritelockResponse(tmp_key, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(release_writelock_response_ptr != NULL);

        // Push release writelock response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(release_writelock_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: release_writelock_response_ptr will be released by edge-to-edge propagation simulator
        release_writelock_response_ptr = NULL;

        return is_finish;
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
}
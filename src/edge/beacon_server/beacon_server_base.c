#include "edge/beacon_server/beacon_server_base.h"

#include <assert.h>

#include "common/bandwidth_usage.h"
#include "common/config.h"
#include "common/util.h"
#include "edge/beacon_server/basic_beacon_server.h"
#include "edge/beacon_server/covered_beacon_server.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BeaconServerBase::kClassName("BeaconServerBase");

    BeaconServerBase* BeaconServerBase::getBeaconServerByCacheName(EdgeWrapper* edge_wrapper_ptr)
    {
        BeaconServerBase* beacon_server_ptr = NULL;

        assert(edge_wrapper_ptr != NULL);
        std::string cache_name = edge_wrapper_ptr->getCacheName();
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            beacon_server_ptr = new CoveredBeaconServer(edge_wrapper_ptr);
        }
        else
        {
            beacon_server_ptr = new BasicBeaconServer(edge_wrapper_ptr);
        }

        assert(beacon_server_ptr != NULL);
        return beacon_server_ptr;
    }

    BeaconServerBase::BeaconServerBase(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr), edge_beacon_server_background_counter_()
    {
        assert(edge_wrapper_ptr != NULL);
        const uint32_t edge_idx = edge_wrapper_ptr->getNodeIdx();
        const uint32_t edgecnt = edge_wrapper_ptr->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        // For receiving control requests

        // Get source address of beacon server recvreq
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_beacon_server_recvreq_port = Util::getEdgeBeaconServerRecvreqPort(edge_idx, edgecnt);
        edge_beacon_server_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_beacon_server_recvreq_port);

        // Prepare a socket server on recvreq port for beacon server
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, edge_beacon_server_recvreq_port);
        edge_beacon_server_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);

        // For receiving control responses (e.g., InvalidationResponse and FinishBlockResponse)

        // Get source address of beacon server recvrsp
        uint16_t edge_beacon_server_recvrsp_port = Util::getEdgeBeaconServerRecvrspPort(edge_idx, edgecnt);
        edge_beacon_server_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_beacon_server_recvrsp_port);

        // Prepare a socket server on recvrsp port for beacon server
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_beacon_server_recvrsp_port);
        edge_beacon_server_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_beacon_server_recvrsp_socket_server_ptr_ != NULL);
    }

    BeaconServerBase::~BeaconServerBase()
    {
        // NOTE: no need to release edge_wrapper_ptr_, which will be released outside BeaconServerBase (e.g., simulator)

        // Release the socket server on recvreq port
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_beacon_server_recvreq_socket_server_ptr_;
        edge_beacon_server_recvreq_socket_server_ptr_ = NULL;

        // Release the socket server on recvrsp port
        assert(edge_beacon_server_recvrsp_socket_server_ptr_ != NULL);
        delete edge_beacon_server_recvrsp_socket_server_ptr_;
        edge_beacon_server_recvrsp_socket_server_ptr_ = NULL;
    }

    void BeaconServerBase::start()
    {
        checkPointers_();

        bool is_finish = false; // Mark if edge node is finished
        while (edge_wrapper_ptr_->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Receive the message payload of control requests
            DynamicArray control_request_msg_payload;
            bool is_timeout = edge_beacon_server_recvreq_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {                
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);

                NetworkAddr edge_cache_server_worker_recvrsp_dst_addr = control_request_ptr->getSourceAddr();

                if (control_request_ptr->isControlRequest()) // Control requests (e.g., invalidation and cache admission/eviction requests)
                {
                    is_finish = processControlRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
                }
                else if (control_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetResponse) // Non-blocking placement deployment
                {
                    // NOTE: NOT embed background events/bandwidth-usage into CoveredPlacementRedirectedGetResponse even if it is received by beacon server, as we need to embed such information into foreground messages to be tracked by clients
                    processRspToRedirectGetForPlacement_(control_request_ptr);
                }
                else if (control_request_ptr->getMessageType() == MessageType::kCoveredPlacementGlobalGetResponse) // Non-blocking placement deployment
                {
                    // NOTE: NOT embed background events/bandwidth-usage into CoveredPlacementRedirectedGetResponse even if it is received by beacon server, as we need to embed such information into foreground messages to be tracked by clients
                    processRspToAccessCloudForPlacement_(control_request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Release messages
                assert(control_request_ptr != NULL);
                delete control_request_ptr;
                control_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_timeout == false)
        } // End of while loop

        return;
    }

    bool BeaconServerBase::processControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL && control_request_ptr->isControlRequest());

        // Current edge node must be the beacon node for the given key of the received control request
        const Key& tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        bool current_is_beacon = edge_wrapper_ptr_->currentIsBeacon(tmp_key);
        assert(current_is_beacon);

        #ifdef DEBUG_BEACON_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "receive a control request", "type:", MessageBase::messageTypeToString(control_request_ptr->getMessageType()).c_str(), "keystr:", MessageBase::getKeyFromMessage(control_request_ptr).getKeystr().c_str());
        #endif

        bool is_finish = false; // Mark if edge node is finished

        MessageType message_type = control_request_ptr->getMessageType();
        if (message_type == MessageType::kDirectoryLookupRequest || message_type == MessageType::kCoveredDirectoryLookupRequest)
        {
            is_finish = processDirectoryLookupRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        }
        else if (message_type == MessageType::kDirectoryUpdateRequest || message_type == MessageType::kCoveredDirectoryUpdateRequest)
        {
            is_finish = processDirectoryUpdateRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        }
        else if (message_type == MessageType::kAcquireWritelockRequest || message_type == MessageType::kCoveredAcquireWritelockRequest)
        {
            is_finish = processAcquireWritelockRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        }
        else if (message_type == MessageType::kReleaseWritelockRequest || message_type == MessageType::kCoveredReleaseWritelockRequest)
        {
            is_finish = processReleaseWritelockRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        }
        else
        {
            // NOTE: only COVERED has other control requests to process
            is_finish = processOtherControlRequest_(control_request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        }
        // TODO: invalidation and cache admission/eviction requests for control message
        // TODO: reply control response message to a beacon node
        return is_finish;
    }

    // (1) Access content directory information

    bool BeaconServerBase::processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) const
    {
        assert(control_request_ptr != NULL);
        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());
        
        // Get key from control request if any
        /*assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();
        const bool skip_propagation_latency = directory_lookup_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received directory lookup request
        uint32_t cross_edge_directory_lookup_req_bandwidth_bytes = control_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_lookup_req_bandwidth_bytes, 0));

        struct timespec lookup_local_directory_start_timestamp = Util::getCurrentTimespec();

        // Calculate cache server worker recvreq destination address
        NetworkAddr edge_cache_server_worker_recvreq_dst_addr = Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(edge_cache_server_worker_recvrsp_dst_addr);

        // Lookup local directory information and randomly select a target edge index
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        processReqToLookupLocalDirectory_(control_request_ptr, edge_cache_server_worker_recvreq_dst_addr, is_being_written, is_valid_directory_exist, directory_info);

        // Add intermediate event if with event tracking
        struct timespec lookup_local_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t lookup_local_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(lookup_local_directory_end_timestamp, lookup_local_directory_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME, lookup_local_directory_latency_us);

        // Prepare a directory lookup response
        embedBackgroundCounterIfNotEmpty_(total_bandwidth_usage, event_list); // Embed background events/bandwidth if any into control response message
        const Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
        MessageBase* directory_lookup_response_ptr = getRspToLookupLocalDirectory_(tmp_key, is_being_written, is_valid_directory_exist, directory_info, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(directory_lookup_response_ptr != NULL);
        
        // Push the directory lookup response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_lookup_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: directory_lookup_response_ptr will be released by edge-to-edge propagation simulator
        directory_lookup_response_ptr = NULL;

        return is_finish;
    }

    bool BeaconServerBase::processDirectoryUpdateRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());

        // Get key, admit/evict,and directory info from control request if any
        /*assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
        //uint32_t tmp_edge_idx = directory_update_request_ptr->getSourceIndex();
        Key tmp_key = directory_update_request_ptr->getKey();
        const bool skip_propagation_latency = directory_update_request_ptr->isSkipPropagationLatency();
        bool is_admit = directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = directory_update_request_ptr->getDirectoryInfo();*/

        checkPointers_();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received directory update request
        uint32_t cross_edge_directory_update_req_bandwidth_bytes = control_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_update_req_bandwidth_bytes, 0));

        struct timespec update_local_directory_start_timestamp = Util::getCurrentTimespec();

        // Update local directory information
        bool is_being_written = processReqToUpdateLocalDirectory_(control_request_ptr);

        // Add intermediate event if with event tracking
        struct timespec update_local_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_local_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_local_directory_end_timestamp, update_local_directory_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME, update_local_directory_latency_us);

        // Prepare a directory update response
        embedBackgroundCounterIfNotEmpty_(total_bandwidth_usage, event_list); // Embed background events/bandwidth if any into control response message
        const Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
        MessageBase* directory_update_response_ptr = getRspToUpdateLocalDirectory_(tmp_key, is_being_written, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(directory_update_response_ptr != NULL);

        // Push the directory update response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: directory_update_response_ptr will be released by edge-to-edge propagation simulator
        directory_update_response_ptr = NULL;

        return is_finish;
    }

    // (2) Process writes and unblock for MSI protocol

    bool BeaconServerBase::processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());

        // Get key from control request if any
        /*assert(control_request_ptr->getMessageType() == MessageType::kAcquireWritelockRequest);
        const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = acquire_writelock_request_ptr->getKey();
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received acquire writelock request
        uint32_t cross_edge_acquire_writelock_req_bandwidth_bytes = control_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_acquire_writelock_req_bandwidth_bytes, 0));

        struct timespec acquire_local_writelock_start_timestamp = Util::getCurrentTimespec();

        // Calculate cache server worker recvreq destination address
        NetworkAddr edge_cache_server_worker_recvreq_dst_addr = Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(edge_cache_server_worker_recvrsp_dst_addr);
        
        // Try to acquire permission for the write
        LockResult lock_result = LockResult::kFailure;
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> all_dirinfo;
        processReqToAcquireLocalWritelock_(control_request_ptr, edge_cache_server_worker_recvreq_dst_addr, lock_result, all_dirinfo);

        // Add intermediate event if with event tracking
        struct timespec acquire_local_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_local_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_local_writelock_end_timestamp, acquire_local_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME, acquire_local_writelock_latency_us);

        Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        // NOTE: we invalidate cache copies by beacon code to avoid transmitting all_dirinfo to cache server of the closest edge node
        if (lock_result == LockResult::kSuccess) // If acquiring write permission successfully
        {
            // Invalidate all cache copies
            edge_wrapper_ptr_->invalidateCacheCopies(edge_beacon_server_recvrsp_socket_server_ptr_, edge_beacon_server_recvrsp_source_addr_, tmp_key, all_dirinfo, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermedate responses if with event tracking
        }

        // Prepare a acquire writelock response
        embedBackgroundCounterIfNotEmpty_(total_bandwidth_usage, event_list); // Embed background events/bandwidth if any into control response message
        MessageBase* acquire_writelock_response_ptr = getRspToAcquireLocalWritelock_(tmp_key, lock_result, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(acquire_writelock_response_ptr != NULL);

        // Push acquire writelock response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(acquire_writelock_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: acquire_writelock_response_ptr will be released by edge-to-edge propagation simulator
        acquire_writelock_response_ptr = NULL;

        return is_finish;
    }

    bool BeaconServerBase::processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());

        // Get key from control request if any
        /*assert(control_request_ptr->getMessageType() == MessageType::kReleaseWritelockRequest);
        const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = release_writelock_request_ptr->getKey();
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();*/

        checkPointers_();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received release writelock request
        uint32_t cross_edge_release_writelock_req_bandwidth_bytes = control_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_release_writelock_req_bandwidth_bytes, 0));

        struct timespec release_local_writelock_start_timestamp = Util::getCurrentTimespec();

        // Release permission for the write
        std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges;
        processReqToReleaseLocalWritelock_(control_request_ptr, blocked_edges);

        // Add intermediate event if with event tracking
        struct timespec release_local_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t release_local_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_local_writelock_end_timestamp, release_local_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_BEACON_SERVER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME, release_local_writelock_latency_us);

        Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        // NOTE: notify blocked edge nodes if any after finishing writes, to avoid transmitting blocked_edges to cache server of the closest edge node
        is_finish = edge_wrapper_ptr_->notifyEdgesToFinishBlock(edge_beacon_server_recvrsp_socket_server_ptr_, edge_beacon_server_recvrsp_source_addr_, tmp_key, blocked_edges, total_bandwidth_usage, event_list, skip_propagation_latency); // Add events of intermedate responses if with event tracking

        // Prepare a release writelock response
        embedBackgroundCounterIfNotEmpty_(total_bandwidth_usage, event_list); // Embed background events/bandwidth if any into control response message
        MessageBase* release_writelock_response_ptr = getRspToReleaseLocalWritelock_(tmp_key, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(release_writelock_response_ptr != NULL);

        // Push release writelock response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(release_writelock_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: release_writelock_response_ptr will be released by edge-to-edge propagation simulator
        release_writelock_response_ptr = NULL;

        return is_finish;
    }

    // (5) Embed background events and bandwidth usage

    void BeaconServerBase::embedBackgroundCounterIfNotEmpty_(BandwidthUsage& bandwidth_usage, EventList& event_list) const
    {
        bool is_empty_before_reset = edge_beacon_server_background_counter_.loadAndReset(bandwidth_usage, event_list);
        UNUSED(is_empty_before_reset);

        return;
    }

    // (6) Utility functions

    void BeaconServerBase::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_beacon_server_recvreq_socket_server_ptr_ != NULL);
        assert(edge_beacon_server_recvrsp_socket_server_ptr_ != NULL);
    }
}
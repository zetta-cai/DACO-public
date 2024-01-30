#include "edge/cache_server/covered_cache_server.h"

#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CoveredCacheServer::kClassName("CoveredCacheServer");

    CoveredCacheServer::CoveredCacheServer(EdgeWrapperBase* edge_wrapper_ptr) : CacheServerBase(edge_wrapper_ptr)
    {
        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_wrapper_ptr->getNodeIdx();
        instance_name_ = oss.str();
    }

    CoveredCacheServer::~CoveredCacheServer()
    {}

    // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
    
    bool CoveredCacheServer::processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        bool is_finish = false;

        // CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();
            
        // Victim synchronization
        const KeyByteVictimsetMessage* directory_update_response_ptr = static_cast<const KeyByteVictimsetMessage*>(control_response_ptr);
        const uint32_t source_edge_idx = directory_update_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = 
        directory_update_response_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        edge_wrapper_ptr_->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);
        
        if (!is_background) // Foreground remote directory eviction (triggered by invalid/valid value update by local get/put and independent admission)
        {
            // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching
            const MessageType message_type = control_response_ptr->getMessageType();
            //assert(message_type == MessageType::kCoveredDirectoryUpdateResponse || message_type == MessageType::kCoveredPlacementDirectoryEvictResponse);
            if (message_type == MessageType::kCoveredDirectoryUpdateResponse) // Normal directory eviction response
            {
                // Get is_being_written (UNUSED) from control response message
                const CoveredDirectoryUpdateResponse* covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = covered_directory_update_response_ptr->isBeingWritten();
            }
            else if (message_type == MessageType::kCoveredPlacementDirectoryEvictResponse) // Directory eviction response with hybrid data fetching
            {
                const CoveredPlacementDirectoryEvictResponse* const covered_placement_directory_evict_response_ptr = static_cast<const CoveredPlacementDirectoryEvictResponse*>(control_response_ptr);

                // Get is_being_written (UNUSED) and best placement edgeset from control response message
                is_being_written = covered_placement_directory_evict_response_ptr->isBeingWritten();
                Edgeset best_placement_edgeset = covered_placement_directory_evict_response_ptr->getEdgesetRef();

                // Trigger placement notification remotely at the beacon edge node
                const Key tmp_key = covered_placement_directory_evict_response_ptr->getKey();
                is_finish = notifyBeaconForPlacementAfterHybridFetchInternal_(tmp_key, value, best_placement_edgeset, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency);
                if (is_finish)
                {
                    return is_finish;
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for processRspToEvictBeaconDirectory_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }
        else // Background remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
        {
            assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementDirectoryUpdateResponse);

            // Get is_being_written (UNUSED) from control response message
            const CoveredPlacementDirectoryUpdateResponse* covered_placement_directory_update_response_ptr = static_cast<const CoveredPlacementDirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = covered_placement_directory_update_response_ptr->isBeingWritten();
        }

        return is_finish;
    }

    // (3) Method-specific functions

    void CoveredCacheServer::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        assert(func_param_ptr != NULL);

        if (funcname == NotifyBeaconForPlacementAfterHybridFetchFuncParam::FUNCNAME)
        {
            NotifyBeaconForPlacementAfterHybridFetchFuncParam* tmp_param_ptr = static_cast<NotifyBeaconForPlacementAfterHybridFetchFuncParam*>(func_param_ptr);

            bool tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = notifyBeaconForPlacementAfterHybridFetchInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->isSkipPropagationLatency());
        }
        else
        {
            std::ostringstream oss;
            oss << funcname << " is not supported in constCustomFunc()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // Trigger non-blocking placement notification
    bool CoveredCacheServer::notifyBeaconForPlacementAfterHybridFetchInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        Edgeset tmp_best_placement_edgest = best_placement_edgeset; // Deep copy for excluding sender if with local placement

        checkPointers_();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();
        
        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t current_edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const uint32_t dst_beacon_edge_idx_for_compression = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != current_edge_idx); 

        bool is_finish = false;

        // Check if current edge node needs placement
        std::unordered_set<uint32_t>::const_iterator tmp_best_placement_edgeset_const_iter = tmp_best_placement_edgest.find(current_edge_idx);
        bool current_need_placement = false;
        bool current_is_only_placement = false;
        if (tmp_best_placement_edgeset_const_iter != tmp_best_placement_edgest.end())
        {
            current_need_placement = true;
            tmp_best_placement_edgest.erase(tmp_best_placement_edgeset_const_iter); // NOTE: we exclude current edge idx from best placement edgeset
            if (tmp_best_placement_edgest.size() == 0) // Current edge node is the only placement
            {
                current_is_only_placement = true;
            }
        }

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = edge_wrapper_ptr_->getBeaconDstaddr_(dst_beacon_edge_idx_for_compression);

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        // Notify result of hybrid data fetching towards the beacon edge node to trigger non-blocking placement notification
        bool is_being_written = false;
        VictimSyncset received_beacon_victim_syncset; // For victim synchronization
        // NOTE: for only-/including-sender hybrid fetching, ONLY consider remote directory lookup/eviction and release writelock instead of local ones
        // -> (i) Remote ones NEVER use coooperation wrapper to get is_neighbor_cached, as sender is NOT beacon here
        // -> (ii) Local ones always need "hybrid fetching" to get value and trigger normal placement (sender MUST be beacon and can get is_neighbor_cached by cooperation wrapper when admiting local directory)
        bool is_neighbor_cached = false; // ONLY used if current_need_placement
        while (true) // Timeout-and-retry mechanism
        {
            // Prepare control request message to notify the beacon edge node
            MessageBase* control_request_ptr = NULL;
            if (!current_need_placement) // Current edge node does NOT need placement
            {
                // Prepare CoveredPlacementHybridFetchedRequest
                control_request_ptr = new CoveredPlacementHybridFetchedRequest(key, value, victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
            }
            else
            {
                // NOTE: we cannot optimistically admit valid object into local edge cache first before issuing dirinfo admission request, as clients may get incorrect value if key is being written
                if (!current_is_only_placement) // Current edge node is NOT the only placement
                {
                    // Prepare CoveredPlacementDirectoryAdmitRequest (also equivalent to directory admission request)
                    control_request_ptr = new CoveredPlacementDirectoryAdmitRequest(key, value, DirectoryInfo(current_edge_idx), victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
                }
                else // Current edge node is the only placement
                {
                    // Prepare CoveredDirectoryUpdateRequest (NOT trigger placement notification; also equivalent to directory admission request)
                    // NOTE: unlike CoveredPlacementDirectoryUpdateRequest, CoveredDirectoryUpdateRequest is a foreground message with foreground events and bandwidth usage
                    const bool is_admit = true;
                    control_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, DirectoryInfo(current_edge_idx), victim_syncset, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
                }
            }
            assert(control_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(control_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: control_request_ptr will be released by edge-to-edge propagation simulator
            control_request_ptr = NULL;

            // Wait for the corresponding control response from the beacon edge node
            DynamicArray control_response_msg_payload;
            bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_wrapper_ptr_->isNodeRunning())
                {
                    is_finish = true; // Edge is NOT running
                    break; // Break from while (true)
                }
                else
                {
                    std::ostringstream oss;
                    oss << "edge timeout to wait for foreground directory update response after hybrid data fetching for key " << key.getKeystr();
                    Util::dumpWarnMsg(instance_name_, oss.str());
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                if (!current_need_placement)
                {
                    assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementHybridFetchedResponse);
                    UNUSED(is_being_written); // NOTE: is_being_written will NOT be used as the current edge node (sender) is NOT a placement
                    const CoveredPlacementHybridFetchedResponse* const covered_placement_hybrid_fetched_response_ptr = static_cast<const CoveredPlacementHybridFetchedResponse*>(control_response_ptr);
                    received_beacon_victim_syncset = covered_placement_hybrid_fetched_response_ptr->getVictimSyncsetRef();
                }
                else
                {
                    if (!current_is_only_placement) // Current edge node is NOT the only placement
                    {
                        assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementDirectoryAdmitResponse);
                        const CoveredPlacementDirectoryAdmitResponse* const covered_placement_directory_admit_response_ptr = static_cast<const CoveredPlacementDirectoryAdmitResponse*>(control_response_ptr);
                        is_being_written = covered_placement_directory_admit_response_ptr->isBeingWritten(); // Used by local edge cache admission later
                        received_beacon_victim_syncset = covered_placement_directory_admit_response_ptr->getVictimSyncsetRef();

                        is_neighbor_cached = covered_placement_directory_admit_response_ptr->isNeighborCached(); // Get is_neighbor_cached for including-sender hybrid fetching
                    }
                    else // Current edge node is the only placement
                    {
                        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);
                        const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                        is_being_written = covered_directory_update_response_ptr->isBeingWritten(); // Used by local edge cache admission later
                        received_beacon_victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();

                        is_neighbor_cached = covered_directory_update_response_ptr->isNeighborCached(); // Get is_neighbor_cached for only-sender hybrid fetching
                    }
                }

                // Victim synchronization
                const uint32_t beacon_edge_idx = control_response_ptr->getSourceIndex();
                UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(beacon_edge_idx, received_beacon_victim_syncset);
                edge_wrapper_ptr_->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

                // Update total bandwidth usage for received control response (directory update after hybrid fetching)
                BandwidthUsage control_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_control_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                control_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_control_rsp_bandwidth_bytes, 0, 0, 1, 0));
                total_bandwidth_usage.update(control_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;

                break; // Break from while (true)
            } // End of (is_timeout == false)
        } // End of while (true)

        if (is_finish)
        {
            return is_finish; // Edge is NOT running now
        }

        // Perform local placement (equivalent an in-advance remote placement notification) if necessary
        if (current_need_placement)
        {
            // (OBSOLETE) NOTE: we do NOT need to notify placement processor of the current sender/closest edge node for local placement, because sender is NOT beacon and waiting for response will NOT block subsequent local/remote placement calculation

            // NOTE: we need to notify placement processor of the current sender/closest edge node for local placement, because we need to use the background directory update requests to DISABLE recursive cache placement and also avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon (similar as EdgeWrapperBase::nonblockNotifyForPlacementInternal_() invoked by local/remote beacon edge node)

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon
            const bool is_valid = !is_being_written;
            bool is_successful = edge_wrapper_ptr_->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, is_neighbor_cached, is_valid, skip_propagation_latency));
            assert(is_successful);
        }

        return is_finish;
    }
}
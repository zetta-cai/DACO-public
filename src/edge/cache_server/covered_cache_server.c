#include "edge/cache_server/covered_cache_server.h"

#include "cache/covered_cache_custom_func_param.h"
#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CoveredCacheServer::kClassName("CoveredCacheServer");

    CoveredCacheServer::CoveredCacheServer(EdgeComponentParam* edge_component_ptr) : CacheServerBase(edge_component_ptr)
    {
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);

        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << tmp_edge_wrapper_ptr->getNodeIdx();
        instance_name_ = oss.str();
    }

    CoveredCacheServer::~CoveredCacheServer()
    {}

    // (1) For local edge cache admission and remote directory admission

    void CoveredCacheServer::admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        bool affect_victim_tracker = false; // If key is a local synced victim now
        const uint32_t beacon_edgeidx = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->admit(key, value, is_neighbor_cached, is_valid, beacon_edgeidx, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
        UpdateCacheManagerForLocalSyncedVictimsFuncParam tmp_param(affect_victim_tracker);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME, &tmp_param);

        // Remove existing cached directory if any as key is local cached, while we ONLY cache valid remote dirinfo for local uncached objects
        tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr()->updateDirectoryCacherToRemoveCachedDirectory(key);

        return;
    }

    MessageBase* CoveredCacheServer::getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        const bool is_admit = true; // Try to admit a new key as local cached object (NOTE: local edge cache has NOT been admitted yet)
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* directory_update_request_ptr = NULL;
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // ONLY need victim synchronization yet without popularity collection/aggregation
        if (!is_background) // Foreground remote directory admission triggered by hybrid data fetching at the sender/closest edge node (different from beacon)
        {
            // (OBSOLETE) NOTE: For COVERED, although there still exist foreground directory update requests for eviction (triggered by local gets to update invalid value and local puts to update cached value), all directory update requests for admission MUST be background due to non-blocking placement deployment

            // NOTE: For COVERED, both directory eviction (triggered by value update and local/remote placement notification) and directory admission (triggered by only-sender hybrid data fetching, fast-path single placement, and local/remote placement notification) can be foreground and background
            directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, source_addr, extra_common_msghdr);
        }
        else // Background remote directory admission triggered by remote placement notification
        {
            // NOTE: use background event names by sending CoveredBgplaceDirectoryUpdateRequest (NOT DISABLE recursive cache placement due to is_admit = true)
            directory_update_request_ptr = new CoveredBgplaceDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, source_addr, extra_common_msghdr);
        }
        assert(directory_update_request_ptr != NULL);

        return directory_update_request_ptr;
    }

    void CoveredCacheServer::processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_neighbor_cached, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        // CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        uint32_t source_edge_idx = control_response_ptr->getSourceIndex();
        VictimSyncset neighbor_victim_syncset;

        // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching, while foreground/background directory admission will NEVER perform placement calculation and hence NO hybrid data fetching
        if (!is_background)
        {
            if (control_response_ptr->getMessageType() != MessageType::kCoveredDirectoryUpdateResponse)
            {
                std::ostringstream oss;
                oss << "Invalid message type " << MessageBase::messageTypeToString(control_response_ptr->getMessageType()) << " in foreground processRspToAdmitBeaconDirectory_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Get is_being_written and victim syncset from control response message
            const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = covered_directory_update_response_ptr->isBeingWritten();
            is_neighbor_cached = covered_directory_update_response_ptr->isNeighborCached();
            neighbor_victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();
        }
        else
        {
            if (control_response_ptr->getMessageType() != MessageType::kCoveredBgplaceDirectoryUpdateResponse)
            {
                std::ostringstream oss;
                oss << "Invalid message type " << MessageBase::messageTypeToString(control_response_ptr->getMessageType()) << " in background processRspToAdmitBeaconDirectory_()";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }

            // Get is_being_written and victim syncset from control response message
            const CoveredBgplaceDirectoryUpdateResponse* const covered_placement_directory_update_response_ptr = static_cast<const CoveredBgplaceDirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = covered_placement_directory_update_response_ptr->isBeingWritten();
            is_neighbor_cached = covered_placement_directory_update_response_ptr->isNeighborCached();
            neighbor_victim_syncset = covered_placement_directory_update_response_ptr->getVictimSyncsetRef();
        }

        // Victim synchronization
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)

    void CoveredCacheServer::evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        tmp_edge_wrapper_ptr->getEdgeCachePtr()->evict(victims, required_size);

        // NOTE: eviction MUST affect victim tracker due to evicting objects with least local rewards (i.e., local synced victims)
        const bool affect_victim_tracker = true;
        UpdateCacheManagerForLocalSyncedVictimsFuncParam tmp_param(affect_victim_tracker);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME, &tmp_param);

        return;
    }

    bool CoveredCacheServer::evictLocalDirectory_(const Key& key, const Value& value, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = tmp_edge_wrapper_ptr->getCooperationWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const bool is_admit = false; // Evict a victim as local uncached object
        bool unused_is_neighbor_cached = false; // NOTE: ONLY need is_neighbor_cached for directory admission to initizalize cached metadata, yet NO need for directory eviction
        MetadataUpdateRequirement metadata_update_requirement;
        bool is_global_cached = tmp_cooperation_wrapper_ptr->updateDirectoryTable(key, current_edge_idx, is_admit, directory_info, is_being_written, unused_is_neighbor_cached, metadata_update_requirement);
        UNUSED(unused_is_neighbor_cached);
        
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation -> while here NOT need piggyacking-based popularity collection and victim synchronization for local directory update

        // Prepare local uncached popularity of key for popularity aggregation
        GetCollectedPopularityParam tmp_param_for_popcollect(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param_for_popcollect); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata

        // Issue metadata update request if necessary, update victim dirinfo, assert NO local uncached popularity, and perform selective popularity aggregation after local directory eviction
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        AfterDirectoryEvictionHelperFuncParam tmp_param_after_direvict(key, current_edge_idx, metadata_update_requirement, directory_info, tmp_param_for_popcollect.getCollectedPopularityConstRef(), is_global_cached, best_placement_edgeset, need_hybrid_fetching, recvrsp_socket_server_ptr, source_addr, total_bandwidth_usage, event_list, extra_common_msghdr, is_background);
        tmp_edge_wrapper_ptr->constCustomFunc(AfterDirectoryEvictionHelperFuncParam::FUNCNAME, &tmp_param_after_direvict);
        is_finish = tmp_param_after_direvict.isFinishConstRef();
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }

        // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching
        if (need_hybrid_fetching)
        {
            assert(!is_background); // Must be foreground local directory eviction (triggered by invalid/valid value update by local get/put and independent admission)

            NonblockNotifyForPlacementFuncParam tmp_param(key, value, best_placement_edgeset, extra_common_msghdr);
            tmp_edge_wrapper_ptr->constCustomFunc(NonblockNotifyForPlacementFuncParam::FUNCNAME, &tmp_param);
        }

        return is_finish;
    }

    MessageBase* CoveredCacheServer::getReqToEvictBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const bool is_admit = false; // Evict a victim as local uncached object (NOTE: local edge cache has already been evicted)
        MessageBase* directory_update_request_ptr = NULL;

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);
    
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Prepare local uncached popularity of key for piggybacking-based popularity collection
        GetCollectedPopularityParam tmp_param(key);
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata (due to selective metadata preservation)

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Need BOTH popularity collection and victim synchronization
        if (!is_background) // Foreground remote directory eviction (triggered by invalid/valid value update by local get/put and independent admission)
        {
            directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, tmp_param.getCollectedPopularityConstRef(), victim_syncset, edge_idx, source_addr, extra_common_msghdr);
        }
        else // Background remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
        {
            // NOTE: use background event names and DISABLE recursive cache placement by sending CoveredBgplaceDirectoryUpdateRequest
            directory_update_request_ptr = new CoveredBgplaceDirectoryUpdateRequest(key, is_admit, directory_info, tmp_param.getCollectedPopularityConstRef(), victim_syncset, edge_idx, source_addr, extra_common_msghdr);
        }

        // NOTE: key MUST NOT have any cached directory, as key is local cached before eviction (even if key may be local uncached and tracked by local uncached metadata due to metadata preservation after eviction, we have NOT lookuped remote directory yet from beacon node)
        CachedDirectory cached_directory;
        bool has_cached_directory = tmp_covered_cache_manager_ptr->accessDirectoryCacherForCachedDirectory(key, cached_directory);
        assert(!has_cached_directory);
        UNUSED(cached_directory);

        assert(directory_update_request_ptr != NULL);
        return directory_update_request_ptr;
    }
    
    bool CoveredCacheServer::processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();

        bool is_finish = false;

        // CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
            
        // Victim synchronization
        const KeyByteVictimsetMessage* directory_update_response_ptr = static_cast<const KeyByteVictimsetMessage*>(control_response_ptr);
        const uint32_t source_edge_idx = directory_update_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = 
        directory_update_response_ptr->getVictimSyncsetRef();
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);
        
        if (!is_background) // Foreground remote directory eviction (triggered by invalid/valid value update by local get/put and independent admission)
        {
            // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching
            const MessageType message_type = control_response_ptr->getMessageType();
            //assert(message_type == MessageType::kCoveredDirectoryUpdateResponse || message_type == MessageType::kCoveredFghybridDirectoryEvictResponse);
            if (message_type == MessageType::kCoveredDirectoryUpdateResponse) // Normal directory eviction response
            {
                // Get is_being_written (UNUSED) from control response message
                const CoveredDirectoryUpdateResponse* covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = covered_directory_update_response_ptr->isBeingWritten();
            }
            else if (message_type == MessageType::kCoveredFghybridDirectoryEvictResponse) // Directory eviction response with hybrid data fetching
            {
                const CoveredFghybridDirectoryEvictResponse* const covered_placement_directory_evict_response_ptr = static_cast<const CoveredFghybridDirectoryEvictResponse*>(control_response_ptr);

                // Get is_being_written (UNUSED) and best placement edgeset from control response message
                is_being_written = covered_placement_directory_evict_response_ptr->isBeingWritten();
                Edgeset best_placement_edgeset = covered_placement_directory_evict_response_ptr->getEdgesetRef();

                // Trigger placement notification remotely at the beacon edge node
                const Key tmp_key = covered_placement_directory_evict_response_ptr->getKey();
                is_finish = notifyBeaconForPlacementAfterHybridFetchInternal_(tmp_key, value, best_placement_edgeset, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr);
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
            assert(control_response_ptr->getMessageType() == MessageType::kCoveredBgplaceDirectoryUpdateResponse);

            // Get is_being_written (UNUSED) from control response message
            const CoveredBgplaceDirectoryUpdateResponse* covered_placement_directory_update_response_ptr = static_cast<const CoveredBgplaceDirectoryUpdateResponse*>(control_response_ptr);
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

            bool& tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = notifyBeaconForPlacementAfterHybridFetchInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr());
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
    bool CoveredCacheServer::notifyBeaconForPlacementAfterHybridFetchInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        Edgeset tmp_best_placement_edgest = best_placement_edgeset; // Deep copy for excluding sender if with local placement

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = getEdgeWrapperPtr();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();
        
        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const uint32_t dst_beacon_edge_idx_for_compression = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
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
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(dst_beacon_edge_idx_for_compression);

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        const uint64_t cur_msg_seqnum = tmp_edge_wrapper_ptr->getAndIncrNodeMsgSeqnum();
        const ExtraCommonMsghdr tmp_extra_common_msghdr(extra_common_msghdr.isSkipPropagationLatency(), extra_common_msghdr.isMonitored(), cur_msg_seqnum); // NOTE: use edge-assigned seqnum instead of client-assigned seqnum

        // Notify result of hybrid data fetching towards the beacon edge node to trigger non-blocking placement notification
        bool is_being_written = false;
        VictimSyncset received_beacon_victim_syncset; // For victim synchronization
        // NOTE: for only-/including-sender hybrid fetching, ONLY consider remote directory lookup/eviction and release writelock instead of local ones
        // -> (i) Remote ones NEVER use coooperation wrapper to get is_neighbor_cached, as sender is NOT beacon here
        // -> (ii) Local ones always need "hybrid fetching" to get value and trigger normal placement (sender MUST be beacon and can get is_neighbor_cached by cooperation wrapper when admiting local directory)
        bool is_neighbor_cached = false; // ONLY used if current_need_placement
        bool is_stale_response = false; // Only recv again instead of send if with a stale response
        while (true) // Timeout-and-retry mechanism
        {
            if (!is_stale_response)
            {
                // Prepare control request message to notify the beacon edge node
                MessageBase* control_request_ptr = NULL;
                if (!current_need_placement) // Current edge node does NOT need placement
                {
                    // Prepare CoveredFghybridHybridFetchedRequest
                    control_request_ptr = new CoveredFghybridHybridFetchedRequest(key, value, victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, tmp_extra_common_msghdr);
                }
                else
                {
                    // NOTE: we cannot optimistically admit valid object into local edge cache first before issuing dirinfo admission request, as clients may get incorrect value if key is being written
                    if (!current_is_only_placement) // Current edge node is NOT the only placement
                    {
                        // Prepare CoveredFghybridDirectoryAdmitRequest (also equivalent to directory admission request)
                        control_request_ptr = new CoveredFghybridDirectoryAdmitRequest(key, value, DirectoryInfo(current_edge_idx), victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, tmp_extra_common_msghdr);
                    }
                    else // Current edge node is the only placement
                    {
                        // Prepare CoveredDirectoryUpdateRequest (NOT trigger placement notification; also equivalent to directory admission request)
                        // NOTE: unlike CoveredBgplaceDirectoryUpdateRequest, CoveredDirectoryUpdateRequest is a foreground message with foreground events and bandwidth usage
                        const bool is_admit = true;
                        control_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, DirectoryInfo(current_edge_idx), victim_syncset, current_edge_idx, recvrsp_source_addr, tmp_extra_common_msghdr);
                    }
                }
                assert(control_request_ptr != NULL);

                // Push the control request into edge-to-edge propagation simulator to the beacon node
                bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(control_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
                assert(is_successful);

                // NOTE: control_request_ptr will be released by edge-to-edge propagation simulator
                control_request_ptr = NULL;
            }

            // Wait for the corresponding control response from the beacon edge node
            DynamicArray control_response_msg_payload;
            bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true; // Edge is NOT running
                    break; // Break from while (true)
                }
                else
                {
                    std::ostringstream oss;
                    oss << "edge timeout to wait for foreground directory update response after hybrid data fetching for key " << key.getKeyDebugstr() << " from beacon " << dst_beacon_edge_idx_for_compression;
                    Util::dumpWarnMsg(instance_name_, oss.str());
                    is_stale_response = false; // Reset to re-send request
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                // Check if the received message is a stale response
                if (control_response_ptr->getExtraCommonMsghdr().getMsgSeqnum() != cur_msg_seqnum)
                {
                    is_stale_response = true; // ONLY recv again instead of send if with a stale response

                    std::ostringstream oss_for_stable_response;
                    oss_for_stable_response << "stale response " << MessageBase::messageTypeToString(control_response_ptr->getMessageType()) << " with seqnum " << control_response_ptr->getExtraCommonMsghdr().getMsgSeqnum() << " != " << cur_msg_seqnum;
                    Util::dumpWarnMsg(instance_name_, oss_for_stable_response.str());

                    delete control_response_ptr;
                    control_response_ptr = NULL;
                    continue; // Jump to while loop
                }

                if (!current_need_placement)
                {
                    assert(control_response_ptr->getMessageType() == MessageType::kCoveredFghybridHybridFetchedResponse);
                    UNUSED(is_being_written); // NOTE: is_being_written will NOT be used as the current edge node (sender) is NOT a placement
                    const CoveredFghybridHybridFetchedResponse* const covered_placement_hybrid_fetched_response_ptr = static_cast<const CoveredFghybridHybridFetchedResponse*>(control_response_ptr);
                    received_beacon_victim_syncset = covered_placement_hybrid_fetched_response_ptr->getVictimSyncsetRef();
                }
                else
                {
                    if (!current_is_only_placement) // Current edge node is NOT the only placement
                    {
                        assert(control_response_ptr->getMessageType() == MessageType::kCoveredFghybridDirectoryAdmitResponse);
                        const CoveredFghybridDirectoryAdmitResponse* const covered_placement_directory_admit_response_ptr = static_cast<const CoveredFghybridDirectoryAdmitResponse*>(control_response_ptr);
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
                tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);

                // Update total bandwidth usage for received control response (directory update after hybrid fetching)
                BandwidthUsage control_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_control_rsp_bandwidth_bytes = control_response_ptr->getMsgBandwidthSize();
                control_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_control_rsp_bandwidth_bytes, 0, 0, 1, 0, control_response_ptr->getMessageType(), control_response_ptr->getVictimSyncsetBytes()));
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

            // NOTE: we need to notify placement processor of the current sender/closest edge node for local placement, because we need to use the background directory update requests to DISABLE recursive cache placement and also avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon (similar as CoveredEdgeWrapper::nonblockNotifyForPlacementInternal_() invoked by local/remote beacon edge node)

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon
            const bool is_valid = !is_being_written;
            bool is_successful = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, is_neighbor_cached, is_valid, tmp_extra_common_msghdr));
            assert(is_successful);
        }

        return is_finish;
    }
}
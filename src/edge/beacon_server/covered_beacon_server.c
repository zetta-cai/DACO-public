#include "edge/beacon_server/covered_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "event/event.h"
#include "event/event_list.h"
#include "message/data_message.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredBeaconServer::kClassName("CoveredBeaconServer");

    CoveredBeaconServer::CoveredBeaconServer(EdgeWrapper* edge_wrapper_ptr) : BeaconServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate CoveredBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredBeaconServer::~CoveredBeaconServer() {}

    // (1) Access content directory information

    bool CoveredBeaconServer::processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const
    {
        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredDirectoryLookupRequest);
        const CoveredDirectoryLookupRequest* const covered_directory_lookup_request_ptr = static_cast<const CoveredDirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = covered_directory_lookup_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        bool is_finish = false;

        // Lookup cooperation wrapper to get valid directory information if any
        // Also check whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        const uint32_t source_edge_idx = covered_directory_lookup_request_ptr->getSourceIndex();
        is_being_written = false;
        bool is_source_cached = false;
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupDirectoryTableByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& neighbor_victim_syncset = covered_directory_lookup_request_ptr->getVictimSyncsetRef();
        covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr_->getCooperationWrapperPtr());

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "before updatePopularityAggregatorForAggregatedPopularity for key", tmp_key.getKeystr().c_str());

        // Selective popularity aggregation
        const CollectedPopularity& collected_popularity = covered_directory_lookup_request_ptr->getCollectedPopularityRef();
        const bool need_placement_calculation = true;
        const bool sender_is_beacon = false; // Sender and beacon are two different edge nodes
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
        is_finish = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, edge_wrapper_ptr_, edge_beacon_server_recvrsp_source_addr_, edge_beacon_server_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, &fast_path_hint); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "after updatePopularityAggregatorForAggregatedPopularity for key", tmp_key.getKeystr().c_str());

        // NOTE: need_hybrid_fetching with best_placement_edgeset is processed in CoveredBeaconServer::getRspToLookupLocalDirectory_() to issue corresponding control response message for hybrid data fetching

        return is_finish;
    }

    MessageBase* CoveredBeaconServer::getRspToLookupLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Get key and skip_propagation_latency from directory lookup request
        assert(control_request_ptr != NULL);
        const Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "before control_request_ptr->getSourceIndex() for key", tmp_key.getKeystr().c_str());

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = control_request_ptr->getSourceIndex();

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "before accessVictimTrackerForLocalVictimSyncset for key", tmp_key.getKeystr().c_str());

        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        // TMPDEBUGTMPDEBUG
        Util::dumpVariablesForDebug(instance_name_, 2, "after accessVictimTrackerForLocalVictimSyncset for key", tmp_key.getKeystr().c_str());

        const uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_lookup_response_ptr = NULL;
        if (need_hybrid_fetching) // Directory lookup response w/ hybrid data fetching
        {
            // NOTE: if need_hybrid_fetching == true, key MUST be tracked by sender local uncached metadata and hence NO need fast-path single-placement calculation

            // NOTE: beacon node uses best_placement_edgeset to tell the closest edge node if to perform hybrid data fetching and trigger non-blocking placement notification
            covered_directory_lookup_response_ptr = new CoveredPlacementDirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, best_placement_edgeset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else // Normal directory lookup response
        {
            #ifdef ENABLE_FAST_PATH_PLACEMENT
            if (!fast_path_hint.isValid()) // Normal directory lookup rsp w/o fast-path placement
            {
                covered_directory_lookup_response_ptr = new CoveredDirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
            else // Corresponding local getreq may be the first cache miss w/o objsize -> need fast-path placement
            {
                covered_directory_lookup_response_ptr = new CoveredFastDirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, fast_path_hint, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
            #else
            covered_directory_lookup_response_ptr = new CoveredDirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            #endif
        }
        assert(covered_directory_lookup_response_ptr != NULL);

        return covered_directory_lookup_response_ptr;
    }

    bool CoveredBeaconServer::processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        checkPointers_();
        assert(control_request_ptr != NULL);

        bool is_finish = false;

        // Get key and directory update information from control request
        uint32_t source_edge_idx = control_request_ptr->getSourceIndex();
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        MessageType message_type = control_request_ptr->getMessageType();
        Key tmp_key;
        VictimSyncset neighbor_victim_syncset;
        bool is_directory_update = false;
        bool is_admit = false; // ONLY used if is_directory_update = true
        DirectoryInfo directory_info; // ONLY used if is_directory_update = true
        CollectedPopularity collected_popularity; // ONLY used if is_directory_update = true and is_admit = false
        bool with_extra_hybrid_fetching_result = false; // If with extra hybrid fetching result (except/besides sender) to trigger non-blocking placement notification
        Value tmp_value; // ONLY used if  with_extra_hybrid_fetching_result = true
        Edgeset extra_placement_edgeset; // ONLY used if with_extra_hybrid_fetching_result = true
        if (message_type == MessageType::kCoveredPlacementDirectoryUpdateRequest) // Background directory updates w/o hybrid data fetching for COVERED
        {
            // Get key, is_admit, directory info, victim syncset, and collected popularity (if any) from directory update request
            const CoveredPlacementDirectoryUpdateRequest* const covered_placement_directory_update_request_ptr = static_cast<const CoveredPlacementDirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = covered_placement_directory_update_request_ptr->getKey();
            neighbor_victim_syncset = covered_placement_directory_update_request_ptr->getVictimSyncsetRef();

            is_directory_update = true;
            is_admit = covered_placement_directory_update_request_ptr->isValidDirectoryExist();
            directory_info = covered_placement_directory_update_request_ptr->getDirectoryInfo();
            if (!is_admit)
            {
                collected_popularity = covered_placement_directory_update_request_ptr->getCollectedPopularityRef();
            }

            with_extra_hybrid_fetching_result = false; // NO hybrid data fetching result
        }
        else if (message_type == kCoveredPlacementHybridFetchedRequest) // Foreground request to notify the result of excluding-sender hybrid data fetching for COVERED (NO directory update)
        {
            const CoveredPlacementHybridFetchedRequest* const covered_placement_hybrid_fetched_request_ptr = static_cast<const CoveredPlacementHybridFetchedRequest*>(control_request_ptr);
            tmp_key = covered_placement_hybrid_fetched_request_ptr->getKey();
            neighbor_victim_syncset = covered_placement_hybrid_fetched_request_ptr->getVictimSyncsetRef();

            is_directory_update = false;

            with_extra_hybrid_fetching_result = true; // With extra hybrid data fetching result (except sender)
            tmp_value = covered_placement_hybrid_fetched_request_ptr->getValue();
            extra_placement_edgeset = covered_placement_hybrid_fetched_request_ptr->getEdgesetRef();
        }
        else if (message_type == MessageType::kCoveredPlacementDirectoryAdmitRequest) // Foreground directory admission with including-sender hybrid data fetching for COVERED
        {
            // Get key, is_admit, directory info, victim syncset, and collected popularity (if any) from directory update request
            const CoveredPlacementDirectoryAdmitRequest* const covered_placement_directory_admit_request_ptr = static_cast<const CoveredPlacementDirectoryAdmitRequest*>(control_request_ptr);
            tmp_key = covered_placement_directory_admit_request_ptr->getKey();
            neighbor_victim_syncset = covered_placement_directory_admit_request_ptr->getVictimSyncsetRef();

            is_directory_update = true;
            is_admit = true;
            directory_info = covered_placement_directory_admit_request_ptr->getDirectoryInfo();
            // NOTE: must NO collected popularity as is_admit = true for directory admission

            with_extra_hybrid_fetching_result = true; // With extra hybrid data fetching result (besides sender)
            tmp_value = covered_placement_directory_admit_request_ptr->getValue();
            extra_placement_edgeset = covered_placement_directory_admit_request_ptr->getEdgesetRef();
        }
        else if (message_type == MessageType::kCoveredDirectoryUpdateRequest) // Foreground directory updates (with only-sender hybrid data fetching for COVERED if is_admit = true)
        {
            // Get key, is_admit, directory info, victim syncset, and collected popularity (if any) from directory update request
            const CoveredDirectoryUpdateRequest* const covered_directory_update_request_ptr = static_cast<const CoveredDirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = covered_directory_update_request_ptr->getKey();
            neighbor_victim_syncset = covered_directory_update_request_ptr->getVictimSyncsetRef();

            is_directory_update = true;
            is_admit = covered_directory_update_request_ptr->isValidDirectoryExist();
            directory_info = covered_directory_update_request_ptr->getDirectoryInfo();
            if (!is_admit)
            {
                collected_popularity = covered_directory_update_request_ptr->getCollectedPopularityRef();
            }

            with_extra_hybrid_fetching_result = false; // Without extra hybrid data fetching result (besides sender)
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << message_type << " for processReqToUpdateLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr_->getCooperationWrapperPtr());

        if (is_directory_update) // Update directory information
        {
            // Update local directory information in cooperation wrapper
            is_being_written = false;
            bool is_source_cached = false;
            bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, source_edge_idx, is_admit, directory_info, is_being_written, is_source_cached);

            // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
            covered_cache_manager_ptr->updateVictimTrackerForLocalBeaconedVictimDirinfo(tmp_key, is_admit, directory_info);

            if (is_admit) // Admit a new key as local cached object
            {
                // (OBSOLETE) NOTE: For COVERED, although there still exist foreground directory update requests for eviction (triggered by local gets to update invalid value and local puts to update cached value), all directory update requests for admission MUST be background due to non-blocking placement deployment
                //assert(control_request_ptr->isBackgroundRequest());

                // NOTE: For COVERED, both directory eviction (triggered by value update and local/remote placement notification) and directory admission (triggered by only-sender hybrid data fetching, fast-path single placement, and local/remote placement notification) can be foreground and background

                // Clear preserved edge nodes for the given key at the source edge node for metadata releasing after local/remote admission notification
                covered_cache_manager_ptr->clearPopularityAggregatorForPreservedEdgesetAfterAdmission(tmp_key, source_edge_idx);
            }
            else // Evict a victim as local uncached object
            {
                assert(!with_extra_hybrid_fetching_result); // NOTE: hybrid data fetching result must be embedded into directory admission instead of eviction to avoid recursive hybrid data fetching

                bool need_placement_calculation = !is_admit; // MUST be true here for is_admit = false
                if (control_request_ptr->isBackgroundRequest()) // If processReqToUpdateLocalDirectory_() is triggered by background cache placement
                {
                    // NOTE: DISABLE recursive cache placement for CoveredPlacementDirectoryUpdateRequest
                    // NOTE: CoveredPlacementHybridFetchedRequest will NOT arrive here due to is_directory_update = false; CoveredPlacementDirectoryAdmitRequest will also NOT arrive here due to is_admit = true
                    // NOTE: NOT DISABLE cache placement for CoveredDirectoryUpdateRequest, as foreground directory eviction is allowed to perform placement calculation (while local/remote placement notification must issue CoveredPlacementDirectoryUpdateRequest for background directory eviction to avoid recursive cache placement)
                    need_placement_calculation = false;
                }

                // NOTE: MUST NO old local uncached popularity for the newly evicted object at the source edge node before popularity aggregation
                covered_cache_manager_ptr->assertNoLocalUncachedPopularity(tmp_key, source_edge_idx);

                // Selective popularity aggregation
                const bool sender_is_beacon = false; // Sender is NOT the beacon
                best_placement_edgeset.clear();
                need_hybrid_fetching = false;
                is_finish = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, edge_wrapper_ptr_, edge_beacon_server_recvrsp_source_addr_, edge_beacon_server_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
                if (is_finish)
                {
                    return is_finish; // Edge node is finished
                }
            }
        }
        
        if (with_extra_hybrid_fetching_result) // With extra hybrid fetching result (except/besides sender) to trigger non-blocking placement notification
        {
            if (!is_directory_update) // is_being_written has NOT been set
            {
                is_being_written = edge_wrapper_ptr_->getCooperationWrapperPtr()->isBeingWritten(tmp_key);
            }

            // NOTE: source edge index must NOT in the extra placement edgeset, as placement edge idx of sender if any has already been removed in CacheServerWorkerBase::notifyBeaconForPlacementAfterHybridFetch_()
            for (std::unordered_set<uint32_t>::const_iterator extra_placement_edgeset_const_iter = extra_placement_edgeset.begin(); extra_placement_edgeset_const_iter != extra_placement_edgeset.end(); extra_placement_edgeset_const_iter++)
            {
                if (source_edge_idx == *extra_placement_edgeset_const_iter)
                {
                    std::ostringstream oss;
                    oss << "Source edge index " << source_edge_idx << " should NOT in the extra placement edgeset for key " << tmp_key.getKeystr() << " in processReqToUpdateLocalDirectory_()";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }
            }

            // Trigger non-blocking placement notification for the extra hybrid fetching result (ONLY for COVERED)
            assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
            edge_wrapper_ptr_->nonblockNotifyForPlacement(tmp_key, tmp_value, extra_placement_edgeset, skip_propagation_latency);
        }

        return is_finish;
    }

    MessageBase* CoveredBeaconServer::getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(control_request_ptr != NULL);

        const Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = control_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        // Send back corresponding response based on request type
        MessageType message_type = control_request_ptr->getMessageType();
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* control_response_ptr = NULL;
        if (message_type == MessageType::kCoveredPlacementDirectoryUpdateRequest) // Background directory updates w/o hybrid data fetching for COVERED
        {
            assert(!need_hybrid_fetching); // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching

            control_response_ptr = new CoveredPlacementDirectoryUpdateResponse(tmp_key, is_being_written, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (message_type == MessageType::kCoveredPlacementHybridFetchedRequest) // Foreground request to notify the result of excluding-sender hybrid data fetching for COVERED (NO directory update)
        {
            assert(!need_hybrid_fetching); // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching

            control_response_ptr = new CoveredPlacementHybridFetchedResponse(tmp_key, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (message_type == MessageType::kCoveredPlacementDirectoryAdmitRequest) // Foreground directory admission with including-sender hybrid data fetching for COVERED
        {
            assert(!need_hybrid_fetching); // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching

            control_response_ptr = new CoveredPlacementDirectoryAdmitResponse(tmp_key, is_being_written, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else if (message_type == MessageType::kCoveredDirectoryUpdateRequest) // Foreground directory updates (with only-sender hybrid data fetching for COVERED if is_admit = true)
        {
            if (need_hybrid_fetching) // Directory update (must be eviction) response w/ hybrid data fetching
            {
                // NOTE: must be foreground directory eviction
                assert(!control_request_ptr->isBackgroundRequest());
                const CoveredDirectoryUpdateRequest* const covered_directory_update_request_ptr = static_cast<const CoveredDirectoryUpdateRequest*>(control_request_ptr);
                assert(!covered_directory_update_request_ptr->isValidDirectoryExist());

                // NOTE: beacon node uses best_placement_edgeset to tell the closest edge node if to perform hybrid data fetching and trigger non-blocking placement notification
                control_response_ptr = new CoveredPlacementDirectoryEvictResponse(tmp_key, is_being_written, victim_syncset, best_placement_edgeset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
            else // Normal directory update response (foreground directory admission/eviction)
            {
                control_response_ptr = new CoveredDirectoryUpdateResponse(tmp_key, is_being_written, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << message_type << " for getRspToUpdateLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(control_response_ptr != NULL);

        return control_response_ptr;
    }

    // (2) Process writes and unblock for MSI protocol

    bool CoveredBeaconServer::processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredAcquireWritelockRequest);
        const CoveredAcquireWritelockRequest* const covered_acquire_writelock_request_ptr = static_cast<const CoveredAcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = covered_acquire_writelock_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        bool is_finish = false;

        // Try to acquire a write lock for the given key if key is global cached
        const uint32_t source_edge_idx = covered_acquire_writelock_request_ptr->getSourceIndex();
        bool is_source_cached = false;
        lock_result = edge_wrapper_ptr_->getCooperationWrapperPtr()->acquireLocalWritelockByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, all_dirinfo, is_source_cached);
        bool is_global_cached = (lock_result != LockResult::kNoneed);

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for remote acquire write lock on local cached objects in source edge node, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as collected_popularity.is_tracked_ MUST be false if key is local cached in source edge node and will NOT update/add aggregated uncached popularity

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& neighbor_victim_syncset = covered_acquire_writelock_request_ptr->getVictimSyncsetRef();
        covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr_->getCooperationWrapperPtr());

        // Selective popularity aggregation
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid even after cache placement
        const CollectedPopularity& collected_popularity = covered_acquire_writelock_request_ptr->getCollectedPopularityRef();
        const bool need_placement_calculation = false;
        const bool sender_is_beacon = false; // Sender is NOT the beacon
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
        is_finish = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, edge_wrapper_ptr_, edge_beacon_server_recvrsp_source_addr_, edge_beacon_server_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // NOTE: remote acquire writelock request MUST NOT need hybrid data fetching due to NO need for placement calculation (newly-admitted cache copies will still be invalid after cache placement)
        assert(!is_finish); // NO victim fetching due to NO placement calculation
        assert(best_placement_edgeset.size() == 0); // NO placement deployment due to NO placement calculation
        assert(!need_hybrid_fetching); // Also NO hybrid data fetching

        return is_finish;
    }

    MessageBase* CoveredBeaconServer::getRspToAcquireLocalWritelock_(MessageBase* control_request_ptr, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Get key and skip_propagation_latency from acquire writelock request
        Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = control_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_acquire_writelock_response_ptr = new CoveredAcquireWritelockResponse(tmp_key, lock_result, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_acquire_writelock_response_ptr != NULL);

        return covered_acquire_writelock_response_ptr;
    }

    bool CoveredBeaconServer::processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredReleaseWritelockRequest);
        const CoveredReleaseWritelockRequest* const covered_release_writelock_request_ptr = static_cast<const CoveredReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = covered_release_writelock_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        bool is_finish = false;

        // Release local write lock and validate sender directory info if any
        uint32_t sender_edge_idx = covered_release_writelock_request_ptr->getSourceIndex();
        DirectoryInfo sender_directory_info(sender_edge_idx);
        bool is_source_cached = false;
        blocked_edges = edge_wrapper_ptr_->getCooperationWrapperPtr()->releaseLocalWritelock(tmp_key, sender_edge_idx, sender_directory_info, is_source_cached);

        // OBSOLETE: NO need to remove old local uncached popularity from aggregated uncached popularity for remote release write lock on local cached objects in source edge node, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as collected_popularity.is_tracked_ MUST be false if key is local cached in source edge node and will NOT update/add aggregated uncached popularity

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& neighbor_victim_syncset = covered_release_writelock_request_ptr->getVictimSyncsetRef();
        covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(sender_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr_->getCooperationWrapperPtr());

        // Selective popularity aggregation
        const CollectedPopularity& collected_popularity = covered_release_writelock_request_ptr->getCollectedPopularityRef();
        const bool is_global_cached = true; // NOTE: receiving remote release writelock request means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        const bool need_placement_calculation = true;
        const bool sender_is_beacon = false; // Sender is NOT the beacon
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;
        const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
        is_finish = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, sender_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, edge_wrapper_ptr_, edge_beacon_server_recvrsp_source_addr_, edge_beacon_server_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
        if (is_finish)
        {
            return is_finish; // Edge node is finished
        }

        return is_finish;
    }

    MessageBase* CoveredBeaconServer::getRspToReleaseLocalWritelock_(MessageBase* control_request_ptr, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Get key and skip_propagation_latency from release writelock request
        Key tmp_key = MessageBase::getKeyFromMessage(control_request_ptr);
        bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = control_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_release_writelock_response_ptr = NULL;
        if (need_hybrid_fetching)
        {
            covered_release_writelock_response_ptr = new CoveredPlacementReleaseWritelockResponse(tmp_key, victim_syncset, best_placement_edgeset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else // Normal release writelock response
        {
            covered_release_writelock_response_ptr = new CoveredReleaseWritelockResponse(tmp_key, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        assert(covered_release_writelock_response_ptr != NULL);

        return covered_release_writelock_response_ptr;
    }

    // (3) Process other control requests

    bool CoveredBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        return false;
    }

    // (4) Process redirected/global get response for non-blocking placement deployment (ONLY for COVERED)

    void CoveredBeaconServer::processRspToRedirectGetForPlacement_(MessageBase* redirected_get_response_ptr)
    {
        checkPointers_();
        assert(redirected_get_response_ptr != NULL);
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        const CoveredPlacementRedirectedGetResponse* const covered_placement_redirected_get_response_ptr = static_cast<const CoveredPlacementRedirectedGetResponse*>(redirected_get_response_ptr);
        //const Value tmp_value = covered_placement_redirected_get_response_ptr->getValue();

        // Victim synchronization
        const uint32_t sender_edge_idx = covered_placement_redirected_get_response_ptr->getSourceIndex();
        const VictimSyncset& neighbor_victim_syncset = covered_placement_redirected_get_response_ptr->getVictimSyncsetRef();
        covered_cache_manager_ptr->updateVictimTrackerForNeighborVictimSyncset(sender_edge_idx, neighbor_victim_syncset, edge_wrapper_ptr_->getCooperationWrapperPtr());

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        const EventList& background_event_list = covered_placement_redirected_get_response_ptr->getEventListRef();
        BandwidthUsage background_bandwidth_usage = covered_placement_redirected_get_response_ptr->getBandwidthUsageRef();
        uint32_t cross_edge_redirected_get_rsp_bandwidth_bytes = covered_placement_redirected_get_response_ptr->getMsgPayloadSize();
        background_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_rsp_bandwidth_bytes, 0));
        edge_wrapper_ptr_->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(background_bandwidth_usage);
        edge_wrapper_ptr_->getEdgeBackgroundCounterForBeaconServerRef().addEvents(background_event_list);

        // Get Hitflag for non-blocking placement deployment
        const Key tmp_key = covered_placement_redirected_get_response_ptr->getKey();
        const Hitflag tmp_hitflag = covered_placement_redirected_get_response_ptr->getHitflag();
        const bool skip_propagation_latency = redirected_get_response_ptr->isSkipPropagationLatency();
        const Edgeset& best_placement_edgeset = covered_placement_redirected_get_response_ptr->getEdgesetRef();
        assert(best_placement_edgeset.size() <= edge_wrapper_ptr_->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time
        if (tmp_hitflag == Hitflag::kCooperativeHit)
        {
            // Perform non-blocking placement notification for neighbor data fetching
            const Value tmp_value = covered_placement_redirected_get_response_ptr->getValue();
            edge_wrapper_ptr_->nonblockNotifyForPlacement(tmp_key, tmp_value, best_placement_edgeset, skip_propagation_latency);
        }
        else // Cooperative invalid or global miss
        {
            // NOTE: as we have replied the sender without hybrid data fetching before, beacon server directly fetches data from cloud by itself here in a non-blocking manner (this is a corner case, as valid dirinfo has cooperative hit in most time)
            edge_wrapper_ptr_->nonblockDataFetchFromCloudForPlacement(tmp_key, best_placement_edgeset, skip_propagation_latency);
        }

        return;
    }

    void CoveredBeaconServer::processRspToAccessCloudForPlacement_(MessageBase* global_get_response_ptr)
    {
        checkPointers_();
        assert(global_get_response_ptr != NULL);
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);

        const CoveredPlacementGlobalGetResponse* const covered_placement_global_get_response_ptr = static_cast<const CoveredPlacementGlobalGetResponse*>(global_get_response_ptr);
        //const Value tmp_value = covered_placement_global_get_response_ptr->getValue();

        // NOTE: NO need for victim synchronization due to edge-cloud communication

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        const EventList& background_event_list = covered_placement_global_get_response_ptr->getEventListRef();
        BandwidthUsage background_bandwidth_usage = covered_placement_global_get_response_ptr->getBandwidthUsageRef();
        uint32_t edge_cloud_global_get_rsp_bandwidth_bytes = covered_placement_global_get_response_ptr->getMsgPayloadSize();
        background_bandwidth_usage.update(BandwidthUsage(0, 0, edge_cloud_global_get_rsp_bandwidth_bytes));
        edge_wrapper_ptr_->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(background_bandwidth_usage);
        edge_wrapper_ptr_->getEdgeBackgroundCounterForBeaconServerRef().addEvents(background_event_list);

        // Perform non-blocking placement notification for cloud data fetching
        const Key& tmp_key = covered_placement_global_get_response_ptr->getKey();
        const Value& tmp_value = covered_placement_global_get_response_ptr->getValue();
        const bool skip_propagation_latency = global_get_response_ptr->isSkipPropagationLatency();
        const Edgeset& best_placement_edgeset = covered_placement_global_get_response_ptr->getEdgesetRef();
        assert(best_placement_edgeset.size() <= edge_wrapper_ptr_->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time
        edge_wrapper_ptr_->nonblockNotifyForPlacement(tmp_key, tmp_value, best_placement_edgeset, skip_propagation_latency);
    }
}
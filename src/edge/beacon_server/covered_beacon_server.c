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

    void CoveredBeaconServer::processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // TODO: For COVERED, beacon node will tell the closest edge node if to admit, w/o independent decision (trade-off-aware admission placement and eviction)

        // Get key from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredDirectoryLookupRequest);
        const CoveredDirectoryLookupRequest* const covered_directory_lookup_request_ptr = static_cast<const CoveredDirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = covered_directory_lookup_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Lookup cooperation wrapper to get valid directory information if any
        // Also check whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        const uint32_t source_edge_idx = covered_directory_lookup_request_ptr->getSourceIndex();
        bool is_source_cached = false;
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupDirectoryTableByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& victim_syncset = covered_directory_lookup_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // Selective popularity aggregation
        const CollectedPopularity& collected_popularity = covered_directory_lookup_request_ptr->getCollectedPopularityRef();
        const bool need_placement_calculation = true;
        Edgeset best_placement_edgeset;
        bool has_best_placement = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, best_placement_edgeset); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // Non-blocking data fetching if with best placement
        if (need_placement_calculation && has_best_placement)
        {
            const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
            edge_wrapper_ptr_->nonblockDataFetchForPlacement(tmp_key, best_placement_edgeset, skip_propagation_latency);
        }

        return;
    }

    MessageBase* CoveredBeaconServer::getRspToLookupLocalDirectory_(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_lookup_response_ptr = new CoveredDirectoryLookupResponse(key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_directory_lookup_response_ptr != NULL);

        return covered_directory_lookup_response_ptr;
    }

    bool CoveredBeaconServer::processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr)
    {
        // Get key and directory update information from control request
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateRequest);
        const CoveredDirectoryUpdateRequest* const covered_directory_update_request_ptr = static_cast<const CoveredDirectoryUpdateRequest*>(control_request_ptr);
        uint32_t source_edge_idx = covered_directory_update_request_ptr->getSourceIndex();
        Key tmp_key = covered_directory_update_request_ptr->getKey();
        bool is_admit = covered_directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = covered_directory_update_request_ptr->getDirectoryInfo();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Update local directory information in cooperation wrapper
        bool is_being_written = false;
        bool is_source_cached = false;
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, source_edge_idx, is_admit, directory_info, is_being_written, is_source_cached);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        covered_cache_manager_ptr->updateVictimTrackerForLocalBeaconedVictimDirinfo(tmp_key, is_admit, directory_info);

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& victim_syncset = covered_directory_update_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        if (is_admit) // Admit a new key as local cached object
        {
            // Clear old local uncached popularity (TODO: preserved edge idx / bitmap) for the given key at soure edge node after admission
            covered_cache_manager_ptr->clearPopularityAggregatorAfterAdmission(tmp_key, source_edge_idx);
        }
        else // Evict a victim as local uncached object
        {
            // Selective popularity aggregation
            const CollectedPopularity& collected_popularity = covered_directory_update_request_ptr->getCollectedPopularityRef();
            const bool need_placement_calculation = !is_admit; // MUST be true here for is_admit = false
            Edgeset best_placement_edgeset;
            bool has_best_placement = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, best_placement_edgeset); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

            // Non-blocking data fetching if with best placement
            if (need_placement_calculation && has_best_placement)
            {
                const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
                edge_wrapper_ptr_->nonblockDataFetchForPlacement(tmp_key, best_placement_edgeset, skip_propagation_latency);
            }
        }

        return is_being_written;
    }

    MessageBase* CoveredBeaconServer::getRspToUpdateLocalDirectory_(const Key& key, const bool& is_being_written, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_update_response_ptr = new CoveredDirectoryUpdateResponse(key, is_being_written, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_directory_update_response_ptr != NULL);

        return covered_directory_update_response_ptr;
    }

    // (2) Process writes and unblock for MSI protocol

    void CoveredBeaconServer::processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredAcquireWritelockRequest);
        const CoveredAcquireWritelockRequest* const covered_acquire_writelock_request_ptr = static_cast<const CoveredAcquireWritelockRequest*>(control_request_ptr);
        Key tmp_key = covered_acquire_writelock_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Try to acquire a write lock for the given key if key is global cached
        const uint32_t source_edge_idx = covered_acquire_writelock_request_ptr->getSourceIndex();
        bool is_source_cached = false;
        lock_result = edge_wrapper_ptr_->getCooperationWrapperPtr()->acquireLocalWritelockByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, all_dirinfo, is_source_cached);
        bool is_global_cached = (lock_result != LockResult::kNoneed);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for remote acquire write lock on local cached objects in source edge node, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as collected_popularity.is_tracked_ MUST be false if key is local cached in source edge node and will NOT update/add aggregated uncached popularity

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& victim_syncset = covered_acquire_writelock_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // Selective popularity aggregation
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid even after cache placement
        const CollectedPopularity& collected_popularity = covered_acquire_writelock_request_ptr->getCollectedPopularityRef();
        const bool need_placement_calculation = false;
        Edgeset best_placement_edgeset;
        bool has_best_placement = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, best_placement_edgeset); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
        assert(!has_best_placement);
        UNUSED(best_placement_edgeset);

        return;
    }

    MessageBase* CoveredBeaconServer::getRspToAcquireLocalWritelock_(const Key& key, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_acquire_writelock_response_ptr = new CoveredAcquireWritelockResponse(key, lock_result, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_acquire_writelock_response_ptr != NULL);

        return covered_acquire_writelock_response_ptr;
    }

    void CoveredBeaconServer::processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges)
    {
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kCoveredReleaseWritelockRequest);
        const CoveredReleaseWritelockRequest* const covered_release_writelock_request_ptr = static_cast<const CoveredReleaseWritelockRequest*>(control_request_ptr);
        Key tmp_key = covered_release_writelock_request_ptr->getKey();

        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Release local write lock and validate sender directory info if any
        uint32_t sender_edge_idx = covered_release_writelock_request_ptr->getSourceIndex();
        DirectoryInfo sender_directory_info(sender_edge_idx);
        bool is_source_cached = false;
        blocked_edges = edge_wrapper_ptr_->getCooperationWrapperPtr()->releaseLocalWritelock(tmp_key, sender_edge_idx, sender_directory_info, is_source_cached);

        // OBSELETE: NO need to remove old local uncached popularity from aggregated uncached popularity for remote release write lock on local cached objects in source edge node, as it MUST have been removed by directory update request with is_admit = true during non-blocking admission placement
        // NOTE: NO need to check if key is local cached or not, as collected_popularity.is_tracked_ MUST be false if key is local cached in source edge node and will NOT update/add aggregated uncached popularity

        // Victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        const VictimSyncset& victim_syncset = covered_release_writelock_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(sender_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        // Selective popularity aggregation
        const CollectedPopularity& collected_popularity = covered_release_writelock_request_ptr->getCollectedPopularityRef();
        const bool is_global_cached = true; // NOTE: receiving remote release writelock request means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        const bool need_placement_calculation = true;
        Edgeset best_placement_edgeset;
        bool has_best_placement = covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, sender_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, best_placement_edgeset); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // Non-blocking data fetching if with best placement
        if (need_placement_calculation && has_best_placement)
        {
            const bool skip_propagation_latency = control_request_ptr->isSkipPropagationLatency();
            edge_wrapper_ptr_->nonblockDataFetchForPlacement(tmp_key, best_placement_edgeset, skip_propagation_latency);
        }

        return;
    }

    MessageBase* CoveredBeaconServer::getRspToReleaseLocalWritelock_(const Key& key, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_release_writelock_response_ptr = new CoveredReleaseWritelockResponse(key, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(covered_release_writelock_response_ptr != NULL);

        return covered_release_writelock_response_ptr;
    }

    // (3) Process other control requests

    bool CoveredBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        return false;
    }

    // (4) Process redirected get response for non-blocking placement deployment (ONLY for COVERED)

    void CoveredBeaconServer::processRspToRedirectGetForPlacement_(MessageBase* redirected_get_response_ptr)
    {
        checkPointers_();
        assert(redirected_get_response_ptr != NULL);
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);

        // Get key, value, hitflag, victim syncset, and edgeset
        const CoveredPlacementRedirectedGetResponse* const covered_placement_redirected_get_response_ptr = static_cast<const CoveredPlacementRedirectedGetResponse*>(redirected_get_response_ptr);
        const Key tmp_key = covered_placement_redirected_get_response_ptr->getKey();
        const Value tmp_value = covered_placement_redirected_get_response_ptr->getValue();
        const bool tmp_hitflag = covered_placement_redirected_get_response_ptr->getHitflag();
        const VictimSyncset& victim_syncset = covered_placement_redirected_get_response_ptr->getVictimSyncsetRef();
        const Edgeset& edgeset = covered_placement_redirected_get_response_ptr->getEdgesetRef();

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        const EventList& background_event_list = covered_placement_redirected_get_response_ptr->getEventListRef();
        BandwidthUsage background_bandwidth_usage = covered_placement_redirected_get_response_ptr->getBandwidthUsageRef();
        uint32_t cross_edge_redirected_get_rsp_bandwidth_bytes = covered_placement_redirected_get_response_ptr->getMsgPayloadSize();
        background_bandwidth_usage.update(BandwidthUsage(0, cross_edge_redirected_get_rsp_bandwidth_bytes, 0));
        edge_beacon_server_background_counter_.updateBandwidthUsgae(background_bandwidth_usage);
        edge_beacon_server_background_counter_.addEvents(background_event_list);

        // TODO: END HERE

        return;
    }
}
#include "edge/beacon_server/covered_beacon_server.h"

#include <assert.h>
#include <sstream>

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
        assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // Differentiate CoveredBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredBeaconServer::~CoveredBeaconServer() {}

    // (1) Access content directory information

    void CoveredBeaconServer::processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_source_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
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
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupDirectoryTableByBeaconServer(tmp_key, edge_cache_server_worker_recvreq_source_addr, is_being_written, is_valid_directory_exist, directory_info);

        // Selective popularity aggregation
        const uint32_t source_edge_idx = covered_directory_lookup_request_ptr->getSourceIndex();
        const CollectedPopularity& collected_popularity = covered_directory_lookup_request_ptr->getCollectedPopularityRef();
        covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // Victim synchronization
        const VictimSyncset& victim_syncset = covered_directory_lookup_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    MessageBase* CoveredBeaconServer::getRspToLookupLocalDirectory_(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_lookup_response_ptr = new CoveredDirectoryLookupResponse(key, is_being_written, is_valid_directory_exist, directory_info, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
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
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, is_admit, directory_info, is_being_written);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        covered_cache_manager_ptr->updateVictimTrackerForSyncedVictimDirinfo(tmp_key, is_admit, directory_info);

        if (is_admit) // Admit a new key as local cached object
        {
            // Clear old local uncached popularity (TODO: preserved edge idx / bitmap) for the given key at soure edge node after admission
            covered_cache_manager_ptr->clearPopularityAggregatorAfterAdmission(tmp_key, source_edge_idx);
        }
        else // Evict a victim as local uncached object
        {
            // Selective popularity aggregation
            const CollectedPopularity& collected_popularity = covered_directory_update_request_ptr->getCollectedPopularityRef();
            covered_cache_manager_ptr->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, collected_popularity, is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node
        }

        // Victim synchronization
        const VictimSyncset& victim_syncset = covered_directory_update_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return is_being_written;
    }

    MessageBase* CoveredBeaconServer::getRspToUpdateLocalDirectory_(const Key& key, const bool& is_being_written, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        CoveredCacheManager* covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_update_response_ptr = new CoveredDirectoryUpdateResponse(key, is_being_written, victim_syncset, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(covered_directory_update_response_ptr != NULL);

        return covered_directory_update_response_ptr;
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
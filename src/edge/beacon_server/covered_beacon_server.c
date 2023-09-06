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

        // Lookup cooperation wrapper to get valid directory information if any
        // Also check whether the key is cached by a local/neighbor edge node (even if invalid temporarily)
        bool is_global_cached = edge_wrapper_ptr_->getCooperationWrapperPtr()->lookupLocalDirectoryByBeaconServer(tmp_key, edge_cache_server_worker_recvreq_source_addr, is_being_written, is_valid_directory_exist, directory_info);

        // Selective popularity aggregation
        const uint32_t source_edge_idx = covered_directory_lookup_request_ptr->getSourceIndex();
        const Popularity local_uncached_popularity = covered_directory_lookup_request_ptr->getLocalUncachedPopularity();
        const bool is_tracked_by_source_edge_node = covered_directory_lookup_request_ptr->isTracked(); // If key is tracked by local uncached metadata in the source edge node (i.e., if local uncached popularity is valid)
        edge_wrapper_ptr_->getCoveredCacheManagerPtr()->updatePopularityAggregatorForAggregatedPopularity(tmp_key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, is_global_cached); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        // Victim synchronization
        const VictimSyncset& victim_syncset = covered_directory_lookup_request_ptr->getVictimSyncsetRef();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
        edge_wrapper_ptr_->getCoveredCacheManagerPtr()->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);

        return;
    }

    MessageBase* CoveredBeaconServer::getRspToLookupLocalDirectory_(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = edge_wrapper_ptr_->getCoveredCacheManagerPtr()->accessVictimTrackerForVictimSyncset();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        MessageBase* covered_directory_lookup_response_ptr = new CoveredDirectoryLookupResponse(key, victim_syncset, is_being_written, is_valid_directory_exist, directory_info, edge_idx, edge_beacon_server_recvreq_source_addr_, event_list, skip_propagation_latency);
        assert(covered_directory_lookup_response_ptr != NULL);

        return covered_directory_lookup_response_ptr;
    }

    bool CoveredBeaconServer::processReqToUpdateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        // Update local directory information in cooperation wrapper
        bool is_being_written = false;
        is_being_written = edge_wrapper_ptr_->getCooperationWrapperPtr()->updateLocalDirectory(key, is_admit, directory_info);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        edge_wrapper_ptr_->getCoveredCacheManagerPtr()->updateVictimTrackerForSyncedVictimDirinfo(key, is_admit, directory_info);

        return is_being_written;
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
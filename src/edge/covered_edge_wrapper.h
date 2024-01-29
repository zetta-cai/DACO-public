/*
 * CoveredEdgeWrapper: a COVERED edge node launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper.
 *
 * NOTE: all non-const shared variables in EdgeWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2024.01.29).
 */

#ifndef COVERED_EDGE_WRAPPER_H
#define COVERED_EDGE_WRAPPER_H

#include <string>

namespace covered
{
    // Forward declaration
    class CoveredEdgeWrapper;
}

#include "edge/edge_wrapper_base.h"

namespace covered
{
    class CoveredEdgeWrapper : public EdgeWrapperBase
    {
    public:
        CoveredEdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt);
        virtual ~CoveredEdgeWrapper();

        // (1) Const getters

        virtual uint32_t getTopkEdgecntForPlacement() const override;
        virtual CoveredCacheManager* getCoveredCacheManagerPtr() const override;
        virtual BackgroundCounter& getEdgeBackgroundCounterForBeaconServerRef() override;
        virtual WeightTuner& getWeightTunerRef() override;

        // (3) Invalidate and unblock for MSI protocol

        // NOTE: NO need to add events of issue_invalidation_req, as they happen in parallel and have been counted in the event of invalidate_cache_copies
        virtual MessageBase* getInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const override;
        virtual void processInvalidationResponse_(MessageBase* invalidation_response_ptr) const override;
        
        // NOTE: NO need to add events of issue_finish_block_req, as they happen in parallel and have been counted in the event of finish_block
        virtual MessageBase* getFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const override;
        virtual void processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const override;

        // (6) Common utility functions (invoked by edge cache server worker/placement-processor or edge beacon server of closest/beacon edge node)

        // (6.1) For local edge cache access
        virtual bool getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value) const = 0; // Return is local cached and valid

        // (6.2) For local directory admission
        virtual void admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const bool& skip_propagation_latency) const = 0; // Admit directory info in current edge node (is_neighbor_cached indicates if key is cached by any other edge node except the current edge node after admiting local dirinfo; invoked by cache server worker or beacon server for local placement notification if sender is or not beacon)

        // (7) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const override;
    private:
        // NOTE: the followings are covered-specific utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

        // (7.1) For victim synchronization
        void updateCacheManagerForLocalSyncedVictimsInternal_(const bool& affect_victim_tracker) const; // NOTE: both edge cache server worker and local/remote beacon node (non-blocking data fetching for placement deployment) will access local edge cache, which may affect local cached metadata and hence update local synced victims (yet always update cache margin bytes)
        void updateCacheManagerForNeighborVictimSyncsetInternal_(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset) const;

        // (7.2) For non-blocking placement deployment (ONLY invoked by beacon edge node)
        // NOTE: (for non-blocking placement deployment) source_addr and recvrsp_socket_server_ptr are used for receiving eviction directory update responses if with local placement notification in current beacon edge node; skip propagation latency is used for all messages during non-blocking placement deployment (intermediate bandwidth usage and event list are counted by edge_background_counter_for_beacon_server_)
        // NOTE: sender_is_beacon indicates whether sender is cache server worker in beacon edge node to trigger local placement calculation, or sender is beacon server in beacon edge node to trigger placement calculation for remote requests; need_hybrid_fetching MUST be true under sender_is_beacon = true if local edge cache misses for local data fetching
        //bool nonblockDataFetchForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const; // (OBSELETE for non-blocking placement deployment) Fetch data from local cache or neighbor to trigger non-blocking placement notification; need_hybrid_fetching indicates if we need hybrid fetching (i.e., resort sender to fetch data from cloud); return if edge is finished
        void nonblockDataFetchForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const; // Fetch data from local cache or neighbor to trigger non-blocking placement notification; need_hybrid_fetching indicates if we need hybrid fetching (i.e., resort sender to fetch data from cloud)
        void nonblockDataFetchFromCloudForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const; // Fetch data from cloud without hybrid data fetching (a corner case) (ONLY invoked by edge beacon server instead of cache server of the beacon edge node)
        //bool nonblockNotifyForPlacementInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency) const; // (OBSELETE for non-blocking placement deployment) Notify all edges in best_placement_edgeset to admit key-value pair into their local edge cache; return if edge is finished
        void nonblockNotifyForPlacementInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const; // Notify all edges in best_placement_edgeset to admit key-value pair into their local edge cache

        // (7.3) For beacon-based cached metadata update (non-blocking notification-based)
        void processMetadataUpdateRequirementInternal_(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const bool& skip_propagation_latency) const;

        // (7.4) Reward calculation for local reward and cache placement (delta reward of admission benefit / eviction cost)
        Reward calcLocalCachedRewardInternal_(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies) const; // For reward of local cached objects or eviction cost of victims
        Reward calcLocalUncachedRewardInternal_(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity = 0.0) const; // For reward for local uncached objects or admission benefit of candidates (NOTE: redirected_uncached_popularity refers to local_uncached_popularity of edge nodes not in placement edgeset, which MUST be zero for local reward of local uncached objects)

        // (7.5) Helper functions after local/remote directory lookup/admission/eviction and writelock acquire/release
        // NOTE: for local beacon, locally perform sender-part and invoke helper functions; for remote beacon, get sender-part results from requests and invoke helper functions
        bool afterDirectoryLookupHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
        void afterDirectoryAdmissionHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const;
        bool afterDirectoryEvictionHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const; // Return if edge is finished
        bool afterWritelockAcquireHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
        bool afterWritelockReleaseHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
    private:
        static const std::string kClassName;

        // (5) Other utilities
 
        virtual void checkPointers_() const;

        std::string instance_name_;

        // Const shared variables
        const uint32_t topk_edgecnt_for_placement_; // Come from CLI for non-blocking placement deployment
        NetworkAddr edge_beacon_server_recvreq_source_addr_for_placement_; // Calculated from edgeidx and edgecnt for non-blocking placement deployment
        NetworkAddr corresponding_cloud_recvreq_dst_addr_for_placement_; // For non-blocking placement deployment

        // ONLY used by COVERED
        // (i) COVERED uses cache manager for popularity aggregation of global admission, victim tracking for placement calculation, and directory metadata cache
        // (ii) COVERED uses beacon background counter to track background events and bandwidth usage for non-blocking placement deployment (NOTE: NOT count events for non-blocking data fetching from local edge cache and non-blocking metadata releasing, due to limited computation overhead and NO bandwidth usage)
        // (iii) COVERED uses weight tuner to track weight info to calculate reward (for local reward calculation and trade-off-aware placement calculation) with online parameter tuning
        CoveredCacheManager* covered_cache_manager_ptr_; // CoveredCacheManager for cooperative-caching-aware cache management (thread safe)
        mutable BackgroundCounter edge_background_counter_for_beacon_server_; // Update and load by beacon server (thread safe)
        WeightTuner weight_tuner_; // Update and load by cache server and beacon server (thread safe)
    };
}

#endif
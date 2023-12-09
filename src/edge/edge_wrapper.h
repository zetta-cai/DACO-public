/*
 * EdgeWrapper: an edge node launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper (and CoveredCacheManager if cache name is COVERED).
 *
 * NOTE: all non-const shared variables in EdgeWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <string>

namespace covered
{
    // Forward declaration
    class EdgeWrapper;
}

#include "cache/cache_wrapper.h"
#include "cli/edge_cli.h"
#include "common/node_wrapper_base.h"
#include "core/covered_cache_manager.h"
#include "core/popularity/edgeset.h"
#include "core/victim/victim_syncset.h"
#include "concurrency/ring_buffer_impl.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "cooperation/directory/dirinfo_set.h"
#include "edge/cache_server/cache_server.h"
#include "edge/utils/background_counter.h"
#include "edge/utils/local_cache_admission_item.h"
#include "edge/utils/weight_tuner.h"
#include "event/event_list.h"
#include "network/propagation_simulator.h"

namespace covered
{
    class EdgeWrapperParam
    {
    public:
        EdgeWrapperParam();
        EdgeWrapperParam(const uint32_t& edge_idx, EdgeCLI* edge_cli_ptr);
        ~EdgeWrapperParam();

        uint32_t getEdgeIdx() const;
        EdgeCLI* getEdgeCLIPtr() const;

        EdgeWrapperParam& operator=(const EdgeWrapperParam& other);
    private:
        uint32_t edge_idx_;
        EdgeCLI* edge_cli_ptr_;
    };
    
    class EdgeWrapper : public NodeWrapperBase
    {
    public:
        static void* launchEdge(void* edge_wrapper_param_ptr);

        EdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt);
        virtual ~EdgeWrapper();

        // (1) Const getters

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        uint32_t getPercacheserverWorkercnt() const;
        uint32_t getPropagationLatencyCrossedgeUs() const;
        uint32_t getPropagationLatencyEdgecloudUs() const;
        uint32_t getTopkEdgecntForPlacement() const;
        CacheWrapper* getEdgeCachePtr() const;
        CooperationWrapperBase* getCooperationWrapperPtr() const;
        PropagationSimulatorParam* getEdgeToclientPropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeToedgePropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeTocloudPropagationSimulatorParamPtr() const;
        CoveredCacheManager* getCoveredCacheManagerPtr() const;
        BackgroundCounter& getEdgeBackgroundCounterForBeaconServerRef();
        WeightTuner& getWeightTunerRef();
        RingBuffer<LocalCacheAdmissionItem>* getLocalCacheAdmissionBufferPtr() const;

        // (2) Utility functions

        uint64_t getSizeForCapacity() const;

        bool currentIsBeacon(const Key& key) const; // Check if current is beacon node
        bool currentIsTarget(const DirectoryInfo& directory_info) const; // Check if current is target node

        // For request redirection (triggered by client-issued local requests and COVERED's non-blocking placement deployment)
        NetworkAddr getBeaconDstaddr_(const Key& key) const; // Get destination address of beacon server recvreq in beacon edge node
        NetworkAddr getTargetDstaddr(const DirectoryInfo& directory_info) const; // Get destination address of cache server recvreq in target edge node

        // (3) Invalidate and unblock for MSI protocol

        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Invalidate all cache copies for the key simultaneously (note that invalidating closest edge node is okay, as it is waiting for AcquireWritelockResponse instead of processing cache access requests)
        bool parallelInvalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // For each edge node idx in directory entry

        // NOTE: NO need to add events of issue_invalidation_req, as they happen in parallel and have been counted in the event of invalidate_cache_copies
        void sendInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const NetworkAddr& edge_invalidation_server_recvreq_dst_addr, const bool& skip_propagation_latency) const;

        void processInvalidationResponse_(MessageBase* invalidation_response_ptr) const;
        
        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Notify all blocked edges for the key simultaneously
        bool parallelNotifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // For each edge node network addr in block list

        // NOTE: NO need to add events of issue_finish_block_req, as they happen in parallel and have been counted in the event of finish_block
        void sendFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, const bool& skip_propagation_latency) const;

        void processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const;

        // (6) common utility functions (invoked by edge cache server worker/placement-processor or edge beacon server of closest/beacon edge node)

        // (6.1) For local edge cache access
        bool getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value) const; // Return is local cached and valid

        // (6.2) For local directory admission
        void admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const bool& skip_propagation_latency) const; // Admit directory info in current edge node (is_neighbor_cached indicates if key is cached by any other edge node except the current edge node after admiting local dirinfo; invoked by cache server worker or beacon server for local placement notification if sender is or not beacon)

        // (7) covered-specific utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

        // (7.1) For victim synchronization
        uint64_t getCacheMarginBytes() const;
        void updateCacheManagerForLocalSyncedVictims(const bool& affect_victim_tracker) const; // NOTE: both edge cache server worker and local/remote beacon node (non-blocking data fetching for placement deployment) will access local edge cache, which may affect local cached metadata and hence update local synced victims (yet always update cache margin bytes)

        // (7.2) For non-blocking placement deployment (ONLY invoked by beacon edge node)
        // NOTE: (for non-blocking placement deployment) source_addr and recvrsp_socket_server_ptr are used for receiving eviction directory update responses if with local placement notification in current beacon edge node; skip propagation latency is used for all messages during non-blocking placement deployment (intermediate bandwidth usage and event list are counted by edge_background_counter_for_beacon_server_)
        // NOTE: sender_is_beacon indicates whether sender is cache server worker in beacon edge node to trigger local placement calculation, or sender is beacon server in beacon edge node to trigger placement calculation for remote requests; need_hybrid_fetching MUST be true under sender_is_beacon = true if local edge cache misses for local data fetching
        //bool nonblockDataFetchForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const; // (OBSELETE for non-blocking placement deployment) Fetch data from local cache or neighbor to trigger non-blocking placement notification; need_hybrid_fetching indicates if we need hybrid fetching (i.e., resort sender to fetch data from cloud); return if edge is finished
        void nonblockDataFetchForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const; // Fetch data from local cache or neighbor to trigger non-blocking placement notification; need_hybrid_fetching indicates if we need hybrid fetching (i.e., resort sender to fetch data from cloud)
        void nonblockDataFetchFromCloudForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const; // Fetch data from cloud without hybrid data fetching (a corner case) (ONLY invoked by edge beacon server instead of cache server of the beacon edge node)
        //bool nonblockNotifyForPlacement(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency) const; // (OBSELETE for non-blocking placement deployment) Notify all edges in best_placement_edgeset to admit key-value pair into their local edge cache; return if edge is finished
        void nonblockNotifyForPlacement(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const; // Notify all edges in best_placement_edgeset to admit key-value pair into their local edge cache

        // (7.3) For beacon-based cached metadata update (non-blocking notification-based)
        void processMetadataUpdateRequirement(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const bool& skip_propagation_latency) const;

        // (7.4) Reward calculation for local reward and cache placement (delta reward of admission benefit / eviction cost)
        Reward calcLocalCachedReward(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies) const; // For reward of local cached objects or eviction cost of victims
        Reward calcLocalUncachedReward(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity = 0.0) const; // For reward for local uncached objects or admission benefit of candidates (NOTE: redirected_uncached_popularity refers to local_uncached_popularity of edge nodes not in placement edgeset, which MUST be zero for local reward of local uncached objects)

        // (7.5) Helper functions after local/remote directory lookup/admission/eviction and writelock acquire/release
        // NOTE: for local beacon, locally perform sender-part and invoke helper functions; for remote beacon, get sender-part results from requests and invoke helper functions
        bool afterDirectoryLookupHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
        void afterDirectoryAdmissionHelper_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const;
        bool afterDirectoryEvictionHelper_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const; // Return if edge is finished
        bool afterWritelockAcquireHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
        bool afterWritelockReleaseHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Return if edge is finished
    private:
        static const std::string kClassName;

        // (4) Benchmark process

        virtual void initialize_() override;
        virtual void processFinishrunRequest_() override;
        virtual void processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr) override;
        virtual void cleanup_() override;

        // (5) Other utilities

        static void* launchBeaconServer_(void* edge_wrapper_ptr);
        static void* launchCacheServer_(void* edge_wrapper_ptr);
        static void* launchInvalidationServer_(void* edge_wrapper_ptr);

        void checkPointers_() const;

        std::string instance_name_;

        // Const shared variables
        const std::string cache_name_; // Come from CLI
        const uint64_t capacity_bytes_; // Come from CLI
        const uint32_t percacheserver_workercnt_; // Come from CLI
        const uint32_t propagation_latency_crossedge_us_; // Come from CLI
        const uint32_t propagation_latency_edgecloud_us_; // Come from CLI
        const uint32_t topk_edgecnt_for_placement_; // Come from CLI for non-blocking placement deployment
        NetworkAddr edge_beacon_server_recvreq_source_addr_for_placement_; // Calculated from edgeidx and edgecnt for non-blocking placement deployment
        NetworkAddr corresponding_cloud_recvreq_dst_addr_for_placement_; // For non-blocking placement deployment

        // NOTE: we do NOT need per-key rwlock for atomicity among CacheWrapper, CooperationWrapperBase, and CoveredCacheMananger.
        // (1) CacheWrapper is already thread-safe for cache server and invalidation server, CooperationWrapperBase is already thread-safe for cache server and beacon server, and CoveredCacheMananger is already thread-safe for cache server and beacon server -> NO dead locking as each thread-safe structure releases its own lock after each function.
        // (2) Cache server needs to access CacheWrapper, CooperationWrapperBase, and CoveredCacheManager, while there NOT exist any serializability/atomicity issue for requests of the same key, as cache server workers have already partitioned requests by hashing keys into ring buffer.
        // (2-1) Note that CacheWrapper and CooperationWrapperBase do NOT have any serializability issue, as the former tracks data and cache metadata, while the latter tracks directory metadata.
        // (3) Beacon server needs to access CacheWrapper, CooperationWrapperBase, and CoveredCacheManager, while we do NOT need strong consistency for aggregated popularity or synchronized victims in CoveredCacheManager, and hence NO need to keep serializability/atomicity.
        // (3-1) Note that CoveredCacheManager in nature has serializaiblity issues with CacheWrapper and CooperationWrapperBase due to aggregated popularity + victim cacheinfo and dirinfo, yet NO need for strong consistency.
        // (4) Invalidation server needs to access CacheWrapper and CooperationWrapperBase, while it ONLY invalidates CacheWrapper (NOT affect directory information) and reads CooperationWrapperBase, and hence NO serializability/atomicity issue.

        // Non-const shared variables (thread safe)
        CacheWrapper* edge_cache_ptr_; // Data and metadata for local edge cache (thread safe)
        CooperationWrapperBase* cooperation_wrapper_ptr_; // Cooperation metadata (thread safe)
        PropagationSimulatorParam* edge_toclient_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_toedge_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_tocloud_propagation_simulator_param_ptr_; // thread safe

        // ONLY for COVERED
        // (i) COVERED uses cache manager for popularity aggregation of global admission, victim tracking for placement calculation, and directory metadata cache
        // (ii) COVERED uses beacon background counter to track background events and bandwidth usage for non-blocking placement deployment (NOTE: NOT count events for non-blocking data fetching from local edge cache and non-blocking metadata releasing, due to limited computation overhead and NO bandwidth usage)
        // (iii) COVERED uses weight tuner to track weight info to calculate reward (for local reward calculation and trade-off-aware placement calculation) with online parameter tuning
        CoveredCacheManager* covered_cache_manager_ptr_; // CoveredCacheManager for cooperative-caching-aware cache management (thread safe)
        mutable BackgroundCounter edge_background_counter_for_beacon_server_; // Update and load by beacon server (thread safe)
        WeightTuner weight_tuner_; // Update and load by cache server and beacon server (thread safe)

        // Sub-threads
        pthread_t edge_toclient_propagation_simulator_thread_;
        pthread_t edge_toedge_propagation_simulator_thread_;
        pthread_t edge_tocloud_propagation_simulator_thread_;
        pthread_t beacon_server_thread_;
        pthread_t cache_server_thread_;
        pthread_t invalidation_server_thread_;

        // Common data structure shared by edge cache server and edge beacon server
        // -> Pushed by cache server worker (for in-advance remote placement notification after hybrid data fetching and local placement notification if sender is beacon) and beacon server (for local placement notification if sender is NOT beacon)
        // -> Popped by cache server placement processor
        // NOTE: we CANNOT expose CacheServer* in EdgeWrapper, as CacheServer and BeaconServer have shorter life span than EdgeWrapper -> if we expose CacheServer* in EdgeWrapper, BeaconServer may still access CacheServer, which has already been released by cache server thread yet
        RingBuffer<LocalCacheAdmissionItem>* local_cache_admission_buffer_ptr_; // thread safe (local cached admission + eviction)
    };
}

#endif
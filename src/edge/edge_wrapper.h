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

#include "cache/cache_wrapper.h"
#include "cli/edge_cli.h"
#include "common/node_wrapper_base.h"
#include "core/covered_cache_manager.h"
#include "core/popularity/edgeset.h"
#include "core/victim/victim_syncset.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "edge/background_counter.h"
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

        EdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt);
        virtual ~EdgeWrapper();

        // (1) Const getters

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        uint32_t getPercacheserverWorkercnt() const;
        uint32_t getTopkEdgecntForPlacement() const;
        CacheWrapper* getEdgeCachePtr() const;
        CooperationWrapperBase* getCooperationWrapperPtr() const;
        CoveredCacheManager* getCoveredCacheManagerPtr() const;
        PropagationSimulatorParam* getEdgeToclientPropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeToedgePropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeTocloudPropagationSimulatorParamPtr() const;
        BackgroundCounter& getEdgeBackgroundCounterForBeaconServerRef();
        Rwlock* getRwlockForEvictionPtr() const;

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
        bool invalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const;
        
        // Return if edge node is finished (invoked by cache server worker or beacon server)
        bool notifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Notify all blocked edges for the key simultaneously

        // (6) common utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

        // (6.1) For local edge cache access
        bool getLocalEdgeCache_(const Key& key, Value& value) const; // Return is local cached and valid

        // (6.2) For local edge cache admission and directory admission
        void admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_valid) const;
        void admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written) const; // Admit directory info in current edge node

        // (6.3) For blocking-based cache eviction and local/remote directory eviction
        bool evictForCapacity_(const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background = false) const; // Including evict local edge cache and directory updates; return if edge is finished
        void evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const; // Evict local edge cache
        bool parallelEvictDirectory_(const std::unordered_map<Key, Value, KeyHasher>& total_victims, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const; // Perform directory updates for evicted victims in parallel; return if edge is finished
        void evictLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency, const bool& is_background) const; // Evict directory info from current edge node
        MessageBase* getReqToEvictBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const; // Send directory update request with is_admit = false to remove dirinfo of evicted victim
        void processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, const bool& is_background) const;

        // (7) covered-specific utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

        // (7.1) For victim synchronization
        void updateCacheManagerForLocalSyncedVictims() const; // NOTE: both edge cache server worker and local/remote beacon node (non-blocking data fetching for placement deployment) will access local edge cache, which affects local cached metadata and may trigger update for local synced victims
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset) const; // NOTE: all edge cache/beacon/invalidation servers will access cooperation wrapper to get content directory information for local beaconed victims from received victim syncset

        // (7.2) For non-blocking placement deployment (ONLY invoked by beacon edge node)
        bool nonblockDataFetchForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency, bool& need_hybrid_fetching) const; // Fetch data from local cache or neighbor to trigger non-blocking placement notification; need_hybrid_fetching indicates if we need hybrid fetching (i.e., resort sender to fetch data from cloud); return if edge is finished
        void nonblockDataFetchFromCloudForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const; // Fetch data from cloud without hybrid data fetching (a corner case) (ONLY invoked by edge beacon server instead of cache server of the beacon edge node)
        bool nonblockNotifyForPlacement(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency) const; // Notify all edges in best_placement_edgeset to admit key-value pair into their local edge cache; return if edge is finished
    private:
        static const std::string kClassName;

        // (3) Invalidate and unblock for MSI protocol

        // NOTE: NO need to add events of issue_invalidation_req, as they happen in parallel and have been counted in the event of invalidate_cache_copies
        void sendInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr edge_invalidation_server_recvreq_dst_addr, const bool& skip_propagation_latency) const;

        // NOTE: NO need to add events of issue_finish_block_req, as they happen in parallel and have been counted in the event of finish_block
        void sendFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, const bool& skip_propagation_latency) const;

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
        CoveredCacheManager* covered_cache_manager_ptr_; // CoveredCacheManager for cooperative-caching-aware cache management (thread safe)
        PropagationSimulatorParam* edge_toclient_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_toedge_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_tocloud_propagation_simulator_param_ptr_; // thread safe
        // COVERED uses beacon background counter to track background events and bandwidth usage for non-blocking placement deployment (NOTE: NOT count events for non-blocking data fetching from local edge cache and non-blocking metadata releasing, due to limited computation overhead and NO bandwidth usage)
        mutable BackgroundCounter edge_background_counter_for_beacon_server_; // Update and load by beacon server (thread safe)

        // Sub-threads
        pthread_t edge_toclient_propagation_simulator_thread_;
        pthread_t edge_toedge_propagation_simulator_thread_;
        pthread_t edge_tocloud_propagation_simulator_thread_;
        pthread_t beacon_server_thread_;
        pthread_t cache_server_thread_;
        pthread_t invalidation_server_thread_;

        mutable Rwlock* rwlock_for_eviction_ptr_; // Guarantee the atomicity of eviction among different edge cache server workers
    };
}

#endif
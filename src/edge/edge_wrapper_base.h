/*
 * EdgeWrapperBase: an edge node base launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper.
 *
 * NOTE: all non-const shared variables in EdgeWrapperBase should be thread safe.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_BASE_H
#define EDGE_WRAPPER_BASE_H

//#define DEBUG_EDGE_WRAPPER_BASE

#include <string>

namespace covered
{
    // Forward declaration
    class EdgeWrapperBase;
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
#include "edge/edge_custom_func_param_base.h"
#include "edge/cache_server/cache_server_base.h"
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
    
    class EdgeWrapperBase : public NodeWrapperBase
    {
    public:
        static void* launchEdge(void* edge_wrapper_param_ptr);

        EdgeWrapperBase(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt);
        virtual ~EdgeWrapperBase();

        // (1) Const getters

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        uint32_t getPercacheserverWorkercnt() const;
        uint32_t getPropagationLatencyCrossedgeUs() const;
        uint32_t getPropagationLatencyEdgecloudUs() const;
        CacheWrapper* getEdgeCachePtr() const;
        CooperationWrapperBase* getCooperationWrapperPtr() const;
        PropagationSimulatorParam* getEdgeToclientPropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeToedgePropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeTocloudPropagationSimulatorParamPtr() const;
        RingBuffer<LocalCacheAdmissionItem>* getLocalCacheAdmissionBufferPtr() const;

        virtual uint32_t getTopkEdgecntForPlacement() const = 0;
        virtual CoveredCacheManager* getCoveredCacheManagerPtr() const = 0;
        virtual BackgroundCounter& getEdgeBackgroundCounterForBeaconServerRef() = 0;
        virtual WeightTuner& getWeightTunerRef() = 0;

        // (2) Utility functions

        virtual uint64_t getSizeForCapacity() const;

        bool currentIsBeacon(const Key& key) const; // Check if current is beacon node
        bool currentIsTarget(const DirectoryInfo& directory_info) const; // Check if current is target node

        // For request redirection (triggered by client-issued local requests and COVERED's non-blocking placement deployment)
        NetworkAddr getBeaconDstaddr_(const Key& key) const; // Get destination address of beacon server recvreq in beacon edge node
        NetworkAddr getBeaconDstaddr_(const uint32_t& beacon_edge_idx) const; // Get destination address of beacon server recvreq in beacon edge node
        NetworkAddr getTargetDstaddr(const DirectoryInfo& directory_info) const; // Get destination address of cache server recvreq in target edge node

        // (3) Invalidate and unblock for MSI protocol

        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Invalidate all cache copies for the key simultaneously (note that invalidating closest edge node is okay, as it is waiting for AcquireWritelockResponse instead of processing cache access requests)
        bool parallelInvalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // For each edge node idx in directory entry

        // NOTE: NO need to add events of issue_invalidation_req, as they happen in parallel and have been counted in the event of invalidate_cache_copies
        virtual MessageBase* getInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const = 0;
        virtual void processInvalidationResponse_(MessageBase* invalidation_response_ptr) const = 0;
        
        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Notify all blocked edges for the key simultaneously
        bool parallelNotifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // For each edge node network addr in block list

        // NOTE: NO need to add events of issue_finish_block_req, as they happen in parallel and have been counted in the event of finish_block
        virtual MessageBase* getFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const bool& skip_propagation_latency) const = 0;
        virtual void processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const = 0;

        // (6) Common utility functions (invoked by edge cache server worker/placement-processor or edge beacon server of closest/beacon edge node)

        // (6.1) For local edge cache access
        virtual bool getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value) const = 0; // Return is local cached and valid

        // (6.2) For local directory admission
        virtual void admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const bool& skip_propagation_latency) const = 0; // Admit directory info in current edge node (is_neighbor_cached indicates if key is cached by any other edge node except the current edge node after admiting local dirinfo; invoked by cache server worker or beacon server for local placement notification if sender is or not beacon)

        // (6.3) For cache size usage
        uint64_t getCacheMarginBytes() const;

        // (7) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const = 0;
    private:
        static const std::string kClassName;

        // (4) Benchmark process

        virtual void initialize_() override;
        virtual void processFinishrunRequest_() override;
        virtual void processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr) override;
        virtual void cleanup_() override;

    protected:
        // (5) Other utilities
 
        virtual void checkPointers_() const;

        // Non-const shared variables (thread safe)
        CacheWrapper* edge_cache_ptr_; // Data and metadata for local edge cache (thread safe)
        CooperationWrapperBase* cooperation_wrapper_ptr_; // Cooperation metadata (thread safe)
        PropagationSimulatorParam* edge_toclient_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_toedge_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_tocloud_propagation_simulator_param_ptr_; // thread safe
    private:
        std::string base_instance_name_;

        // Const shared variables
        const std::string cache_name_; // Come from CLI
        const uint64_t capacity_bytes_; // Come from CLI
        const uint32_t percacheserver_workercnt_; // Come from CLI
        const uint32_t propagation_latency_crossedge_us_; // Come from CLI
        const uint32_t propagation_latency_edgecloud_us_; // Come from CLI

        // NOTE: we do NOT need per-key rwlock for atomicity among CacheWrapper, CooperationWrapperBase, and CoveredCacheMananger.
        // (1) CacheWrapper is already thread-safe for cache server, CooperationWrapperBase is already thread-safe for cache server and beacon server, and CoveredCacheMananger is already thread-safe for cache server and beacon server -> NO dead locking as each thread-safe structure releases its own lock after each function.
        // (2) Cache server needs to access CacheWrapper, CooperationWrapperBase, and CoveredCacheManager, while there NOT exist any serializability/atomicity issue for requests of the same key, as cache server workers have already partitioned requests by hashing keys into ring buffer.
        // (2-1) Note that CacheWrapper and CooperationWrapperBase do NOT have any serializability issue, as the former tracks data and cache metadata, while the latter tracks directory metadata.
        // (3) Beacon server needs to access CacheWrapper, CooperationWrapperBase, and CoveredCacheManager, while we do NOT need strong consistency for aggregated popularity or synchronized victims in CoveredCacheManager, and hence NO need to keep serializability/atomicity.
        // (3-1) Note that CoveredCacheManager in nature has serializaiblity issues with CacheWrapper and CooperationWrapperBase due to aggregated popularity + victim cacheinfo and dirinfo, yet NO need for strong consistency.

        // Sub-threads
        pthread_t edge_toclient_propagation_simulator_thread_;
        pthread_t edge_toedge_propagation_simulator_thread_;
        pthread_t edge_tocloud_propagation_simulator_thread_;
        pthread_t beacon_server_thread_;
        pthread_t cache_server_thread_;

        // Common data structure shared by edge cache server and edge beacon server
        // -> Pushed by cache server worker (for in-advance remote placement notification after hybrid data fetching and local placement notification if sender is beacon) and beacon server (for local placement notification if sender is NOT beacon)
        // -> Popped by cache server placement processor
        // NOTE: we CANNOT expose CacheServerBase* in EdgeWrapper, as CacheServer and BeaconServer have shorter life span than EdgeWrapper -> if we expose CacheServerBase* in EdgeWrapper, BeaconServer may still access CacheServer, which has already been released by cache server thread yet
        RingBuffer<LocalCacheAdmissionItem>* local_cache_admission_buffer_ptr_; // thread safe (local cached admission + eviction)
    };
}

#endif
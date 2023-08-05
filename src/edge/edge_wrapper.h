/*
 * EdgeWrapper: an edge node launches BeaconServer, CacheServer, and InvalidationServer to process data/control requests based on corresponding CacheWrapper and CooperationWrapper.
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
#include "cooperation/cooperation_wrapper_base.h"
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

        EdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& percacheserver_workercnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us);
        virtual ~EdgeWrapper();

        // (1) Const getters

        std::string getCacheName() const;
        uint64_t getCapacityBytes() const;
        uint32_t getPercacheserverWorkercnt() const;
        CacheWrapper* getEdgeCachePtr() const;
        CooperationWrapperBase* getCooperationWrapperPtr() const;
        PropagationSimulatorParam* getEdgeToclientPropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeToedgePropagationSimulatorParamPtr() const;
        PropagationSimulatorParam* getEdgeTocloudPropagationSimulatorParamPtr() const;

        // (2) Utility functions

        uint64_t getSizeForCapacity() const;

        bool currentIsBeacon(const Key& key) const; // Check if current is beacon node
        bool currentIsTarget(const DirectoryInfo& directory_info) const; // Check if current is target node

        // (3) Invalidate and unblock for MSI protocol

        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Invalidate all cache copies for the key simultaneously (note that invalidating closest edge node is okay, as it is waiting for AcquireWritelockResponse instead of processing cache access requests)
        bool invalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, EventList& event_list, const bool& skip_propagation_latency) const;
        
        // Return if edge node is finished (invoked by cache server worker or beacon server)
        bool notifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, EventList& event_list, const bool& skip_propagation_latency) const; // Notify all blocked edges for the key simultaneously
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

        // NOTE: we do NOT need per-key rwlock for atomicity among CacheWrapper and CooperationWrapperBase.
        // (i) CacheWrapper is already thread-safe for cache server and invalidation server, and CooperationWrapperBase is already thread-safe for cache server and beacon server.
        // (2) Only cache server needs to access both CacheWrapper and CooperationWrapperBase, while there NOT exist any serializability/atomicity issue for requests of the same key, as cache server workers have already partitioned requests by hashing keys into ring buffer.

        // Non-const shared variables (thread safe)
        CacheWrapper* edge_cache_ptr_; // Data and metadata for local edge cache (thread safe)
        CooperationWrapperBase* cooperation_wrapper_ptr_; // Cooperation metadata (thread safe)
        PropagationSimulatorParam* edge_toclient_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_toedge_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_tocloud_propagation_simulator_param_ptr_; // thread safe

        // Sub-threads
        pthread_t edge_toclient_propagation_simulator_thread_;
        pthread_t edge_toedge_propagation_simulator_thread_;
        pthread_t edge_tocloud_propagation_simulator_thread_;
        pthread_t beacon_server_thread_;
        pthread_t cache_server_thread_;
        pthread_t invalidation_server_thread_;
    };
}

#endif
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
#include "cooperation/cooperation_wrapper_base.h"
#include "edge/edge_param.h"
#include "network/propagation_simulator.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class EdgeWrapper
    {
    public:
        static void* launchEdge(void* edge_param_ptr);

        EdgeWrapper(const std::string& cache_name, const uint32_t& capacity_bytes, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& percacheserver_workercnt, const uint32_t& propagation_latency_clientedge, const uint32_t& propagation_latency_crossedge, const uint32_t& propagation_latency_edgecloud, EdgeParam* edge_param_ptr);
        virtual ~EdgeWrapper();

        void start();

        friend class CacheServer;
        friend class CacheServerWorkerBase;
        friend class BasicCacheServerWorker;
        friend class CoveredCacheServerWorker;
        friend class BeaconServerBase;
        friend class BasicBeaconServer;
        friend class CoveredBeaconServer;
        friend class InvalidationServerBase;
        friend class BasicInvalidationServer;
        friend class CoveredInvalidationServer;
    private:
        static const std::string kClassName;

        static void* launchBeaconServer_(void* edge_wrapper_ptr);
        static void* launchCacheServer_(void* edge_wrapper_ptr);
        static void* launchInvalidationServer_(void* edge_wrapper_ptr);

        uint32_t getSizeForCapacity_() const;

        // (1) Utility functions

        bool currentIsBeacon_(const Key& key) const; // Check if current is beacon node
        bool currentIsTarget_(const DirectoryInfo& directory_info) const; // Check if current is target node

        // (2) Invalidate for MSI protocol

        // Return if edge node is finished (invoked by cache server worker or beacon server)
        // Invalidate all cache copies for the key simultaneously (note that invalidating closest edge node is okay, as it is waiting for AcquireWritelockResponse instead of processing cache access requests)
        bool invalidateCacheCopies_(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo) const;
        void sendInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr edge_invalidation_server_recvreq_dst_addr) const;

        // (3) Unblock for MSI protocol
        
        // Return if edge node is finished (invoked by cache server worker or beacon server)
        bool notifyEdgesToFinishBlock_(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges) const; // Notify all blocked edges for the key simultaneously
        void sendFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr) const;

        // (4) Other utilities

        void checkPointers_() const;

        std::string instance_name_;

        // Const shared variables
        const std::string cache_name_; // Come from Param
        const uint32_t capacity_bytes_; // Come from Param
        const uint32_t edgecnt_; // Come from Param
        const uint32_t percacheserver_workercnt_; // Come from Param
        const EdgeParam* edge_param_ptr_; // Thread safe

        // NOTE: we do NOT need per-key rwlock for atomicity among CacheWrapper and CooperationWrapperBase.
        // (i) CacheWrapper is already thread-safe for cache server and invalidation server, and CooperationWrapperBase is already thread-safe for cache server and beacon server.
        // (2) Only cache server needs to access both CacheWrapper and CooperationWrapperBase, while there NOT exist any serializability/atomicity issue for requests of the same key, as cache server workers have already partitioned requests by hashing keys into ring buffer.

        // Non-const shared variables (thread safe)
        CacheWrapper* edge_cache_ptr_; // Data and metadata for local edge cache (thread safe)
        CooperationWrapperBase* cooperation_wrapper_ptr_; // Cooperation metadata (thread safe)
        PropagationSimulatorParam* edge_toclient_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_toedge_propagation_simulator_param_ptr_; // thread safe
        PropagationSimulatorParam* edge_tocloud_propagation_simulator_param_ptr_; // thread safe
    };
}

#endif
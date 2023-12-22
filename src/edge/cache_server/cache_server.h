/*
 * CacheServer: listen to receive local/redirected requests issued by clients/neighbors; launch multiple cache server worker threads to process received requests in parallel.
 *
 * NOTE: we place cache and directory admission/eviction in CacheServer as they can be invoked by both cache server worker and cache server placement processor.
 * (i) Admission can be foreground in cache server worker for baselines (independent admission) and COVERED (only-/including-sender hybrid data fetching = in-advance remote placement notification); and also background in cache server placement processor for COVERED (remote placement notification).
 * (ii) Eviction can be foreground in cache server worker for baselines (independent admission) and COVERED (invalid/valid value update); and also background in cache server placement processor for COVERED (local/remote placement notification).
 * 
 * NOTE: we place admitLocalDirectory_() in EdgeWrapper, as it can be invoked by both cache server worker (baselines for independent admission, or COVERED for local placement notification if sender is beacon) and beacon server (COVERED for local placement notification if sender is NOT beacon).
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_H
#define CACHE_SERVER_H

//#define DEBUG_CACHE_SERVER

#include <string>
#include <vector>

namespace covered
{
    class CacheServer;
}

#include "concurrency/rwlock.h"
#include "edge/cache_server/cache_server_processor_param.h"
#include "edge/cache_server/cache_server_worker_param.h"
#include "edge/edge_wrapper.h"
#include "hash/hash_wrapper_base.h"
#include "network/udp_msg_socket_server.h"

namespace covered
{
    class CacheServer
    {
    public:
        static void* launchCacheServer(void* edge_wrapper_ptr);

        CacheServer(EdgeWrapper* edge_wrapper_ptr);
        ~CacheServer();

        void start();

        EdgeWrapper* getEdgeWrapperPtr() const;
        NetworkAddr getEdgeCacheServerRecvreqSourceAddr() const;

        // (1) For local edge cache admission and remote directory admission (invoked by edge cache server worker for independent admission; or by placement processor for remote placement notification)
        void admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid) const;
        bool admitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background = false) const; // Admit directory info in remote beacon node; return if edge node is finished
        MessageBase* getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const;
        void processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_neighbor_cached, const bool& is_background) const;

        // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
        bool evictForCapacity_(const Key& key, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background = false) const; // Including evict local edge cache and directory updates; return if edge is finished
        void evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const; // Evict local edge cache
        // Perform directory updates for evicted victims in parallel; return if edge is finished
        bool parallelEvictDirectory_(const std::unordered_map<Key, Value, KeyHasher>& total_victims, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const; // For each evicted key in total victims
        bool evictLocalDirectory_(const Key& key, const Value& value, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const; // Evict directory info from current edge node (return if edge node is finished)
        MessageBase* getReqToEvictBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const; // Send directory update request with is_admit = false to remove dirinfo of evicted victim
        bool processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const;

        // (3) Trigger non-blocking placement notification (ONLY for COVERED)
        // NOTE: recvrsp_source_addr and recvrsp_socket_server_ptr refer to some edge cache server worker
        bool notifyBeaconForPlacementAfterHybridFetch_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Sender is NOT beacon; return if edge node is finished
    private:
        static const bool IS_HIGH_PRIORITY_FOR_METADATA_UPDATE;
        static const bool IS_HIGH_PRIORITY_FOR_VICTIM_FETCH;

        static const std::string kClassName;

        void receiveRequestsAndPartition_();
        void partitionRequest_(MessageBase* data_requeset_ptr);

        void checkPointers_() const;

        // Const shared variable
        std::string instance_name_;
        EdgeWrapper* edge_wrapper_ptr_;
        HashWrapperBase* hash_wrapper_ptr_;

        // Non-const individual variable
        std::vector<CacheServerWorkerParam> cache_server_worker_params_; // Each cache server thread has a unique param
        CacheServerProcessorParam* cache_server_metadata_update_processor_param_ptr_; // Only one cache server metadata update processor thread
        CacheServerProcessorParam* cache_server_victim_fetch_processor_param_ptr_; // Only one cache server victim fetch processor thread
        CacheServerProcessorParam* cache_server_redirection_processor_param_ptr_; // Only one cache server redirection processor thread
        CacheServerProcessorParam* cache_server_placement_processor_param_ptr_; // Only one cache server placement processor thread
        CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr_; // Only one cache server invalidation processor thread

        // For receiving local requests
        NetworkAddr edge_cache_server_recvreq_source_addr_; // The same as that used by clients or neighbors to send local/redirected requests (const shared variable)
        UdpMsgSocketServer* edge_cache_server_recvreq_socket_server_ptr_; // Used by cache server to receive local requests from clients and redirected requests from neighbors (non-const individual variable)

        mutable Rwlock* rwlock_for_eviction_ptr_; // Guarantee the atomicity of eviction among different edge cache server workers
    };
}

#endif
/*
 * CacheServerWorkerBase: process requests partitioned by cache server; access local edge cache, cooperative cache, and cloud to reply clients.
 *
 * A. Workflow of local get request:
 * (1) Cache server of closest edge node checks local edge cache.
 *   (1.1) If the object is cached and valid, cache server of closest edge node directly replies clients.
 *   (1.2) If the object is uncached or invalid, cache server of closest edge node tries to fetch data by cooperative caching.
 *     (1.2.1) Cache server of closest edge node first looks up directory information by DHT-based content discovery (beacon node may be closest edge node itself or beacon server of neighbor edge node).
 *     (1.2.2) If key is being written, cache server of closest edge node waits for writes.
 *       (1.2.2.1) If closest edge node itself is beacon, cache server of closest edge node waits by polling.
 *       (1.2.2.2) If beacon node is a remote neighbor, cache server of closest edge node waits by interruption, while beacon server of remote neighbor remembers closest edge node and notify it to finish blocking after writes.
 *     (1.2.3) If key is not being written and with valid directory information, cache server of closest edge node redirects get request to cache server of the target edge node to fetch data.
 *       (1.2.3.1) If cache server of the target edge node caches valid object, it replies cache server of the closest edge node.
 *       (1.2.3.2) If cache server of the target edge node caches invalid object, it notifies cache server of the closest edge node to repeat from step 1.2 again.
 *       (1.2.3.3) If cache server of the target edge node does not cache the object, it notifies cache server of the closest edge node to fetch data from cloud.
 *     (1.2.4) If key is not being written yet with invalid directory information, cache server of the closest edge node fetches data from cloud.
 * (2) Cache server of the closest edge node tries to update local edge cache if cached yet invalid.
 * (3) Cache server of the closest edge node tries to independently admit object in local edge cache if necessary.
 * 
 * B. Involved messages of local get requests:
 * (1) Receive local requests partitioned by cache server
 * (2) Issue/receive directory lookup requests/responses
 * (3) Receive/issue finish block requests
 * (4) Issue/receive redirected requests/responses
 * (5) Issue/receive global requests/responses
 * (6) Issue/receive directory update requests/responses
 * (7) Issue local responses
 * 
 * C. Workflow of local put/del request:
 * (1) Cache server of closest edge node tries to acquire a write lock from local MSI metadata or from beacon server of remote beacon node.
 *   (1.1) If lock result is kFailure, cache server of closest edge node waits for the write lock by polling or interruption.
 *   (1.2) If lock result is kNoneed or kSuccess, go to step 2.
 *     (1.2.1) Note that if lock result is kSuccess, cache server of closest edge node or beacon server of remote beacon node will invalidate all cache copies.
 * (2) Cache server of closest edge node writes cloud and update local edge cache if cached (i.e., write-through policy).
 * (3) Cache server of the closest edge node tries to independently admit object in local edge cache if necessary.
 * (4) If lock result is kSuccess, cache server of closest edge node notifies local MSI metadata or beacon server of remote beacon node to release write lock and finish writes.
 * 
 * D. Involved messages of local put/del requests:
 * (1) Receive local requests partitioned by cache server
 * (2) Issue/receive acquire writelock requests/responses
 * (3) Issue/receive invalidation requests/responses
 * (4) Receive/issue finish block requests
 * (5) Issue/receive global requests/responses
 * (6) Issue/receive finish write requests/responses
 * (7) Issue/receive directory update requests/responses
 * (8) Issue local response
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef CACHE_SERVER_WORKER_BASE_H
#define CACHE_SERVER_WORKER_BASE_H

//#define DEBUG_CACHE_SERVER

#include <string>

#include "edge/cache_server/cache_server_worker_param.h"
#include "event/event_list.h"
#include "message/message_base.h"

namespace covered
{
    class CacheServerWorkerBase
    {
    public:
        static void* launchCacheServerWorker(void* cache_server_worker_param_ptr);
    
        CacheServerWorkerBase(CacheServerWorkerParam* cache_server_worker_param_ptr);
        virtual ~CacheServerWorkerBase();

        void start();
    private:
        static const std::string kClassName;

        // Const variable
        std::string base_instance_name_;

        static CacheServerWorkerBase* getCacheServerWorkerByCacheName_(CacheServerWorkerParam* cache_server_worker_param_ptr);
    protected:
        bool processDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Return if edge node is finished

        // (1) Process read requests

        bool processLocalGetRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr) const; // Return if edge node is finished

        // (1.1) Access local edge cache

        virtual bool getLocalEdgeCache_(const Key& key, Value& value) const = 0; // Return is local cached and valid

        // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

        // Return if edge node is finished
        bool fetchDataFromNeighbor_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid, EventList& event_list, const bool& skip_propagation_latency) const;

        virtual void lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0;
        bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, EventList& event_list, const bool& skip_propagation_latency) const; // Check remote directory info
        virtual MessageBase* getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const = 0;
        virtual void processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0;

        virtual bool redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, EventList& event_list, const bool& skip_propagation_latency) const = 0; // Request redirection

        // (1.3) Access cloud

        // Return if edge node is finished
        bool fetchDataFromCloud_(const Key& key, Value& value, EventList& event_list, const bool& skip_propagation_latency) const;

        // (1.4) Update invalid cached objects in local edge cache

        virtual bool tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const = 0; // Return if key is local cached yet invalid

        // (2) Process write requests

        bool processLocalWriteRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr); // For put/del

        // (2.1) Acquire write lock and block for MSI protocol

        bool acquireWritelock_(const Key& key, LockResult& lock_result, EventList& event_list, const bool& skip_propagation_latency); // Return if edge node is finished
        virtual void acquireLocalWritelock_(const Key& key, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo) = 0;
        bool acquireBeaconWritelock_(const Key& key, LockResult& lock_result, EventList& event_list, const bool& skip_propagation_latency); // Return if edge node is finished
        virtual MessageBase* getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const = 0;
        virtual void processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const = 0;

        // Return if edge node is finished
        bool blockForWritesByInterruption_(const Key& key, EventList& event_list, const bool& skip_propagation_latency) const; // Block for MSI protocol

        // (2.2) Update cloud

        // Return if edge node is finished
        bool writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type, EventList& event_list, const bool& skip_propagation_latency);

        // (2.3) Update cached objects in local edge cache

        virtual bool updateLocalEdgeCache_(const Key& key, const Value& value) const = 0; // Return if key is cached after udpate
        virtual bool removeLocalEdgeCache_(const Key& key) const = 0; // Return if key is cached after removal

        // (2.4) Release write lock for MSI protocol

        // Return if edge node is finished
        bool releaseWritelock_(const Key& key, EventList& event_list, const bool& skip_propagation_latency);
        virtual void releaseLocalWritelock_(const Key& key, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges) = 0;
        bool releaseBeaconWritelock_(const Key& key, EventList& event_list, const bool& skip_propagation_latency); // Notify beacon node to finish writes
        virtual MessageBase* getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const = 0;
        virtual void processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr) const = 0;

        // (3) Process redirected requests

        bool processRedirectedRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr);
        bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const;

        // (4) Cache management

        // (4.1) Admit uncached objects in local edge cache

        // Return if edge node is finished (we will check capacity and trigger eviction for cache admission)
        bool tryToTriggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list, const bool& skip_propagation_latency); // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        bool admitObject_(const Key& key, const Value& value, EventList& event_list, const bool& skip_propagation_latency); // Including directory updates, admit local edge cache, and trigger eviction if necessary
        virtual void admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_valid) = 0;

        // (4.2) Evict cached objects from local edge cache

        bool evictForCapacity_(EventList& event_list, const bool& skip_propagation_latency) const; // Including evict local edge cache and directory updates
        virtual void evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) = 0;

        // (4.3) Update content directory information

        // Return if edge node is finished
        bool updateDirectory_(const Key& key, const bool& is_admit, bool& is_being_written, EventList& event_list, const bool& skip_propagation_latency) const; // Update content directory information
        virtual void updateLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const = 0; // Update directory info in current edge node
        bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, EventList& event_list, const bool& skip_propagation_latency) const; // Update directory info in remote beacon node
        virtual MessageBase* getReqToUpdateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const = 0;
        virtual void processRspToUpdateBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written) const = 0;

        // (5) Utility functions

        NetworkAddr getBeaconDstaddr_(const Key& key) const; // Get destination address of beacon server recvreq in beacon edge node
        NetworkAddr getTargetDstaddr_(const DirectoryInfo& directory_info) const; // Get destination address of cache server recvreq in target edge node

        void checkPointers_() const;

        // Member variables

        // Const variable
        const CacheServerWorkerParam* cache_server_worker_param_ptr_;

        // NOTE: we do NOT need per-key rwlock for serializability, which has been addressed by the ring buffer

        // NOTE: destination addresses for sending control requests come from beacon edge index, directory entry of all cache copies, and blocklist of all blocked edges
        // NOTE: destination address for sending redirected data requests come from directory info of target edge node
        // NOTE: destination address for sending finish block responses come from received finish block requests

        // For sending global data requests
        NetworkAddr corresponding_cloud_recvreq_dst_addr_;

        // For receiving control responses, redirected data responses, and global data responses
        NetworkAddr edge_cache_server_worker_recvrsp_source_addr_; // Used by beacon server to send back control responses, cache server to send back redirected data responses, and cloud to send back global data responses (const individual variable)
        UdpMsgSocketServer* edge_cache_server_worker_recvrsp_socket_server_ptr_; // Used by cache server worker to receive control responses from beacon server, redirected responses from cache server, and global responses from cloud (non-const individual variable)

        // For receiving finish block requests
        NetworkAddr edge_cache_server_worker_recvreq_source_addr_; // The same as that used by cache server worker or beacon server to send finish block requests (const individual variable)
        UdpMsgSocketServer* edge_cache_server_worker_recvreq_socket_server_ptr_; // Used by cache server worker to receive finish block requests from cache server worker or beacon server (non-const individual variable)
    };
}

#endif
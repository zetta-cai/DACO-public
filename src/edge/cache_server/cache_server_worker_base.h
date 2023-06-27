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

#include <string>

#include "edge/cache_server/cache_server_worker_param.h"
#include "lock/perkey_rwlock.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class CacheServerWorkerBase
    {
    public:
        static CacheServerWorkerBase* getCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr);

        CacheServerWorkerBase(CacheServerWorkerParam* cache_server_worker_param_ptr);
        virtual ~CacheServerWorkerBase();

        void start();
    private:
        static const std::string kClassName;

        // (1) Process data requests
    
        // Return if edge node is finished
        bool processDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& network_addr);
        bool processLocalGetRequest_(MessageBase* local_request_ptr, const NetworkAddr& network_addr) const;
        bool processLocalWriteRequest_(MessageBase* local_request_ptr, const NetworkAddr& network_addr); // For put/del
        bool processRedirectedRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& network_addr);
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& network_addr) const = 0;

        // (2) Access cooperative edge cache

        // (2.1) Fetch data from neighbor edge nodes

        // Return if edge node is finished
        bool fetchDataFromNeighbor_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) const;
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0; // Check remote directory info
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const = 0; // Request redirection

        // (2.2) Update content directory information

        // Return if edge node is finished
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const = 0; // Update remote directory info

        // (2.3) Process writes and block for MSI protocol

        // Return if edge node is finished
        bool acquireWritelock_(const Key& key, LockResult& lock_result);
        virtual bool acquireBeaconWritelock_(const Key& key, LockResult& lock_result) = 0;
        bool blockForWritesByInterruption_(const Key& key) const; // Block for MSI protocol
        bool releaseWritelock_(const Key& key);
        virtual bool releaseBeaconWritelock_(const Key& key) = 0; // Notify beacon node to finish writes

        // (2.4) Utility functions for cooperative caching

        void locateBeaconNode_(const Key& key) const; // Set remote address as beacon
        void locateTargetNode_(const DirectoryInfo& directory_info) const; // Set remote address as target

        // (3) Access cloud

        // Return if edge node is finished
        bool fetchDataFromCloud_(const Key& key, Value& value) const;
        bool writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type);

        // (4) Update cached objects in local edge cache

        // Return if edge node is finished
        bool tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const;
        // NOTE: we will check capacity and trigger eviction for value updates
        bool updateLocalEdgeCache_(const Key& key, const Value& value, bool& is_local_cached_after_udpate) const;
        void removeLocalEdgeCache_(const Key& key, bool& is_local_cached_after_udpate) const;

        // (5) Admit uncached objects in local edge cache

        // Return if edge node is finished
        bool tryToTriggerIndependentAdmission_(const Key& key, const Value& value) const;
        // NOTE: we will check capacity and trigger eviction for cache admission
        virtual bool triggerIndependentAdmission_(const Key& key, const Value& value) const = 0;

        // Member variables

        // Const variable
        std::string base_instance_name_;

        // Non-const individual variable
        UdpSocketWrapper* edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_;
    protected:
        // (2.2) Update content directory information

        // Return if edge node is finished
        bool updateDirectory_(const Key& key, const bool& is_admit, bool& is_being_written) const; // Update content directory information

        // (6) Utility functions

        void checkPointers_() const;

        // Member variables

        // Const variable
        const CacheServerWorkerParam* cache_server_worker_param_ptr_;

        // Guarantee the global serializability for writes of the same key
        mutable PerkeyRwlock* perkey_rwlock_for_serializability_ptr_;

        // Non-const individual variable
        UdpSocketWrapper* edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_;
        UdpSocketWrapper* edge_cache_server_worker_sendreq_totarget_socket_client_ptr_;
        UdpSocketWrapper* edge_cache_server_worker_sendrsp_tosource_socket_client_ptr_; // source could be client or neighbor edge node
    };
}

#endif
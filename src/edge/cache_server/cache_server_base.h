/*
 * CacheServerBase: listen to receive local requests issued by clients; access local edge cache, cooperative cache, and cloud to reply clients.
 *
 * Workflow of local get request:
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
 * Workflow of local put/del request: TODO.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef CACHE_SERVER_BASE
#define CACHE_SERVER_BASE

#include <string>

#include "edge/edge_wrapper.h"
#include "lock/perkey_rwlock.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class CacheServerBase
    {
    public:
        static CacheServerBase* getCacheServer(EdgeWrapper* edge_wrapper_ptr);

        CacheServerBase(EdgeWrapper* edge_wrapper_ptr);
        ~CacheServerBase();

        void start();
    private:
        static const std::string kClassName;

        // (1) Process data requests
    
        // Return if edge node is finished
        bool processDataRequest_(MessageBase* data_request_ptr);
        bool processLocalGetRequest_(MessageBase* local_request_ptr) const;
        bool processLocalWriteRequest_(MessageBase* local_request_ptr); // For put/del
        bool processRedirectedRequest_(MessageBase* redirected_request_ptr);
        virtual bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const = 0;

        // Return if edge node is finished
        bool fetchDataFromCloud_(const Key& key, Value& value) const;
        bool writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type);

        void tryToUpdateLocalEdgeCache_(const Key& key, const Value& value) const;
        void tryToTriggerIndependentAdmission_(const Key& key, const Value& value) const;
        virtual void triggerIndependentAdmission_(const Key& key, const Value& value) const = 0;

        // (2) Member variables

        const EdgeWrapper* edge_wrapper_ptr_;
        std::string base_instance_name_;

        // Guarantee the global serializability for writes of the same key
        mutable PerkeyRwlock* perkey_rwlock_for_serializability_ptr_;

        UdpSocketWrapper* edge_cache_server_recvreq_socket_server_ptr_;
        UdpSocketWrapper* edge_cache_server_sendreq_tocloud_socket_client_ptr_;
    };
}

#endif
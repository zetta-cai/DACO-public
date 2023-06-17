/*
 * EdgeWrapperBase: the base class of edge node to process data/control requests (thread safe).
 *
 * NOTE: all non-const shared variables in EdgeWrapperBase should be thread safe.
 * 
 * Workflow of local get request:
 * (1) Closest edge node checks local edge cache.
 *   (1.1) If the object is cached and valid, closest edge node directly replies clients.
 *   (1.2) If the object is uncached or invalid, closest edge node tries to fetch data by cooperative caching.
 *     (1.2.1) Closest edge node first looks up directory information by DHT-based content discovery (beacon node may be closest edge node itself or a neighbor edge node).
 *     (1.2.2) If key is being written, closest edge node waits for writes.
 *       (1.2.2.1) If closest edge node itself is beacon, closest waits by polling.
 *       (1.2.2.2) If beacon is a remote neighbor, closest waits by interruption, while beacon will remember the closest edge node and notify the closest edge node to finish blocking after writes.
 *     (1.2.3) If key is not being written and with valid directory information, closest edge node redirects get request to the target edge node to fetch data.
 *       (1.2.3.1) If the target edge node caches valid object, target replies the closest edge node.
 *       (1.2.3.2) If the target edge node caches invalid object, target notifies closest edge node to repeat from step 1.2 again.
 *       (1.2.3.3) If the target edge node does not cache the object, target notifies closest edge node to fetch data from cloud.
 *     (1.2.4) If key is not being written yet with invalid directory information, closest edge node fetches data from cloud.
 * (2) Closest edge node tries to update local edge cache if cached yet invalid.
 * (3) Closest edge node tries to independently admit object into local edge cache if necessary.
 * 
 * Workflow of local put/del request: TODO.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_BASE_H
#define EDGE_WRAPPER_BASE_H

#include <string>

#include "cache/cache_wrapper_base.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "edge/edge_param.h"
#include "lock/perkey_rwlock.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class EdgeWrapperBase
    {
    public:
        static EdgeWrapperBase* getEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);

        EdgeWrapperBase(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~EdgeWrapperBase();

        void start();
    private:
        static const std::string kClassName;

        // (1) Data requests
    
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

        // (2) Control requests

        // Return if edge node is finished
        bool processControlRequest_(MessageBase* control_request_ptr);
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr) const = 0;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) = 0;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) = 0;

        std::string base_instance_name_;
    protected:
        // Const shared variables
        const std::string cache_name_;
        const EdgeParam* edge_param_ptr_;

        // Guarantee the global serializability for writes of the same key
        mutable PerkeyRwlock* perkey_rwlock_for_serializability_ptr_;

        // Non-const shared variables (thread safe)
        mutable CacheWrapperBase* edge_cache_ptr_;
        CooperationWrapperBase* cooperation_wrapper_ptr_;

        // Non-const individual variables
        UdpSocketWrapper* edge_recvreq_socket_server_ptr_;
        UdpSocketWrapper* edge_sendreq_tocloud_socket_client_ptr_;
    };
}

#endif
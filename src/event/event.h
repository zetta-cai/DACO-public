/*
 * Event: store an event name and its latency.
 * 
 * By Siyuan Sheng (2023.07.18).
 */

#ifndef EVENT_H
#define EVENT_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class Event
    {
    public:
        static const std::string INVALID_EVENT_NAME;

        // For reads in edge cache server worker
        static const std::string EDGE_CACHE_SERVER_WORKER_GET_LOCAL_CACHE_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_GET_COOPERATIVE_CACHE_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_LOOKUP_REMOTE_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_LOOKUP_REQ_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_BLOCK_FOR_WRITES_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_REDIRECT_GET_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_REDIRECT_GET_REQ_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_GET_CLOUD_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_GET_REQ_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_UPDATE_INVALID_LOCAL_CACHE_EVENT_NAME;

        // For independent admission in edge cache server worker
        static const std::string EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_UPDATE_DIRECTORY_TO_ADMIT_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_UPDATE_REMOTE_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_UPDATE_REQ_EVENT_NAME;

        // Fot writes in edge cache server worker
        static const std::string EDGE_CACHE_SERVER_WORKER_ACQUIRE_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ACQUIRE_REMOTE_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_ACQUIRE_WRITELOCK_REQ_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_WRITE_CLOUD_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_WRITE_REQ_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_WRITE_LOCAL_CACHE_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_RELEASE_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_RELEASE_REMOTE_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_CACHE_SERVER_WORKER_ISSUE_RELEASE_WRITELOCK_REQ_EVENT_NAME;

        // For edge beacon server
        static const std::string EDGE_BEACON_SERVER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME;
        static const std::string EDGE_BEACON_SERVER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME;
        static const std::string EDGE_BEACON_SERVER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME;

        // For edge beacon server or cache server worker
        static const std::string EDGE_INVALIDATE_CACHE_COPIES_EVENT_NAME;
        static const std::string EDGE_FINISH_BLOCK_EVENT_NAME;
        static const std::string EDGE_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME;
        static const std::string EDGE_VICTIM_FETCH_EVENT_NAME;

        // For edge invalidation server
        static const std::string EDGE_INVALIDATION_SERVER_INVALIDATE_LOCAL_CACHE_EVENT_NAME;

        // For cloud
        static const std::string CLOUD_GET_ROCKSDB_EVENT_NAME;
        static const std::string CLOUD_PUT_ROCKSDB_EVENT_NAME;
        static const std::string CLOUD_DEL_ROCKSDB_EVENT_NAME;

        // For edge cache server victim fetch processor
        static const std::string EDGE_CACHE_SERVER_VICTIM_FETCH_PROCESSOR_FETCHING_EVENT_NAME;

        // For background events
        static const std::string BG_EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME; // For reads in edge cache server worker (non-blocking data fetching)
        static const std::string BG_CLOUD_GET_ROCKSDB_EVENT_NAME; // For cloud (non-blocking data fetching)
        static const std::string BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_ADMISSION_EVENT_NAME; // For admission in edge cache server placement processor (non-blocking placement notification)
        static const std::string BG_EDGE_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME; // For edge beacon server or cache server worker (non-blocking placement notification)
        static const std::string BG_EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME; // For edge beacon server (non-blocking placement notification)

        Event();
        Event(const std::string& event_name, const uint32_t& event_latency_us);
        ~Event();

        std::string getEventName() const;
        uint32_t getEventLatencyUs() const;
        bool isBackgroundEvent() const;

        uint32_t getEventPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const Event& operator=(const Event& other);
    private:
        static const std::string kClassName;

        std::string event_name_;
        uint32_t event_latency_us_;
    };
}

#endif
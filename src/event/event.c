#include "event/event.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>

#include "common/dynamic_array.h"

namespace covered
{
    const std::string Event::INVALID_EVENT_NAME("");

    // For reads in edge cache server worker
    const std::string Event::EDGE_CACHE_SERVER_WORKER_GET_LOCAL_CACHE_EVENT_NAME("edge::cache_server_worker::get_local_cache");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_GET_COOPERATIVE_CACHE_EVENT_NAME("edge::cache_server_worker::get_cooperative_cache");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME("edge::cache_server_worker::lookup_local_directory");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_REMOTE_DIRECTORY_EVENT_NAME("edge::cache_server_worker::lookup_remote_directory");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_LOOKUP_REQ_EVENT_NAME("edge::cache_server_worker::issue_directory_lookup_req");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_BLOCK_FOR_WRITES_EVENT_NAME("edge::cache_server_worker::block_for_writes");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_REDIRECT_GET_EVENT_NAME("edge::cache_server_worker::redirect_get");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_REDIRECT_GET_REQ_EVENT_NAME("edge::cache_server_worker::issue_redirect_get_req");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME("edge::cache_server_worker::target_get_local_cache");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_GET_CLOUD_EVENT_NAME("edge::cache_server_worker::get_cloud");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_GET_REQ_EVENT_NAME("edge::cache_server_worker::issue_global_get_req");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_UPDATE_INVALID_LOCAL_CACHE_EVENT_NAME("edge::cache_server_worker::update_invalid_local_cache");

    // For independent admission in edge cache server worker
    const std::string Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME("edge::cache_server_worker::independent_admission");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_UPDATE_DIRECTORY_TO_ADMIT_EVENT_NAME("edge::cache_server_worker::update_directory_to_admit");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME("edge::cache_server_worker::update_local_directory");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_UPDATE_REMOTE_DIRECTORY_EVENT_NAME("edge::cache_server_worker::update_remote_directory");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_UPDATE_REQ_EVENT_NAME("edge::cache_server_worker::issue_directory_update_req");

    // For writes in edge cache server worker
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_WRITELOCK_EVENT_NAME("edge::cache_server_worker::acquire_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME("edge::cache_server_worker::acquire_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_REMOTE_WRITELOCK_EVENT_NAME("edge::cache_server_worker::acquire_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_ACQUIRE_WRITELOCK_REQ_EVENT_NAME("edge::cache_server_worker::issue_acquire_writelock_req");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_WRITE_CLOUD_EVENT_NAME("edge::cache_server_worker::write_cloud");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_WRITE_REQ_EVENT_NAME("edge::cache_server_worker::issue_global_write_req");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_WRITE_LOCAL_CACHE_EVENT_NAME("edge::cache_server_worker::write_local_cache");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_RELEASE_WRITELOCK_EVENT_NAME("edge::cache_server_worker::release_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME("edge::cache_server_worker::release_local_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_RELEASE_REMOTE_WRITELOCK_EVENT_NAME("edge::cache_server_worker::release_remote_writelock");
    const std::string Event::EDGE_CACHE_SERVER_WORKER_ISSUE_RELEASE_WRITELOCK_REQ_EVENT_NAME("edge::cache_server_worker::issue_release_writelock_req");

    // For edge beacon server
    const std::string Event::EDGE_BEACON_SERVER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME("edge::beacon_server::lookup_local_directory");
    const std::string Event::EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME("edge::beacon_server::update_local_directory");
    const std::string Event::EDGE_BEACON_SERVER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME("edge::beacon_server::acquire_local_writelock");
    const std::string Event::EDGE_BEACON_SERVER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME("edge::beacon_server::release_local_writelock");

    // For edge beacon server or cache server worker
    const std::string Event::EDGE_INVALIDATE_CACHE_COPIES_EVENT_NAME("edge::invalidate_cache_copies");
    const std::string Event::EDGE_FINISH_BLOCK_EVENT_NAME("edge::finish_block");
    const std::string Event::EDGE_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME("edge::update_directory_to_evict");

    // For edge invalidation server
    const std::string Event::EDGE_INVALIDATION_SERVER_INVALIDATE_LOCAL_CACHE_EVENT_NAME("edge::invalidation_server::invalidate_local_cache");

    // For cloud
    const std::string Event::CLOUD_GET_ROCKSDB_EVENT_NAME("cloud::get_rocksdb");
    const std::string Event::CLOUD_PUT_ROCKSDB_EVENT_NAME("cloud::put_rocksdb");
    const std::string Event::CLOUD_DEL_ROCKSDB_EVENT_NAME("cloud::del_rocksdb");

    // For background events
    const std::string Event::BG_EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME("bg::edge::cache_server_worker::target_get_local_cache");
    const std::string Event::BG_CLOUD_GET_ROCKSDB_EVENT_NAME("bg::cloud::get_rocksdb");
    const std::string Event::BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_ADMISSION_EVENT_NAME("bg::edge::cache_server_placement_processor::admission");
    const std::string Event::BG_EDGE_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME("bg::edge::update_directory_to_evict");
    const std::string Event::BG_EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME("bg::edge::beacon_server::update_local_directory");

    const std::string Event::kClassName("Event");

    Event::Event()
    {
        event_name_ = INVALID_EVENT_NAME;
        event_latency_us_ = 0;
    }
    
    Event::Event(const std::string& event_name, const uint32_t& event_latency_us)
    {
        assert(event_name != INVALID_EVENT_NAME);

        event_name_ = event_name;
        event_latency_us_ = event_latency_us;
    }

    Event::~Event() {}

    std::string Event::getEventName() const
    {
        return event_name_;
    }

    uint32_t Event::getEventLatencyUs() const
    {
        return event_latency_us_;
    }

    bool Event::isBackgroundEvent() const
    {
        if (event_name_ == BG_EDGE_CACHE_SERVER_WORKER_TARGET_GET_LOCAL_CACHE_EVENT_NAME || event_name_ == BG_CLOUD_GET_ROCKSDB_EVENT_NAME || event_name_ == BG_EDGE_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME || event_name_ == BG_EDGE_BEACON_SERVER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    uint32_t Event::getEventPayloadSize() const
    {
        // event name length + event name + event latency us
        return sizeof(uint32_t) + event_name_.length() + sizeof(uint32_t);
    }

    uint32_t Event::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_eventname_length = htonl(event_name_.length());
        msg_payload.deserialize(size, (const char*)&bigendian_eventname_length, sizeof(uint32_t));
        size += sizeof(uint32_t);
        msg_payload.deserialize(size, (const char*)(event_name_.data()), event_name_.length());
        size += event_name_.length();
        uint32_t bigendian_event_latency_us = htonl(event_latency_us_);
        msg_payload.deserialize(size, (const char*)&bigendian_event_latency_us, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t Event::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t bigendian_eventname_length = 0;
        msg_payload.serialize(size, (char *)&bigendian_eventname_length, sizeof(uint32_t));
        uint32_t eventname_length = ntohl(bigendian_eventname_length);
        size += sizeof(uint32_t);
        DynamicArray eventname_bytes(eventname_length);
        msg_payload.arraycpy(size, eventname_bytes, 0, eventname_length);
        event_name_ = std::string(eventname_bytes.getBytesRef().data(), eventname_length);
        size += eventname_length;
        uint32_t bigendian_event_latency_us = 0;
        msg_payload.serialize(size, (char *)&bigendian_event_latency_us, sizeof(uint32_t));
        event_latency_us_ = ntohl(bigendian_event_latency_us);
        size += sizeof(uint32_t);
        return size - position;
    }
    
    const Event& Event::operator=(const Event& other)
    {
        event_name_ = other.event_name_;
        event_latency_us_ = other.event_latency_us_;
        return *this;
    }
}
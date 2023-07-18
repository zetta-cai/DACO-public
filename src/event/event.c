#include "event/event.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>

#include "common/dynamic_array.h"

namespace covered
{
    const std::string Event::INVALID_EVENT_NAME("");

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
        event_name_ = std::string(eventname_bytes.getBytes().data(), eventname_length);
        size += eventname_length;
        uint32_t bigendian_event_latency_us = 0;
        msg_payload.serialize(size, (char *)&bigendian_event_latency_us, sizeof(uint32_t));
        event_latency_us_ = ntohl(bigendian_event_latency_us);
        size += sizeof(uint32_t);
        return size - position;
    }
    
    Event& Event::operator=(const Event& other)
    {
        event_name_ = other.event_name_;
        event_latency_us_ = other.event_latency_us_;
        return *this;
    }
}
/*
 * Event: store an event name and its latency.
 * 
 * By Siyuan Sheng (2023.07.18).
 */

#ifndef EVENT_H
#define EVENT_H

#include <string>

namespace covered
{
    class Event
    {
    public:
        static const std::string INVALID_EVENT_NAME;

        Event();
        Event(const std::string& event_name, const uint32_t& event_latency_us);
        ~Event();

        std::string getEventName() const;
        uint32_t getEventLatencyUs() const;

        uint32_t getEventPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        Event& operator=(const Event& other);
    private:
        static const std::string kClassName;

        std::string event_name_;
        uint32_t event_latency_us_;
    };
}

#endif
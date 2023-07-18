/*
 * EventList: store a list of events.
 * 
 * By Siyuan Sheng (2023.07.18).
 */

#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include <string>
#include <vector>

#include "common/dynamic_array.h"
#include "event/event.h"

namespace covered
{
    class EventList
    {
    public:
        EventList();
        ~EventList();

        const std::vector<Event>& getEventsRef() const;

        void addEvent(const Event& event);
        void addEvent(const std::string& event_name, const uint32_t& event_latency_us);
        void addEvents(const std::vector<Event>& events);

        uint32_t getEventListPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        EventList& operator=(const EventList& other);
    private:
        static const std::string kClassName;

        std::vector<Event> events_;
    };
}

#endif
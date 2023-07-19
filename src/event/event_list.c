#include "event/event_list.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string EventList::kClassName("EventList");

    EventList::EventList()
    {
        events_.resize(0);
    }
    
    EventList::~EventList() {}

    const std::vector<Event>& EventList::getEventsRef() const
    {
        if (!Param::isTrackEvent() && events_.size() > 0)
        {
            std::ostringstream oss;
            oss << "size of events_ (" << events_.size() << ") should be 0 when disabling event tracking!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return events_;
    }

    void EventList::addEvent(const Event& event)
    {
        if (Param::isTrackEvent())
        {
            events_.push_back(event);
        }
        return;
    }
    
    void EventList::addEvent(const std::string& event_name, const uint32_t& event_latency_us)
    {
        if (Param::isTrackEvent())
        {
            events_.push_back(Event(event_name, event_latency_us));
        }
        return;
    }

    void EventList::addEvents(const std::vector<Event>& events)
    {
        if (Param::isTrackEvent() && events.size() > 0)
        {
            for (uint32_t i = 0; i < events.size(); i++)
            {
                events_.push_back(events[i]);
            }
        }
        return;
    }

    void EventList::addEvents(const EventList& event_list)
    {
        if (Param::isTrackEvent() && event_list.events_.size() > 0)
        {
            for (uint32_t i = 0; i < event_list.events_.size(); i++)
            {
                events_.push_back(event_list.events_[i]);
            }
        }
        return;
    }

    uint32_t EventList::getEventListPayloadSize() const
    {
        uint32_t payload_size = 0;
        if (Param::isTrackEvent())
        {
            // event list size + event list
            payload_size = sizeof(uint32_t);
            for (uint32_t i = 0; i < events_.size(); i++)
            {
                payload_size += events_[i].getEventPayloadSize();
            }
        }

        // NOTE: NOT embed event list if without event tracking
        assert(!Param::isTrackEvent() && payload_size == 0);

        return payload_size;
    }

    uint32_t EventList::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;

        if (Param::isTrackEvent())
        {
            // Event list size
            uint32_t bigendian_eventlist_size = htonl(events_.size());
            msg_payload.deserialize(size, (const char*)&bigendian_eventlist_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Event list
            for (uint32_t i = 0; i < events_.size(); i++)
            {
                uint32_t tmp_event_serialize_size = events_[i].serialize(msg_payload, size);
                size += tmp_event_serialize_size;
            }
        }

        // NOTE: NOT embed event list if without event tracking
        assert(!Param::isTrackEvent() && size == position);

        return size - position;
    }

    uint32_t EventList::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;

        if (Param::isTrackEvent())
        {
            // Event list
            uint32_t bigendian_eventlist_size = 0;
            msg_payload.serialize(size, (char *)&bigendian_eventlist_size, sizeof(uint32_t));
            uint32_t evenlist_size = ntohl(bigendian_eventlist_size);
            size += sizeof(uint32_t);

            // Allocate space for evenlist_size events
            events_.resize(evenlist_size);

            // Event list
            for (uint32_t i = 0; i < evenlist_size; i++)
            {
                Event tmp_event;
                uint32_t tmp_event_deserialize_size = tmp_event.deserialize(msg_payload, size);
                events_[i] = tmp_event;
                size += tmp_event_deserialize_size;
            }
        }

        // NOTE: NOT embed event list if without event tracking
        assert(!Param::isTrackEvent() && size == position);

        return size - position;
    }

    std::string EventList::toString() const
    {
        std::ostringstream oss;
        if (Param::isTrackEvent() && events_.size() > 0)
        {
            for (uint32_t i = 0; i < events_.size(); i++)
            {
                oss << "<" << events_[i].getEventName() << ", " << events_[i].getEventLatencyUs() << " us> ";
            }
        }
        return oss.str();
    }

    EventList& EventList::operator=(const EventList& other)
    {
        events_ = other.events_; // deep copy
        return *this;
    }
}
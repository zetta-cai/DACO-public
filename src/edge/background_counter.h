/*
 * BackgroundCounter: count background events and bandwidth usage for debugging and measurement (thread safe).
 * 
 * By Siyuan Sheng (2023.09.26).
 */

#ifndef BACKGROUND_COUNTER_H
#define BACKGROUND_COUNTER_H

#include <string>

#include "common/bandwidth_usage.h"
#include "concurrency/rwlock.h"
#include "event/event_list.h"

namespace covered
{
    class BackgroundCounter
    {
    public:
        BackgroundCounter();
        ~BackgroundCounter();

        void updateBandwidthUsgae(const BandwidthUsage& other);
        void addEvent(const Event& event);
        void addEvent(const std::string& event_name, const uint32_t& event_latency_us);
        void addEvents(const std::vector<Event>& events);
        void addEvents(const EventList& event_list);

        bool loadAndReset(BandwidthUsage& bandwidth_usage, EventList& event_list); // Return is empty before reset
    private:
        static const std::string kClassName;

        void assertBackgroundEvent_(const Event& event) const;

        Rwlock* rwlock_for_bankground_counter_ptr_; // For atomicity

        bool is_empty_;
        BandwidthUsage bandwidth_usage_;
        EventList event_list_;
    };
}

#endif
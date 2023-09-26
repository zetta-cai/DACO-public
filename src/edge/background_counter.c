#include "edge/background_counter.h"

namespace covered
{
    const std::string BackgroundCounter::kClassName = "BackgroundCounter";

    BackgroundCounter::BackgroundCounter() : is_empty_(true), bandwidth_usage_(), event_list_()
    {
        std::ostringstream oss;
        oss << kClassName << " rwlock_for_bankground_counter_ptr_";
        rwlock_for_bankground_counter_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_bankground_counter_ptr_ != NULL);
    }

    BackgroundCounter::~BackgroundCounter()
    {
        assert(rwlock_for_bankground_counter_ptr_ != NULL);
        delete rwlock_for_bankground_counter_ptr_;
        rwlock_for_bankground_counter_ptr_ = NULL;
    }

    void BackgroundCounter::updateBandwidthUsgae(const BandwidthUsage& other)
    {
        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::updateBandwidthUsgae()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        bandwidth_usage_.update(other);
        is_empty_ = false;

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return;
    }

    void BackgroundCounter::addEvent(const Event& event)
    {
        assert(event.isBackgroundEvent());

        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::addEvent()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        event_list_.addEvent(event);
        is_empty_ = false;

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return;
    }

    void BackgroundCounter::addEvent(const std::string& event_name, const uint32_t& event_latency_us)
    {
        Event tmp_event(event_name, event_latency_us);
        assert(tmp_event.isBackgroundEvent());

        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::addEvent()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        event_list_.addEvent(tmp_event);
        is_empty_ = false;

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return;
    }

    void BackgroundCounter::addEvents(const std::vector<Event>& events)
    {
        for (uint32_t i = 0; i < events.size(); i++)
        {
            assert(events[i].isBackgroundEvent());
        }

        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::addEvents()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        event_list_.addEvents(events);
        is_empty_ = false;

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return;
    }

    void BackgroundCounter::addEvents(const EventList& event_list)
    {
        const std::vector<Event>& events_ref = event_list.getEventsRef();
        for (uint32_t i = 0; i < events_ref.size(); i++)
        {
            assert(events_ref[i].isBackgroundEvent());
        }

        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::addEvents()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        event_list_.addEvents(event_list);
        is_empty_ = false;

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return;
    }

    bool BackgroundCounter::loadAndReset(BandwidthUsage& bandwidth_usage, EventList& event_list)
    {
        // Acquire a write lock
        const std::string context_name = "BackgroundCounter::loadAndReset()";
        rwlock_for_bankground_counter_ptr_->acquire_lock(context_name);

        bool is_empty_before_reset = is_empty_;
        if (!is_empty_before_reset)
        {
            // Load
            bandwidth_usage = bandwidth_usage_;
            event_list = event_list_;

            // Reset
            bandwidth_usage_ = BandwidthUsage();
            event_list_ = EventList();
            is_empty_ = true;
        }

        rwlock_for_bankground_counter_ptr_->unlock(context_name);
        return is_empty_before_reset;
    }
}
/*
 * BlockingRingBuffer: provide blocking/interruption-based ring buffer interfaces for concurrency between provider(s) and customer(s) (based on locking for condition variable).
 *
 * NOTE: as polling-based RingBuffer has better performance than interruption-based BlockingRingBuffer, this is ONLY used by low-priority threads to reduce CPU resource usage.
 * 
 * By Siyuan Sheng (2023.12.21).
 */

#ifndef BLOCKING_RING_BUFFER_H
#define BLOCKING_RING_BUFFER_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace covered
{
    // NOTE: class T must support default constructor and operator=.
    template<class T>
    class BlockingRingBuffer
    {
    public:
        typedef bool (*finish_condition_func_t)(void*);

        BlockingRingBuffer(const T& default_element, const uint32_t& buffer_size, finish_condition_func_t finish_condition_func = NULL);
        ~BlockingRingBuffer();
        void getAllToRelease(std::vector<T>& remaining_elements); // NOTE: NO need to acquire any lock due to ONLY being invoked once in deconstructor

        // NOTE: thread-safe structure cannot return a reference, which may violate atomicity
        bool push(const T& element);
        bool pop(T& element, void* finish_condition_param_ptr); // Blocking pop: wait until finish condition if any is satisfied (return is_successful as false) or ring buffer is not empty (return is_successful as true)
        void notifyFinish(void* finish_condition_param_ptr) const; // NOTE: finish_condition_func_ MUST NOT NULL and return true

        uint32_t getElementCnt() const;
        uint32_t getBufferSize() const;
        T getDefaultElement() const;
        finish_condition_func_t getFinishConditionFunc() const;

        const BlockingRingBuffer<T>& operator=(const BlockingRingBuffer<T>& other);
    private:
        static const std::string kClassName;

        // For wait/notify
        mutable std::mutex condition_mutex_; // For atomicity of head_ and tail_
        finish_condition_func_t finish_condition_func_;
        mutable std::condition_variable condition_variable_;

        volatile uint32_t head_;
		volatile uint32_t tail_;
		volatile uint32_t buffer_size_; // Come from Config::data_request_buffer_size_
        T default_element_;
        std::vector<T> ring_buffer_;
    };
}

#endif
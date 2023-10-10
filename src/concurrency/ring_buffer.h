/*
 * RingBuffer: provide ring buffer interfaces for concurrency between a provider and a customer (lock free).
 * 
 * By Siyuan Sheng (2023.06.14).
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <string>
#include <vector>

#include "concurrency/rwlock.h"

namespace covered
{
    // NOTE: class T must support default constructor and operator=.
    template<class T>
    class RingBuffer
    {
    public:
        RingBuffer(const T& default_element, const uint32_t& buffer_size, const bool& with_multi_providers);
        ~RingBuffer();

        // NOTE: thread-safe structure cannot return a reference, which may violate atomicity
        bool push(const T& element);
        bool pop(T& element);

        bool withMultiProviders() const;
        uint32_t getElementCnt() const;
        uint32_t getBufferSize() const;
        T getDefaultElement() const;

        //uint64_t getSizeForCapacity() const;

        // NOTE: if you want to use getElementsForDebug() for debugging under multiple providers/readers, providers/readers of ring buffer should be protected by read-write locking
        //std::vector<T> getElementsForDebug() const;

        const RingBuffer<T>& operator=(const RingBuffer<T>& other);
    private:
        static const std::string kClassName;

        bool with_multi_providers_; // If need to support multiple providers
        Rwlock* rwlock_for_multi_providers_ptr_; // Provide atomicity for multiple providers (only work if with_multi_providers_ is true)

        volatile uint32_t head_;
		volatile uint32_t tail_;
		volatile uint32_t buffer_size_; // Come from Config::data_request_buffer_size_
        T default_element_;
        std::vector<T> ring_buffer_;
    };
}

#endif
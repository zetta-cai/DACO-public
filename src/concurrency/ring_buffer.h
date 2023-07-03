/*
 * RingBuffer: provide ring buffer interfaces for concurrency between a provider and a customer (lock free).
 * 
 * By Siyuan Sheng (2023.06.14).
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <string>
#include <vector>

namespace covered
{
    // NOTE: class T must support default constructor and operator=.
    template<class T>
    class RingBuffer
    {
    public:
        RingBuffer(const T& default_element, const uint32_t& buffer_size);
        ~RingBuffer();

        // NOTE: thread-safe structure cannot return a reference, which may violate atomicity
        bool push(const T& element);
        bool pop(T& element);

        uint32_t getElementCnt() const;
        uint32_t getBufferSize() const;
        T getDefaultElement() const;

        //uint32_t getSizeForCapacity() const;

        RingBuffer<T>& operator=(const RingBuffer<T>& other);
    private:
        static const std::string kClassName;

        uint32_t head_;
		uint32_t tail_;
		uint32_t buffer_size_; // Come from Config::data_request_buffer_size_
        T default_element_;
        std::vector<T> ring_buffer_;
    };
}

#endif
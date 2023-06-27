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
    template<class T>
    class RingBuffer
    {
    public:
        static const uint32_t RINGBUFFER_CAPACITY;

        RingBuffer(const T& default_element, const uint32_t& capacity = RINGBUFFER_CAPACITY);
        ~RingBuffer();

        bool push(const T& element);
        bool pop(T& element);

        uint32_t getElementCnt() const;
        uint32_t getCapacity() const;
        T getDefaultElement() const;

        //uint32_t getSizeForCapacity() const;

        RingBuffer<T>& operator=(const RingBuffer<T>& other);
    private:
        static const std::string kClassName;

        uint32_t head_;
		uint32_t tail_;
		uint32_t capacity_;
        T default_element_;
        std::vector<T> ring_buffer_;
    };
}

#endif
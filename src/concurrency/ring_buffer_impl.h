#ifndef RING_BUFFER_IMPL_H
#define RING_BUFFER_IMPL_H

#include "concurrency/ring_buffer.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    template<class T>
    const std::string RingBuffer<T>::kClassName = "RingBuffer<" + std::string(typeid(T).name()) + ">";

    template<class T>
    RingBuffer<T>::RingBuffer(const T& default_element, const uint32_t& buffer_size)
    {
        assert(buffer_size > 0);

        head_ = 0;
        tail_ = 0;
        buffer_size_ = buffer_size;
        default_element_ = default_element;
        ring_buffer_.resize(buffer_size, default_element);
    }

    template<class T>
    RingBuffer<T>::~RingBuffer()
    {
        /*
        // NOTE: poped pointers are released outside RingBuffer, yet remaining pointers in ring_buffer_ are released by RingBuffer
        while (true)
        {
            T* ptr = pop();
            if (ptr != NULL)
            {
                delete ptr;
                ptr = NULL;
            }
            else
            {
                break;
            }
        }
        */
    }

    template<class T>
    bool RingBuffer<T>::push(const T& element)
    {
        /*
        if (ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "ptr is NULL for push()!");
            exit(1);
        }
        */

        bool is_successful = false;

        if ((head_ + 1) % buffer_size_ == tail_) // ring buffer is full
        {
            Util::dumpWarnMsg(kClassName, "ring buffer is fulll!");
            is_successful = false;
        }
        else // still with free space
        {
            //assert(ring_buffer_[head_] == NULL);
            ring_buffer_[head_] = element;
            //assert(ring_buffer_[head_] != NULL);
            
            head_ = (head_ + 1) % buffer_size_;
            is_successful = true;
        }

        return is_successful;
    }

    template<class T>
    bool RingBuffer<T>::pop(T& element)
    {
        bool is_successful = false;

        if (tail_ == head_) // ring buffer is empty
        {
            is_successful = false;
        }
        else // ring buffer is NOT empty
        {
            element = ring_buffer_[tail_];
            //assert(result != NULL);
            ring_buffer_[tail_] = default_element_;

            tail_ = (tail_ + 1) % buffer_size_;
            is_successful = true;
        }

        return is_successful;
    }

    template<class T>
    uint32_t RingBuffer<T>::getElementCnt() const
    {
        uint32_t size = 0;
        if (head_ >= tail_)
        {
            size = head_ - tail_;
        }
        else
        {
            size = head_ + buffer_size_ - tail_;
        }
        assert(size >= 0 && size < buffer_size_);
        return size;
    }

    template<class T>
    uint32_t RingBuffer<T>::getBufferSize() const
    {
        return buffer_size_;
    }

    template<class T>
    T RingBuffer<T>::getDefaultElement() const
    {
        return default_element_;
    }

    /*template<class T>
    uint32_t RingBuffer<T>::getSizeForCapacity() const
    {
        uint32_t size = 0;
        for (uint32_t ring_buffer_idx = tail_; ring_buffer_idx != head_; ring_buffer_idx++)
        {
            if (ring_buffer_idx >= buffer_size_)
            {
                ring_buffer_idx %= buffer_size_;
            }
            size += ring_buffer_[ring_buffer_idx].getSizeForCapacity();
        }
        return size;
    }*/

    template<class T>
    RingBuffer<T>& RingBuffer<T>::operator=(const RingBuffer<T>& other)
    {
        head_ = other.head_;
        tail_ = other.tail_;
        buffer_size_ = other.buffer_size_;
        default_element_ = other.default_element_;
        ring_buffer_ = other.ring_buffer_; // Deep copy
        return *this;
    }
}

#endif
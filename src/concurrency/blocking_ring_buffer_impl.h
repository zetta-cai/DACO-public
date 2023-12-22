#ifndef BLOCKING_RING_BUFFER_IMPL_H
#define BLOCKING_RING_BUFFER_IMPL_H

#include "concurrency/blocking_ring_buffer.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    template<class T>
    const std::string BlockingRingBuffer<T>::kClassName = "BlockingRingBuffer<" + std::string(typeid(T).name()) + ">";

    template<class T>
    BlockingRingBuffer<T>::BlockingRingBuffer(const T& default_element, const uint32_t& buffer_size, finish_condition_func_t finish_condition_func)
    {
        assert(buffer_size > 0);

        finish_condition_func_ = finish_condition_func;

        head_ = 0;
        tail_ = 0;
        buffer_size_ = buffer_size;
        default_element_ = default_element;
        ring_buffer_.resize(buffer_size, default_element);
    }

    template<class T>
    BlockingRingBuffer<T>::~BlockingRingBuffer() {}

    template<class T>
    void BlockingRingBuffer<T>::getAllToRelease(std::vector<T>& remaining_elements)
    {
        // NOTE: NO need to acquire any lock due to ONLY being invoked once in deconstructor

        while (tail_ != head_)
        {
            remaining_elements.push_back(ring_buffer_[tail_]);
            ring_buffer_[tail_] = default_element_;

            tail_ = (tail_ + 1) % buffer_size_;
        }

        return;
    }

    template<class T>
    bool BlockingRingBuffer<T>::push(const T& element)
    {
        // NOTE: invoked by provider

        bool is_successful = false;

        // Push element into ring buffer atomically
        {
            // Acquire condition mutex lock
            std::lock_guard lock_guard(condition_mutex_);

            // Push element (i.e., set ring buffer condition)
            if ((head_ + 1) % buffer_size_ == tail_) // ring buffer is full
            {
                Util::dumpWarnMsg(kClassName, "ring buffer is fulll!");
                is_successful = false;
            }
            else // still with free space
            {
                ring_buffer_[head_] = element;
                
                head_ = (head_ + 1) % buffer_size_;
                is_successful = true;
            }
        } // NOTE: condition mutex lock will be released when destroying the lock guard

        if (is_successful) // If push a new element into ring buffer
        {
            condition_variable_.notify_one(); // Notify a consumer waiting for the condition variable
        }

        return is_successful;
    }

    template<class T>
    bool BlockingRingBuffer<T>::pop(T& element, void* finish_condition_param_ptr)
    {
        // NOTE: invoked by consumer

        bool is_successful = false;
        bool with_finish_condition = false;
        bool with_ring_buffer_condition = false;

        // Wait until finish condition is satisfied or provider pushes a new element into ring buffer
        std::unique_lock unique_lock(condition_mutex_); // Acquire condition mutex lock
        condition_variable_.wait(unique_lock,
            [&]()
            {
                // NOTE: condition mutex lock MUST be acquired when checking finish and ring buffer conditions in this reference-based lambda function
                if (finish_condition_func_ != NULL)
                {
                    with_finish_condition = finish_condition_func_(finish_condition_param_ptr);
                }
                with_ring_buffer_condition = (tail_ != head_);
                return with_finish_condition || with_ring_buffer_condition;
            }
        ); // NOTE: condition mutex lock will be released when waiting for the condition variable and re-acquired when the condition variable is notified

        // NOTE: condition mutex lock is still acquired after waking from conditional wait
        if (with_finish_condition) // Finish condition is satisfied
        {
            // NOTE: NOT set element due to already finished

            unique_lock.unlock();
            is_successful = false;
        }
        else // Finish condition is NOT satisfied
        {
            assert(with_ring_buffer_condition); // Ring buffer condition MUST be satisfied

            // Set element
            assert(tail_ != head_); // Ring buffer MUST NOT empty
            element = ring_buffer_[tail_];
            ring_buffer_[tail_] = default_element_;
            tail_ = (tail_ + 1) % buffer_size_;

            unique_lock.unlock();
            is_successful = true;
        }

        return is_successful;
    }

    template<class T>
    void BlockingRingBuffer<T>::notifyFinish(void* finish_condition_param_ptr) const
    {
        // NOTE: invoked by provider

        // NOTE: finish condition MUST be satisfied
        assert(finish_condition_func_ != NULL);
        assert(finish_condition_func_(finish_condition_param_ptr) == true);

        // NOTE: acquire&release condition mutex lock is important here, which ensures that either finish condition is true before consumer checks condition, or provider's notification happens after consumer's conditional wait -> set finish condition and notification in provider will NOT happen between check and wait in consumer, which will incur dead consumer
        {
            // Acquire condition mutex lock
            std::lock_guard lock_guard(condition_mutex_);
        } // NOTE: condition mutex lock will be released when destroying the lock guard

        condition_variable_.notify_all(); // Notify all consumers waiting for the condition variable

        return;
    }

    template<class T>
    uint32_t BlockingRingBuffer<T>::getElementCnt() const
    {
        uint32_t size = 0;

        // Acquire condition mutex
        condition_mutex_.lock();

        // Get the number of elements in ring buffer atomically
        if (head_ >= tail_)
        {
            size = head_ - tail_;
        }
        else
        {
            size = head_ + buffer_size_ - tail_;
        }
        assert(size >= 0 && size < buffer_size_);

        // Release condition mutex
        condition_mutex_.unlock();

        return size;
    }

    template<class T>
    uint32_t BlockingRingBuffer<T>::getBufferSize() const
    {
        // NOTE: NO need to acquire condition mutex due to buffer_size_ is NEVER changed after initialization

        return buffer_size_;
    }

    template<class T>
    T BlockingRingBuffer<T>::getDefaultElement() const
    {
        // NOTE: NO need to acquire condition mutex due to default_element_ is NEVER changed after initialization

        return default_element_;
    }

    template<class T>
    typename BlockingRingBuffer<T>::finish_condition_func_t BlockingRingBuffer<T>::getFinishConditionFunc() const
    {
        // NOTE: NO need to acquire condition mutex due to finish_condition_func_ is NEVER changed after initialization

        return finish_condition_func_;
    }

    template<class T>
    const BlockingRingBuffer<T>& BlockingRingBuffer<T>::operator=(const BlockingRingBuffer<T>& other)
    {
        finish_condition_func_ = other.finish_condition_func_;

        head_ = other.head_;
        tail_ = other.tail_;
        buffer_size_ = other.buffer_size_;
        default_element_ = other.default_element_;
        ring_buffer_ = other.ring_buffer_; // Deep copy
        return *this;
    }
}

#endif
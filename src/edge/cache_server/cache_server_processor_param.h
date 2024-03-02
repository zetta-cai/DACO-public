/*
 * CacheServerProcessorParam: parameters to launch cache server processors (including metadata update, placement, redirection, and victim fetch) in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.11.28).
 */

#ifndef CACHE_SERVER_PROCESSOR_PARAM_H
#define CACHE_SERVER_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerProcessorParam;
}

#include "common/subthread_param_base.h"
#include "concurrency/blocking_ring_buffer_impl.h"
#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server_base.h"

namespace covered
{   
    class CacheServerProcessorParam : public SubthreadParamBase
    {
    public:
        CacheServerProcessorParam();
        CacheServerProcessorParam(CacheServerBase* cache_server_ptr, const uint32_t& data_request_buffer_size, const bool& is_polling_based);
        ~CacheServerProcessorParam();

        const CacheServerProcessorParam& operator=(const CacheServerProcessorParam& other);

        bool isPollingBase() const;
        CacheServerBase* getCacheServerPtr() const;

        bool push(const CacheServerItem& cache_server_item);
        bool pop(CacheServerItem& cache_server_item, void* edge_wrapper_ptr); // NOTE: edge_wrapper_ptr is ONLY used if is_polling_based_ = false
        void notifyFinishIfNecessary(void* edge_wrapper_ptr) const; // NOTE: notify condition variable ONLY if is_polling_based_ = false
    private:
        static const std::string kClassName;

        // Static function for finish condition of interruption-based ring buffer
        static bool isNodeFinish_(void* edge_wrapper_ptr);

        // RingBuffer<CacheServerItem>* getCacheServerItemBufferPtr_() const;
        // BlockingRingBuffer<CacheServerItem>* getCacheServerItemBlockingItemBufferPtr_() const;

        // Const variable
        bool is_polling_based_;

        CacheServerBase* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* cache_server_item_buffer_ptr_; // thread safe (used if is_polling_based_ = true)
        BlockingRingBuffer<CacheServerItem>* cache_server_item_blocking_buffer_ptr_; // thread safe (used if is_polling_based_ = false)
    };
}

#endif
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

#include "concurrency/blocking_ring_buffer_impl.h"
#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class CacheServerProcessorParam
    {
    public:
        CacheServerProcessorParam();
        CacheServerProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size, const bool& is_polling_based);
        ~CacheServerProcessorParam();

        const CacheServerProcessorParam& operator=(const CacheServerProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getCacheServerItemBufferPtr() const;
        BlockingRingBuffer<CacheServerItem>* getCacheServerItemBlockingItemBufferPtr() const;
    private:
        static const std::string kClassName;

        // Static function for finish condition of interruption-based ring buffer
        static bool isNodeFinish(void* edge_wrapper_ptr);

        // Const variable
        bool is_polling_based_;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* cache_server_item_buffer_ptr_; // thread safe (used if is_polling_based_ = true)
        BlockingRingBuffer<CacheServerItem>* cache_server_item_blocking_buffer_ptr_; // thread safe (used if is_polling_based_ = false)
    };
}

#endif
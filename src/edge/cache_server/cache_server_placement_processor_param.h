/*
 * CacheServerPlacementProcessorParam: parameters to launch a cache server worker in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef CACHE_SERVER_PLACEMENT_PROCESSOR_PARAM_H
#define CACHE_SERVER_PLACEMENT_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerPlacementProcessorParam;
}

#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class CacheServerPlacementProcessorParam
    {
    public:
        CacheServerPlacementProcessorParam();
        CacheServerPlacementProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size);
        ~CacheServerPlacementProcessorParam();

        const CacheServerPlacementProcessorParam& operator=(const CacheServerPlacementProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getDataRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* data_request_buffer_ptr_; // thread safe
    };
}

#endif
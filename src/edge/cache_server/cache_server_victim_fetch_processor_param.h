/*
 * CacheServerVictimFetchProcessorParam: parameters to launch a cache server victim fetch processor in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef CACHE_SERVER_VICTIM_FETCH_PROCESSOR_PARAM_H
#define CACHE_SERVER_VICTIM_FETCH_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerVictimFetchProcessorParam;
}

#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class CacheServerVictimFetchProcessorParam
    {
    public:
        CacheServerVictimFetchProcessorParam();
        CacheServerVictimFetchProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size);
        ~CacheServerVictimFetchProcessorParam();

        const CacheServerVictimFetchProcessorParam& operator=(const CacheServerVictimFetchProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getControlRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* control_request_buffer_ptr_; // thread safe
    };
}

#endif
/*
 * CacheServerRedirectionProcessorParam: parameters to launch a cache server redirection processor in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.10.25).
 */

#ifndef CACHE_SERVER_REDIRECTION_PROCESSOR_PARAM_H
#define CACHE_SERVER_REDIRECTION_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerRedirectionProcessorParam;
}

#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class CacheServerRedirectionProcessorParam
    {
    public:
        CacheServerRedirectionProcessorParam();
        CacheServerRedirectionProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size);
        ~CacheServerRedirectionProcessorParam();

        const CacheServerRedirectionProcessorParam& operator=(const CacheServerRedirectionProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getDataRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* data_request_buffer_ptr_; // thread safe
    };
}

#endif
/*
 * CacheServerMetadataUpdateProcessorParam: parameters to launch a cache server metadata update processor in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.11.28).
 */

#ifndef CACHE_SERVER_METADATA_UPDATE_PROCESSOR_PARAM_H
#define CACHE_SERVER_METADATA_UPDATE_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerMetadataUpdateProcessorParam;
}

#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class CacheServerMetadataUpdateProcessorParam
    {
    public:
        CacheServerMetadataUpdateProcessorParam();
        CacheServerMetadataUpdateProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size);
        ~CacheServerMetadataUpdateProcessorParam();

        const CacheServerMetadataUpdateProcessorParam& operator=(const CacheServerMetadataUpdateProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getControlRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* control_request_buffer_ptr_; // thread safe
    };
}

#endif
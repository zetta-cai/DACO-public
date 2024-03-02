/*
 * CacheServerWorkerParam: parameters to launch a cache server worker in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_WORKER_PARAM_H
#define CACHE_SERVER_WORKER_PARAM_H

#include <string>

namespace covered
{
    class CacheServerWorkerParam;
}

#include "common/subthread_param_base.h"
#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server_base.h"

namespace covered
{   
    class CacheServerWorkerParam : public SubthreadParamBase
    {
    public:
        CacheServerWorkerParam();
        CacheServerWorkerParam(CacheServerBase* cache_server_ptr, const uint32_t& local_cache_server_worker_idx, const uint32_t& data_request_buffer_size);
        ~CacheServerWorkerParam();

        const CacheServerWorkerParam& operator=(const CacheServerWorkerParam& other);

        CacheServerBase* getCacheServerPtr() const;
        uint32_t getLocalCacheServerWorkerIdx() const;
        RingBuffer<CacheServerItem>* getDataRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServerBase* cache_server_ptr_; // thread safe
        uint32_t local_cache_server_worker_idx_; // const shared variable
        RingBuffer<CacheServerItem>* data_request_buffer_ptr_; // thread safe
    };
}

#endif
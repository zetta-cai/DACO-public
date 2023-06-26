/*
 * CacheServerWorkerParam: parameters to launch a cache server worker in an edge node.
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_WORKER_PARAM_H
#define CACHE_SERVER_WORKER_PARAM_H

#include <string>

#include "common/ring_buffer_impl.h"
#include "edge/edge_wrapper.h"

namespace covered
{
    // Item passed between cache server and cache server workers
    class CacheServerWorkerItem
    {
    public:
        CacheServerWorkerItem();
        CacheServerWorkerItem(MessageBase* local_request_ptr, const NetworkAddr& client_addr);
        ~CacheServerWorkerItem();

        MessageBase* getLocalRequestPtr() const;
        NetworkAddr getClientAddr() const;

        CacheServerWorkerItem& operator=(const CacheServerWorkerItem& other);
    private:
        MessageBase* local_request_ptr_;
        NetworkAddr client_addr_;
    };

    class CacheServerWorkerParam
    {
    public:
        static const uint32_t LOCAL_REQUEST_BUFFER_CAPACITY;

        CacheServerWorkerParam();
        CacheServerWorkerParam(EdgeWrapper* edge_wrapper_ptr, uint32_t local_cache_server_worker_idx, const uint32_t& buffer_capacity = LOCAL_REQUEST_BUFFER_CAPACITY);
        ~CacheServerWorkerParam();

        const CacheServerWorkerParam& operator=(const CacheServerWorkerParam& other);

        EdgeWrapper* getEdgeWrapperPtr() const;
        uint32_t getLocalCacheServerWorkerIdx() const;
        RingBuffer<CacheServerWorkerItem>* getLocalRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        EdgeWrapper* edge_wrapper_ptr_;
        uint32_t local_cache_server_worker_idx_;
        RingBuffer<CacheServerWorkerItem>* local_request_buffer_ptr_;
    };
}

#endif
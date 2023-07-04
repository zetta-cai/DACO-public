/*
 * CacheServerWorkerParam: parameters to launch a cache server worker in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_WORKER_PARAM_H
#define CACHE_SERVER_WORKER_PARAM_H

#include <string>

#include "concurrency/ring_buffer_impl.h"
#include "edge/edge_wrapper.h"

namespace covered
{
    // Item passed between cache server and cache server workers
    class CacheServerWorkerItem
    {
    public:
        CacheServerWorkerItem();
        CacheServerWorkerItem(MessageBase* data_request_ptr, const NetworkAddr& network_addr);
        ~CacheServerWorkerItem();

        MessageBase* getDataRequestPtr() const;
        NetworkAddr getNetworkAddr() const;

        CacheServerWorkerItem& operator=(const CacheServerWorkerItem& other);
    private:
        MessageBase* data_request_ptr_;
        NetworkAddr network_addr_;
    };

    class CacheServerWorkerParam
    {
    public:
        CacheServerWorkerParam();
        CacheServerWorkerParam(EdgeWrapper* edge_wrapper_ptr, uint32_t local_cache_server_worker_idx, const uint32_t& data_request_buffer_size);
        ~CacheServerWorkerParam();

        const CacheServerWorkerParam& operator=(const CacheServerWorkerParam& other);

        EdgeWrapper* getEdgeWrapperPtr() const;
        uint32_t getLocalCacheServerWorkerIdx() const;
        RingBuffer<CacheServerWorkerItem>* getDataRequestBufferPtr() const;
    private:
        static const std::string kClassName;

        EdgeWrapper* edge_wrapper_ptr_; // thread safe
        uint32_t local_cache_server_worker_idx_; // const shared variable
        RingBuffer<CacheServerWorkerItem>* data_request_buffer_ptr_; // thread safe
    };
}

#endif
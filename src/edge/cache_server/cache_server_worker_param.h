/*
 * CacheServerWorkerParam: parameters to launch a cache server worker in an edge node.
 * 
 * By Siyuan Sheng (2023.06.26).
 */

#ifndef CACHE_SERVER_WORKER_PARAM_H
#define CACHE_SERVER_WORKER_PARAM_H

#include <string>

#include "edge/edge_wrapper.h"

namespace covered
{
    class CacheServerWorkerParam
    {
    public:
        CacheServerWorkerParam();
        CacheServerWorkerParam(EdgeWrapper* edge_wrapper_ptr, uint32_t local_cache_server_worker_idx);
        ~CacheServerWorkerParam();

        const CacheServerWorkerParam& operator=(const CacheServerWorkerParam& other);

        EdgeWrapper* getEdgeWrapperPtr() const;
        uint32_t getLocalCacheServerWorkerIdx() const;
    private:
        static const std::string kClassName;

        EdgeWrapper* edge_wrapper_ptr_;
        uint32_t local_cache_server_worker_idx_;
    };
}

#endif
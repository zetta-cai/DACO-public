#include "edge/cache_server/cache_server_worker_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerWorkerParam::kClassName("CacheServerWorkerParam");

    CacheServerWorkerParam::CacheServerWorkerParam()
    {
        edge_wrapper_ptr_ = NULL;
        local_cache_server_worker_idx_ = 0;
    }

    CacheServerWorkerParam::CacheServerWorkerParam(EdgeWrapper* edge_wrapper_ptr, uint32_t local_cache_server_worker_idx)
    {
        edge_wrapper_ptr_ = edge_wrapper_ptr;
        local_cache_server_worker_idx_ = local_cache_server_worker_idx;
    }

    CacheServerWorkerParam::~CacheServerWorkerParam() {}

    const CacheServerWorkerParam& CacheServerWorkerParam::operator=(const CacheServerWorkerParam& other)
    {
        edge_wrapper_ptr_ = other.edge_wrapper_ptr_;
        local_cache_server_worker_idx_ = other.local_cache_server_worker_idx_;
        return *this;
    }

    EdgeWrapper* CacheServerWorkerParam::getEdgeWrapperPtr() const
    {
        return edge_wrapper_ptr_;
    }

    uint32_t CacheServerWorkerParam::getLocalCacheServerWorkerIdx() const
    {
        return local_cache_server_worker_idx_;
    }
}
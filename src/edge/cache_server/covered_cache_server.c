#include "edge/cache_server/covered_cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string CoveredCacheServer::kClassName("CoveredCacheServer");

    CoveredCacheServer::CoveredCacheServer(EdgeWrapper* edge_wrapper_ptr) : CacheServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ == Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();

        // Differentiate CoveredCacheServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredCacheServer::~CoveredCacheServer() {}

    // (1) Data requests

    bool CoveredCacheServer::processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const
    {
        // TODO: acquire a read lock for serializability before accessing any shared variable in the target edge node

        return false;
    }

    void CoveredCacheServer::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // NOTE: no need to acquire per-key rwlock for serializability, which has been done in processRedirectedGetRequest_() and processRedirectedWriteRequest_()

        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        std::ostringstream oss;
        Util::dumpErrorMsg(instance_name_, "triggerIndependentAdmission_() should NOT be invoked in CoveredCacheServer!");
        exit(1);
        return;
    }
}
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

    // (1) Process data requests

    bool CoveredCacheServer::processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const
    {
        return false;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool CoveredCacheServer::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        return false;
    }

    bool CoveredCacheServer::redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const
    {
        return false;
    }

    // (2.2) Update content directory information

    bool CoveredCacheServer::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) const
    {
        return false;
    }

    // (2.3) Process writes and block for MSI protocol

    bool CoveredCacheServer::acquireBeaconWritelock_(const Key& key, LockResult& lock_result)
    {
        return false;
    }

    bool CoveredCacheServer::releaseBeaconWritelock_(const Key& key)
    {
        return false;
    }

    // (5) Admit uncached objects in local edge cache

    bool CoveredCacheServer::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        std::ostringstream oss;
        Util::dumpErrorMsg(instance_name_, "triggerIndependentAdmission_() should NOT be invoked in CoveredCacheServer!");
        exit(1);
        return false;
    }
}
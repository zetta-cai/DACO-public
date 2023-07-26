#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string CoveredCacheServerWorker::kClassName("CoveredCacheServerWorker");

    CoveredCacheServerWorker::CoveredCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapper* tmp_edgewrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_;
        assert(tmp_edgewrapper_ptr->cache_name_ == Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edgewrapper_ptr->node_idx_;
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate CoveredCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    CoveredCacheServerWorker::~CoveredCacheServerWorker() {}

    // (1) Process data requests

    bool CoveredCacheServerWorker::processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        return false;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool CoveredCacheServerWorker::lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, EventList& event_list) const
    {
        return false;
    }

    bool CoveredCacheServerWorker::redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, EventList& event_list) const
    {
        return false;
    }

    // (2.2) Update content directory information

    bool CoveredCacheServerWorker::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, EventList& event_list) const
    {
        return false;
    }

    // (2.3) Process writes and block for MSI protocol

    bool CoveredCacheServerWorker::acquireBeaconWritelock_(const Key& key, LockResult& lock_result, EventList& event_list)
    {
        return false;
    }

    bool CoveredCacheServerWorker::releaseBeaconWritelock_(const Key& key, EventList& event_list)
    {
        return false;
    }

    // (5) Admit uncached objects in local edge cache

    bool CoveredCacheServerWorker::triggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list) const
    {
        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        std::ostringstream oss;
        Util::dumpErrorMsg(instance_name_, "triggerIndependentAdmission_() should NOT be invoked in CoveredCacheServerWorker!");
        exit(1);
        return false;
    }
}
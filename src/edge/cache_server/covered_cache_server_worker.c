#include "edge/cache_server/covered_cache_server_worker.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string CoveredCacheServerWorker::kClassName("CoveredCacheServerWorker");

    CoveredCacheServerWorker::CoveredCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapper* tmp_edgewrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edgewrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edgewrapper_ptr->getNodeIdx();
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
        // TODO: Piggyback candidate victims in current edge node
        return false;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    MessageBase* CoveredCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Piggyback local uncached popularity for key in current edge node (END HERE)

        /*checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Prepare directory lookup request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(directory_lookup_request_ptr != NULL);

        return directory_lookup_request_ptr;*/
    }

    void CoveredCacheServerWorker::processRspToLookupBeaconDirectory_(const DynamicArray& control_response_msg_payload, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, EventList& event_list) const
    {
        // TODO: Process directory lookup response for non-blocking admission placement deployment

        return;
    }

    bool CoveredCacheServerWorker::redirectGetToTarget_(const DirectoryInfo& directory_info, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Update/invaidate expiration-based local directory cache
        // TODO: If local uncached popularity has large change compared with last sync, explicitly sync latest local uncached popularity to beacon node by piggybacking to update aggregated uncached popularity
        return false;
    }

    // (2.2) Update content directory information

    bool CoveredCacheServerWorker::updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // TODO: Piggyback candidate victims in current edge node

        // NOTE: If updateBeaconDirectory_() is admitting a local uncached object, beacon node has already removed local uncached popularities of plaecment nodes from aggregated uncached popularity after placement calculation, and only needs to assert node ID NOT exist in aggregate bitmap and remove the preserved node ID if any -> NO need to piggyback local uncached popularity
        // NOTE: If updateBeaconDirectory_() is evicting a local cached object, beacon node only needs to assert node ID NOT exist in aggregate bitmap -> NO need to piggyback local uncached popularity
        return false;
    }

    // (2.3) Process writes and block for MSI protocol

    bool CoveredCacheServerWorker::acquireBeaconWritelock_(const Key& key, LockResult& lock_result, EventList& event_list, const bool& skip_propagation_latency)
    {
        // TODO: Piggyback candidate victims in current edge node

        // TODO: Piggyback local uncached popularity for key in current edge node
        return false;
    }

    bool CoveredCacheServerWorker::releaseBeaconWritelock_(const Key& key, EventList& event_list, const bool& skip_propagation_latency)
    {
        // TODO: Piggyback candidate victims in current edge node

        // NOTE: NO need to piggyback local uncached popularity for key in current edge node, as it has been done by acquireBeaconWritelock_() (local uncached popularity is NOT changed when processing a single local data request)
        return false;
    }

    // (5) Admit uncached objects in local edge cache

    bool CoveredCacheServerWorker::tryToTriggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list, const bool& skip_propagation_latency) const
    {
        // std::ostringstream oss;
        // Util::dumpErrorMsg(instance_name_, "tryToTriggerIndependentAdmission_() should NOT be invoked in CoveredCacheServerWorker!");
        // exit(1);

        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        bool is_finish = false;
        return is_finish;
    }
}
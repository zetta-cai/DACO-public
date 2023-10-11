#include "edge/cache_server/basic_cache_server_worker.h"

#include <assert.h>
#include <sstream>

#include "message/control_message.h"
#include "message/data_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCacheServerWorker::kClassName("BasicCacheServerWorker");

    BasicCacheServerWorker::BasicCacheServerWorker(CacheServerWorkerParam* cache_server_worker_param_ptr) : CacheServerWorkerBase(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        EdgeWrapper* tmp_edgewrapper_ptr = cache_server_worker_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edgewrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edgewrapper_ptr->getNodeIdx();
        uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();

        // Differentiate BasicCacheServerWorker in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << local_cache_server_worker_idx;
        instance_name_ = oss.str();
    }

    BasicCacheServerWorker::~BasicCacheServerWorker() {}

    // (1.1) Access local edge cache

    // (1.2) Access cooperative edge cache to fetch data from neighbor edge nodes

    bool BasicCacheServerWorker::lookupLocalDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    bool BasicCacheServerWorker::needLookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // NOTE: basic cooperative edge caching does NOT have directory metadata cache, so it always needs to send request to lookup remote directory information at beacon node
        bool need_lookup_beacon_directory = true;
        return need_lookup_beacon_directory;
    }

    MessageBase* BasicCacheServerWorker::getReqToLookupBeaconDirectory_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Prepare directory lookup request to check directory information in beacon node
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* directory_lookup_request_ptr = new DirectoryLookupRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(directory_lookup_request_ptr != NULL);

        return directory_lookup_request_ptr;
    }

    void BasicCacheServerWorker::processRspToLookupBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kDirectoryLookupResponse);

        // Get directory information from the control response message
        const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(control_response_ptr);
        is_being_written = directory_lookup_response_ptr->isBeingWritten();
        is_valid_directory_exist = directory_lookup_response_ptr->isValidDirectoryExist();
        directory_info = directory_lookup_response_ptr->getDirectoryInfo();

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return;
    }

    MessageBase* BasicCacheServerWorker::getReqToRedirectGet_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Prepare redirected get request to fetch data from other edge nodes
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* redirected_get_request_ptr = new RedirectedGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(redirected_get_request_ptr != NULL);

        return redirected_get_request_ptr;
    }

    void BasicCacheServerWorker::processRspToRedirectGet_(MessageBase* redirected_response_ptr, Value& value, Hitflag& hitflag) const
    {
        assert(redirected_response_ptr != NULL);
        assert(redirected_response_ptr->getMessageType() == MessageType::kRedirectedGetResponse);

        // Get value and hitflag from redirected response message
        const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(redirected_response_ptr);
        value = redirected_get_response_ptr->getValue();
        Hitflag hitflag = redirected_get_response_ptr->getHitflag();

        return;
    }

    // (1.4) Update invalid cached objects in local edge cache

    bool BasicCacheServerWorker::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_and_invalid = false;
        if (value.isDeleted()) // value is deleted
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->removeIfInvalidForGetrsp(key, affect_victim_tracker); // remove will NOT trigger eviction
        }
        else // non-deleted value
        {
            is_local_cached_and_invalid = tmp_edge_wrapper_ptr->getEdgeCachePtr()->updateIfInvalidForGetrsp(key, value, affect_victim_tracker); // update may trigger eviction (see CacheServerWorkerBase::processLocalGetRequest_)
        }
        UNUSED(affect_victim_tracker); // ONLY for COVERED
        
        return is_local_cached_and_invalid;
    }

    // (2.1) Acquire write lock and block for MSI protocol

    bool BasicCacheServerWorker::acquireLocalWritelock_(const Key& key, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_source_cached = false;
        lock_result = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->acquireLocalWritelockByCacheServer(key, current_edge_idx, all_dirinfo, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);
        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToAcquireBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* acquire_writelock_request_ptr = new AcquireWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(acquire_writelock_request_ptr != NULL);

        return acquire_writelock_request_ptr;
    }

    void BasicCacheServerWorker::processRspToAcquireBeaconWritelock_(MessageBase* control_response_ptr, LockResult& lock_result) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kAcquireWritelockResponse);

        // Get result of acquiring write lock
        const AcquireWritelockResponse* const acquire_writelock_response_ptr = static_cast<const AcquireWritelockResponse*>(control_response_ptr);
        lock_result = acquire_writelock_response_ptr->getLockResult();

        return;
    }

    // (2.3) Update cached objects in local edge cache

    bool BasicCacheServerWorker::updateLocalEdgeCache_(const Key& key, const Value& value) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_udpate = tmp_edge_wrapper_ptr->getEdgeCachePtr()->update(key, value, affect_victim_tracker);
        UNUSED(affect_victim_tracker); // ONLY for COVERED        

        return is_local_cached_after_udpate;
    }

    bool BasicCacheServerWorker::removeLocalEdgeCache_(const Key& key) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool affect_victim_tracker = false;
        bool is_local_cached_after_remove = tmp_edge_wrapper_ptr->getEdgeCachePtr()->remove(key, affect_victim_tracker);
        UNUSED(affect_victim_tracker); // ONLY for COVERED

        return is_local_cached_after_remove;
    }

    // (2.4) Release write lock for MSI protocol

    bool BasicCacheServerWorker::releaseLocalWritelock_(const Key& key, const Value& value, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usgae, EventList& event_list, const bool& skip_propagation_latency)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(key, current_edge_idx, current_directory_info, is_source_cached);

        UNUSED(value);
        UNUSED(total_bandwidth_usgae);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    MessageBase* BasicCacheServerWorker::getReqToReleaseBeaconWritelock_(const Key& key, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        MessageBase* release_writelock_request_ptr = new ReleaseWritelockRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_, skip_propagation_latency);
        assert(release_writelock_request_ptr != NULL);

        return;
    }

    bool BasicCacheServerWorker::processRspToReleaseBeaconWritelock_(MessageBase* control_response_ptr, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        assert(control_response_ptr != NULL);
        assert(control_response_ptr->getMessageType() == MessageType::kReleaseWritelockResponse);

        bool is_finish = false;

        // Do nothing for ReleaseWritelockResponse

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    // (3) Process redirected requests

    void BasicCacheServerWorker::processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const
    {
        // Get key from redirected get request
        assert(redirected_request_ptr != NULL);
        assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Access local edge cache for redirected get request
        is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, value);
        is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);
        
        return;
    }

    MessageBase* BasicCacheServerWorker::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        // Get key from redirected get request
        assert(redirected_request_ptr != NULL);
        assert(redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        const bool skip_propagation_latency = redirected_get_request_ptr->isSkipPropagationLatency();

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Prepare redirected get response
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->getEdgeCacheServerRecvreqSourceAddr();
        MessageBase* redirected_get_response_ptr = new RedirectedGetResponse(tmp_key, value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        assert(redirected_get_response_ptr != NULL);

        return redirected_get_response_ptr;
    }

    // (4.1) Admit uncached objects in local edge cache

    // (4.2) Admit content directory information
}
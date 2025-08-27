#include "edge/covered_edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "cache/covered_cache_custom_func_param.h"
#include "common/util.h"
#include "edge/covered_edge_custom_func_param.h"
#include "message/control_message.h"
#include "message/data_message.h"

namespace covered
{
    const std::string CoveredEdgeWrapper::kClassName("CoveredEdgeWrapper");

    // NOTE: client-edge/cross-edge/edge-cloud propagation latency from CLI is a single trip latency, which should be counted twice within an RTT for weight tuner
    CoveredEdgeWrapper::CoveredEdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint64_t& local_uncached_capacity_bytes, const uint64_t& local_uncached_lru_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const CLILatencyInfo& cli_latency_info, const uint32_t& topk_edgecnt, const std::string& realnet_option, const std::string& realnet_expname, const std::vector<uint32_t> _p2p_latency_array) : EdgeWrapperBase(cache_name, capacity_bytes, edge_idx, edgecnt, hash_name, keycnt, local_uncached_capacity_bytes, local_uncached_lru_bytes, percacheserver_workercnt, peredge_synced_victimcnt, peredge_monitored_victimsetcnt, popularity_aggregation_capacity_bytes, popularity_collection_change_ratio, cli_latency_info, topk_edgecnt, realnet_option, realnet_expname, _p2p_latency_array), topk_edgecnt_for_placement_(topk_edgecnt), weight_tuner_(edge_idx, edgecnt, cli_latency_info.getPropagationLatencyClientedgeAvgUs(), cli_latency_info.getPropagationLatencyCrossedgeAvgUs(), cli_latency_info.getPropagationLatencyEdgecloudAvgUs(), _p2p_latency_array), covered_cache_manager_ptr_(NULL)
    {
        assert(cache_name == Util::COVERED_CACHE_NAME);

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Get source address of beacon server recvreq for non-blocking placement deployment
        const bool is_private_edge_ipstr = false; // NOTE: cross-edge communication for non-blocking placement deployment uses public IP address
        const bool is_launch_edge = true; // The beacon server belongs to the logical edge node launched in the current physical machine
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_private_edge_ipstr, is_launch_edge);
        uint16_t edge_beacon_server_recvreq_port = Util::getEdgeBeaconServerRecvreqPort(edge_idx, edgecnt);
        edge_beacon_server_recvreq_source_addr_for_placement_ = NetworkAddr(edge_ipstr, edge_beacon_server_recvreq_port);

        // Get destination address towards the corresponding cloud recvreq for non-blocking placement deployment
        const bool is_private_cloud_ipstr = false; // NOTE: edge communicates with cloud via public IP address
        const bool is_launch_cloud = false; // Just connect cloud by the logical edge node instead of launching the cloud
        std::string cloud_ipstr = Config::getCloudIpstr(is_private_cloud_ipstr, is_launch_cloud);
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        corresponding_cloud_recvreq_dst_addr_for_placement_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // Allocate covered cache manager
        covered_cache_manager_ptr_ = new CoveredCacheManager(edge_idx, edgecnt, peredge_synced_victimcnt, peredge_monitored_victimsetcnt, popularity_aggregation_capacity_bytes, popularity_collection_change_ratio, topk_edgecnt);
        assert(covered_cache_manager_ptr_ != NULL);

        // Load snapshot to initialize edge node for real-net exps if necessary
        tryToInitializeForRealnet_();
    }
        
    CoveredEdgeWrapper::~CoveredEdgeWrapper()
    {
        #ifdef DEBUG_EDGE_WRAPPER_BASE
        uint64_t cache_manager_size = covered_cache_manager_ptr_->getSizeForCapacity();
        uint64_t weight_tuner_size = weight_tuner_.getSizeForCapacity();
        std::ostringstream oss;
        oss << "cache_manager_size: " << cache_manager_size << ", weight_tuner_size: " << weight_tuner_size;
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        // Release covered cache manager
        assert(covered_cache_manager_ptr_ != NULL);
        delete covered_cache_manager_ptr_;
        covered_cache_manager_ptr_ = NULL;
    }

    // (1) Const getters

    uint32_t CoveredEdgeWrapper::getTopkEdgecntForPlacement() const
    {
        return topk_edgecnt_for_placement_;
    }

    CoveredCacheManager* CoveredEdgeWrapper::getCoveredCacheManagerPtr() const
    {
        // NOTE: non-COVERED caches should NOT call this function
        assert(covered_cache_manager_ptr_ != NULL);
        return covered_cache_manager_ptr_;
    }

    WeightTuner& CoveredEdgeWrapper::getWeightTunerRef()
    {
        return weight_tuner_;
    }

    // (2) Utility functions

    uint64_t CoveredEdgeWrapper::getSizeForCapacity() const
    {
        uint64_t size = EdgeWrapperBase::getSizeForCapacity();

        uint64_t cache_manager_size = covered_cache_manager_ptr_->getSizeForCapacity();
        uint64_t weight_tuner_size = weight_tuner_.getSizeForCapacity();
        size += (cache_manager_size + weight_tuner_size);

        return size;
    }

    // (3) Invalidate and unblock for MSI protocol

    MessageBase* CoveredEdgeWrapper::getInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;

        // Prepare victim syncset for piggybacking-based victim synchronization
        assert(dst_edge_idx_for_compression != edge_idx);
        VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, getCacheMarginBytes());

        // Prepare invalidation request to invalidate the cache copy
        // invalidation_request_ptr = new CoveredInvalidationRequest(key, victim_syncset, edge_idx, recvrsp_source_addr, extra_common_msghdr);
        MessageBase* invalidation_request_ptr = new CoveredInvalidationRequest(key, edge_idx, recvrsp_source_addr, extra_common_msghdr);
        assert(invalidation_request_ptr != NULL);

        return invalidation_request_ptr;
    }

    void CoveredEdgeWrapper::processInvalidationResponse_(MessageBase* invalidation_response_ptr) const
    {
        assert(invalidation_response_ptr != NULL);
        assert(invalidation_response_ptr->getMessageType() == MessageType::kCoveredInvalidationResponse);

        // const CoveredInvalidationResponse* covered_invalidation_response_ptr = static_cast<const CoveredInvalidationResponse*>(invalidation_response_ptr);
        // const uint32_t source_edge_idx = covered_invalidation_response_ptr->getSourceIndex();

        // // Victim synchronization
        // const VictimSyncset& neighbor_victim_syncset = covered_invalidation_response_ptr->getVictimSyncsetRef();
        // updateCacheManagerForNeighborVictimSyncsetInternal_(source_edge_idx, neighbor_victim_syncset);

        return;
    }

    MessageBase* CoveredEdgeWrapper::getFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;
        
        // Prepare victim syncset for piggybacking-based victim synchronization
        assert(dst_edge_idx_for_compression != edge_idx);
        VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, getCacheMarginBytes());

        // Prepare finish block request to finish blocking for writes in all closest edge nodes
        MessageBase* finish_block_request_ptr = new CoveredFinishBlockRequest(key, victim_syncset, edge_idx, recvrsp_source_addr, extra_common_msghdr);
        assert(finish_block_request_ptr != NULL);

        return finish_block_request_ptr;
    }

    void CoveredEdgeWrapper::processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const
    {
        assert(finish_block_response_ptr != NULL);
        assert(finish_block_response_ptr->getMessageType() == MessageType::kCoveredFinishBlockResponse);

        const CoveredFinishBlockResponse* covered_finish_block_response_ptr = static_cast<const CoveredFinishBlockResponse*>(finish_block_response_ptr);
        const uint32_t source_edge_idx = covered_finish_block_response_ptr->getSourceIndex();

        // Victim synchronization
        const VictimSyncset& neighbor_victim_syncset = covered_finish_block_response_ptr->getVictimSyncsetRef();
        updateCacheManagerForNeighborVictimSyncsetInternal_(source_edge_idx, neighbor_victim_syncset);

        return;
    }

    // (5) Other utilities

    void CoveredEdgeWrapper::checkPointers_() const
    {
        EdgeWrapperBase::checkPointers_();

        assert(covered_cache_manager_ptr_ != NULL);

        return;
    }

    // (6) Common utility functions

    // (6.1) For local edge cache access

    bool CoveredEdgeWrapper::getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value, bool& is_tracked_before_fetch_value) const
    {
        // Cache server gets local edge cache for foreground local data requests
        // Beacon server gets local edge cache for background non-blocking data fetching

        checkPointers_();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_and_valid = edge_cache_ptr_->get(key, is_redirected, value, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update by checking affect_victim_tracker for COVERED
        updateCacheManagerForLocalSyncedVictimsInternal_(affect_victim_tracker);

        // Get is tracked by local uncached metadata before fetching value from neighbor/cloud
        // TODO: is_tracked_before_fetch_value can be set by edge_cache_ptr_->get() for less processing overhead
        is_tracked_before_fetch_value = false;
        GetCollectedPopularityParam tmp_param(key);
        edge_cache_ptr_->constCustomFunc(GetCollectedPopularityParam::FUNCNAME, &tmp_param);
        is_tracked_before_fetch_value = tmp_param.getCollectedPopularityConstRef().isTracked();

        return is_local_cached_and_valid;
    }

    // (6.2) For local directory admission

    void CoveredEdgeWrapper::admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        // Cache server admits local directory for foreground independent admission
        // Beacon server admits local directory for background non-blocking placement notification at local/remote beacon edge node

        checkPointers_();

        uint32_t current_edge_idx = getNodeIdx();
        const bool is_admit = true; // Admit content directory
        MetadataUpdateRequirement metadata_update_requirement;
        cooperation_wrapper_ptr_->updateDirectoryTable(key, current_edge_idx, is_admit, directory_info, is_being_written, is_neighbor_cached, metadata_update_requirement);

        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation -> while here NOT need piggyacking-based popularity collection and victim synchronization for local directory update

        // Issue metadata update request if necessary, update victim dirinfo, and clear preserved edgeset after local directory admission for COVERED
        afterDirectoryAdmissionHelperInternal_(key, current_edge_idx, metadata_update_requirement, directory_info, extra_common_msghdr);

        // NOTE: NO need to clear directory metadata cache, as key is a local-beaconed uncached object

        return;
    }

    // (7) Method-specific functions

    void CoveredEdgeWrapper::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        assert(func_param_ptr != NULL);

        if (funcname == UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME)
        {
            UpdateCacheManagerForLocalSyncedVictimsFuncParam* tmp_param_ptr = static_cast<UpdateCacheManagerForLocalSyncedVictimsFuncParam*>(func_param_ptr);
            updateCacheManagerForLocalSyncedVictimsInternal_(tmp_param_ptr->isAffectVictimTracker());
        }
        else if (funcname == UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME)
        {
            UpdateCacheManagerForNeighborVictimSyncsetFuncParam* tmp_param_ptr = static_cast<UpdateCacheManagerForNeighborVictimSyncsetFuncParam*>(func_param_ptr);
            updateCacheManagerForNeighborVictimSyncsetInternal_(tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getVictimSyncsetConstRef());
        }
        else if (funcname == NonblockDataFetchForPlacementFuncParam::FUNCNAME)
        {
            NonblockDataFetchForPlacementFuncParam* tmp_param_ptr = static_cast<NonblockDataFetchForPlacementFuncParam*>(func_param_ptr);
            nonblockDataFetchForPlacementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getExtraCommonMsghdr(), tmp_param_ptr->isSenderBeacon(), tmp_param_ptr->needHybridFetchingRef());
        }
        else if (funcname == NonblockDataFetchFromCloudForPlacementFuncParam::FUNCNAME)
        {
            NonblockDataFetchFromCloudForPlacementFuncParam* tmp_param_ptr = static_cast<NonblockDataFetchFromCloudForPlacementFuncParam*>(func_param_ptr);
            nonblockDataFetchFromCloudForPlacementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == NonblockNotifyForPlacementFuncParam::FUNCNAME)
        {
            NonblockNotifyForPlacementFuncParam* tmp_param_ptr = static_cast<NonblockNotifyForPlacementFuncParam*>(func_param_ptr);
            nonblockNotifyForPlacementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getValueConstRef(), tmp_param_ptr->getBestPlacementEdgesetConstRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == ProcessMetadataUpdateRequirementFuncParam::FUNCNAME)
        {
            ProcessMetadataUpdateRequirementFuncParam* tmp_param_ptr = static_cast<ProcessMetadataUpdateRequirementFuncParam*>(func_param_ptr);
            processMetadataUpdateRequirementInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getMetadataUpdateRequirementConstRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == CalcLocalCachedRewardFuncParam::FUNCNAME)
        {
            CalcLocalCachedRewardFuncParam* tmp_param_ptr = static_cast<CalcLocalCachedRewardFuncParam*>(func_param_ptr);

            Reward& tmp_local_cached_reward_ref = tmp_param_ptr->getRewardRef();
            tmp_local_cached_reward_ref = calcLocalCachedRewardInternal_(tmp_param_ptr->getLocalCachedPopularityConstRef(), tmp_param_ptr->getRedirectedCachedPopularityConstRef(), tmp_param_ptr->isLastCopies());
        }
        else if (funcname == CalcLocalUncachedRewardFuncParam::FUNCNAME)
        {
            CalcLocalUncachedRewardFuncParam* tmp_param_ptr = static_cast<CalcLocalUncachedRewardFuncParam*>(func_param_ptr);

            Reward& tmp_local_uncached_reward_ref = tmp_param_ptr->getRewardRef();
            tmp_local_uncached_reward_ref = calcLocalUncachedRewardInternal_(tmp_param_ptr->getLocalUncachedPopularityConstRef(), tmp_param_ptr->isGlobalCached(), tmp_param_ptr->getRedirectedUncachedPopularityConstRef());
        }
        else if (funcname == AfterDirectoryLookupHelperFuncParam::FUNCNAME)
        {
            AfterDirectoryLookupHelperFuncParam* tmp_param_ptr = static_cast<AfterDirectoryLookupHelperFuncParam*>(func_param_ptr);

            bool& tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = afterDirectoryLookupHelperInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getCollectedPopularityConstRef(), tmp_param_ptr->isGlobalCached(), tmp_param_ptr->isSourceCached(), tmp_param_ptr->getBestPlacementEdgesetRef(), tmp_param_ptr->needHybridFetchingRef(), tmp_param_ptr->getFastPathHintPtr(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == AfterDirectoryAdmissionHelperFuncParam::FUNCNAME)
        {
            AfterDirectoryAdmissionHelperFuncParam* tmp_param_ptr = static_cast<AfterDirectoryAdmissionHelperFuncParam*>(func_param_ptr);
            afterDirectoryAdmissionHelperInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getMetadataUpdateRequirementConstRef(), tmp_param_ptr->getDirectoryInfoConstRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == AfterDirectoryEvictionHelperFuncParam::FUNCNAME)
        {
            AfterDirectoryEvictionHelperFuncParam* tmp_param_ptr = static_cast<AfterDirectoryEvictionHelperFuncParam*>(func_param_ptr);

            bool& tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = afterDirectoryEvictionHelperInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getMetadataUpdateRequirementConstRef(), tmp_param_ptr->getDirectoryInfoConstRef(), tmp_param_ptr->getCollectedPopularityConstRef(), tmp_param_ptr->isGlobalCached(), tmp_param_ptr->getBestPlacementEdgesetRef(), tmp_param_ptr->needHybridFetchingRef(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr(), tmp_param_ptr->isBackground());
        }
        else if (funcname == AfterWritelockAcquireHelperFuncParam::FUNCNAME)
        {
            AfterWritelockAcquireHelperFuncParam* tmp_param_ptr = static_cast<AfterWritelockAcquireHelperFuncParam*>(func_param_ptr);

            bool& tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = afterWritelockAcquireHelperInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getCollectedPopularityConstRef(), tmp_param_ptr->isGlobalCached(), tmp_param_ptr->isSourceCached(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else if (funcname == AfterWritelockReleaseHelperFuncParam::FUNCNAME)
        {
            AfterWritelockReleaseHelperFuncParam* tmp_param_ptr = static_cast<AfterWritelockReleaseHelperFuncParam*>(func_param_ptr);

            bool& tmp_is_finish_ref = tmp_param_ptr->isFinishRef();
            tmp_is_finish_ref = afterWritelockReleaseHelperInternal_(tmp_param_ptr->getKeyConstRef(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getCollectedPopularityConstRef(), tmp_param_ptr->isSourceCached(), tmp_param_ptr->getBestPlacementEdgesetRef(), tmp_param_ptr->needHybridFetchingRef(), tmp_param_ptr->getRecvrspSocketServerPtr(), tmp_param_ptr->getRecvrspSourceAddrConstRef(), tmp_param_ptr->getTotalBandwidthUsageRef(), tmp_param_ptr->getEventListRef(), tmp_param_ptr->getExtraCommonMsghdr());
        }
        else
        {
            std::ostringstream oss;
            oss << funcname << " is not supported in constCustomFunc()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // NOTE: the followings are covered-specific utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

    // (7.1) For victim synchronization

    void CoveredEdgeWrapper::updateCacheManagerForLocalSyncedVictimsInternal_(const bool& affect_victim_tracker) const
    {
        checkPointers_();

        // Get local edge margin bytes
        uint64_t local_cache_margin_bytes = getCacheMarginBytes();

        if (affect_victim_tracker) // Need to update victim cacheinfos and dirinfos of local synced victims besides local cache margin bytes
        {
            // Get victim cacheinfos of local synced victims for the current edge node
            GetLocalSyncedVictimCacheinfosParam tmp_param;
            edge_cache_ptr_->constCustomFunc(GetLocalSyncedVictimCacheinfosParam::FUNCNAME, &tmp_param); // NOTE: victim cacheinfos from local edge cache MUST be complete

            // Update local synced victims for the current edge node
            covered_cache_manager_ptr_->updateVictimTrackerForLocalSyncedVictims(local_cache_margin_bytes, tmp_param.getVictimCacheinfosConstRef(), cooperation_wrapper_ptr_);
        }
        else // ONLY update local cache margin bytes
        {
            // Update local cache margin bytes
            covered_cache_manager_ptr_->updateVictimTrackerForLocalCacheMarginBytes(local_cache_margin_bytes);
        }

        return;
    }

    void CoveredEdgeWrapper::updateCacheManagerForNeighborVictimSyncsetInternal_(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset) const
    {
        // Foreground victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        covered_cache_manager_ptr_->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, cooperation_wrapper_ptr_);

        return;
    }

    // (7.2) For non-blocking placement deployment (ONLY invoked by beacon edge node)

    //bool CoveredEdgeWrapper::nonblockDataFetchForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const ExtraCommonMsghdr& extra_common_msghdr, const bool& sender_is_beacon, bool& need_hybrid_fetching) const
    void CoveredEdgeWrapper::nonblockDataFetchForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const ExtraCommonMsghdr& extra_common_msghdr, const bool& sender_is_beacon, bool& need_hybrid_fetching) const
    {
        checkPointers_();
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        need_hybrid_fetching = false;

        // Check local edge cache in local/remote beacon node first
        // NOTE: NOT update aggregated uncached popularity to avoid recursive placement calculation even if local uncached popularity is cached
        Value value;
        const bool is_redirected = !sender_is_beacon; // Approximately treat local control message as local cache access if sender is beacon, yet treat remote control message as remote cache access if sender is NOT beacon
        bool unused_is_tracked_before_fetch_value = false;
        bool is_local_cached_and_valid = getLocalEdgeCache_(key, is_redirected, value, unused_is_tracked_before_fetch_value);
        UNUSED(unused_is_tracked_before_fetch_value);

        if (is_local_cached_and_valid) // Directly get valid value from local edge cache in local/remote beacon node
        {
            // Perform non-blocking placement notification for local data fetching
            nonblockNotifyForPlacementInternal_(key, value, best_placement_edgeset, extra_common_msghdr);
        }
        else // No valid value in local edge cache in local/remote beacon node
        {
            if (sender_is_beacon) // Sender and beacon is the same edge node
            {
                // NOTE: we always resort sender for hybrid data fetching if sender and beacon is the same edge node, due to zero extra latency between sender and beacon!
                need_hybrid_fetching = true;
            }
            else // Sender and beacon are two different dge nodes
            {
                // Check if exist valid dirinfo to fetch data from neighbor
                uint32_t current_edge_idx = getNodeIdx();
                bool is_being_written = false;
                bool is_valid_directory_exist = false;
                DirectoryInfo directory_info;
                bool is_source_cached = false;
                bool is_global_cached = cooperation_wrapper_ptr_->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached); // NOTE: local/remote beacon node lookup its own local directory table (= current is beacon for cache server)
                UNUSED(is_source_cached);
                UNUSED(is_global_cached);

                if (is_valid_directory_exist) // A neighbor edge node has cached the object
                {
                    assert(!is_being_written); // Being-written object CANNOT have any valid dirinfo

                    // Send CoveredBgfetchRedirectedGetRequest to the neighbor node
                    // NOTE: we use edge_beacon_server_recvreq_source_addr_ as the source address even if invoker is cache server to wait for responses
                    // (i) Although current is beacon for cache server, the role is still beacon node, which should be responsible for non-blocking placement deployment. And we don't want to resort cache server worker, which may degrade KV request processing
                    // (ii) Although we may wait for responses, beacon server is blocking for recvreq port and we don't want to introduce another blocking for recvrsp port
                    const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(directory_info.getTargetEdgeIdx(), getCacheMarginBytes());
                    CoveredBgfetchRedirectedGetRequest* covered_placement_redirected_get_request_ptr = new CoveredBgfetchRedirectedGetRequest(key, victim_syncset, best_placement_edgeset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, extra_common_msghdr); // NOTE: NO need to assign a new msg seqnum by edge for COVERED background redirected get request, as we do NOT wait for the correpsonding response by a timeout-and-retry mechanism (i.e., duplicate responses do NOT affect)
                    assert(covered_placement_redirected_get_request_ptr != NULL);
                    NetworkAddr target_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(directory_info); // Send to cache server of the target edge node for cache server worker
                    bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_placement_redirected_get_request_ptr, target_edge_cache_server_recvreq_dst_addr);
                    assert(is_successful);
                    covered_placement_redirected_get_request_ptr = NULL; // NOTE: covered_placement_redirected_get_request_ptr will be released by edge-to-edge propagation simulator

                    // NOTE: CoveredBgfetchRedirectedGetResponse will be processed by covered beacon server in the current edge node (current edge node is a remote beacon node for neighbor edge node, as neighbor MUST NOT be a different edge node for request redirection)
                }
                else // No valid cache copy in edge layer
                {
                    // We need hybrid fetching to resort the sender to fetch data from cloud for reducing edge-cloud bandwidth usage
                    // NOTE: extra latency between sender and beacon is limited compared with edge-cloud latency (i.e., w1 - w2 / w1)
                    need_hybrid_fetching = true;
                }
            }
        }

        return;
    }

    void CoveredEdgeWrapper::nonblockDataFetchFromCloudForPlacementInternal_(const Key& key, const Edgeset& best_placement_edgeset, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        // NOTE: as we have replied the sender without hybrid data fetching before, beacon server directly fetches data from cloud by itself here in a non-blocking manner (this is a corner case, as valid dirinfo has cooperative hit in most time)

        // Send CoveredBgfetchGlobalGetRequest to cloud
        // NOTE: we use edge_beacon_server_recvreq_source_addr_ as the source address even if the invoker (i.e., beacon server) is waiting for global responses
        // (i) Although wait for global responses, beacon server is blocking for recvreq port and we don't want to introduce another blocking for recvrsp port
        uint32_t current_edge_idx = getNodeIdx();
        CoveredBgfetchGlobalGetRequest* covered_placement_global_get_request_ptr = new CoveredBgfetchGlobalGetRequest(key, best_placement_edgeset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, extra_common_msghdr); // NOTE: NO need to assign a new msg seqnum by edge for COVERED background global get request, as we do NOT wait for the correpsonding response by a timeout-and-retry mechanism (i.e., duplicate responses do NOT affect)
        assert(covered_placement_global_get_request_ptr != NULL);
        // Push the global request into edge-to-cloud propagation simulator to cloud
        bool is_successful = edge_tocloud_propagation_simulator_param_ptr_->push(covered_placement_global_get_request_ptr, corresponding_cloud_recvreq_dst_addr_for_placement_);
        assert(is_successful);
        covered_placement_global_get_request_ptr = NULL; // NOTE: covered_placement_global_get_request_ptr will be released by edge-to-cloud propagation simulator

        // NOTE: CoveredBgfetchRedirectedGetResponse will be processed by covered beacon server in the current edge node

        return;
    }

    //bool CoveredEdgeWrapper::nonblockNotifyForPlacementInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const ExtraCommonMsghdr& extra_common_msghdr) const
    void CoveredEdgeWrapper::nonblockNotifyForPlacementInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        //bool is_finish = false;
        //BandwidthUsage total_bandwidth_usage;
        //EventList event_list;
        //const bool is_background = true;

        // Check writelock for validity of cache placement
        bool is_being_written = cooperation_wrapper_ptr_->isBeingWritten(key);
        bool is_valid = !is_being_written;

        // Send placement notification for each non-local edge node in best_placement_edgeset in a non-blocking manner
        const uint32_t current_edge_idx = getNodeIdx();
        std::unordered_set<uint32_t>::const_iterator edgeset_const_iter_for_local_notification = best_placement_edgeset.end();
        for (std::unordered_set<uint32_t>::const_iterator edgeset_const_iter_for_remote_notification = best_placement_edgeset.begin(); edgeset_const_iter_for_remote_notification != best_placement_edgeset.end(); edgeset_const_iter_for_remote_notification++)
        {
            const uint32_t& tmp_edge_idx = *edgeset_const_iter_for_remote_notification;
            if (tmp_edge_idx == current_edge_idx) // Skip local edge node
            {
                edgeset_const_iter_for_local_notification = edgeset_const_iter_for_remote_notification;
                continue;
            }

            // Send CoveredBgplacePlacementNotifyRequest to remote edge node
            // NOTE: source addr will NOT be used by placement processor of remote edge node due to without explicit notification ACK (we use directory update request with is_admit = true as the implicit ACK for placement notification)
            const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(tmp_edge_idx, getCacheMarginBytes());
            CoveredBgplacePlacementNotifyRequest* covered_placement_notify_request_ptr = new CoveredBgplacePlacementNotifyRequest(key, value, is_valid, victim_syncset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, extra_common_msghdr); // NOTE: NO need to assign a new msg seqnum by edge for COVERED background placement notification request, as we do NOT wait for the correpsonding response by a timeout-and-retry mechanism (i.e., duplicate responses do NOT affect)
            assert(covered_placement_notify_request_ptr != NULL);

            // Push the global request into edge-to-edge propagation simulator to the remote edge node
            NetworkAddr remote_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(DirectoryInfo(tmp_edge_idx)); // Send to cache server of the target edge node for cache server placement processor
            bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_placement_notify_request_ptr, remote_edge_cache_server_recvreq_dst_addr);
            assert(is_successful);
            covered_placement_notify_request_ptr = NULL; // NOTE: covered_placement_notify_request_ptr will be released by edge-to-edge propagation simulator
        }

        // Local placement notification if necessary
        if (edgeset_const_iter_for_local_notification != best_placement_edgeset.end()) // If current edge node is also in best_placement_edgeset
        {
            // Current edge node MUST be the beacon node for the given key
            MYASSERT(currentIsBeacon(key));

            // Admit local directory information
            // NOTE: NO need to update aggregated uncached popularity due to admitting a cached object
            // NOTE: we cannot optimistically admit valid object into local edge cache first before admiting local dirinfo, as clients may get incorrect value if key is being written
            bool tmp_is_being_written = false;
            bool tmp_is_neighbor_cached = false; // NOTE: we directly use coooperation wrapper to get is_neighbor_cached, as target edge node for local placement notification MUST be beacon here
            admitLocalDirectory_(key, DirectoryInfo(current_edge_idx), tmp_is_being_written, tmp_is_neighbor_cached, extra_common_msghdr); // Local directory update for local placement notification
            if (tmp_is_being_written) // Double-check is_being_written to udpate is_valid if necessary
            {
                // NOTE: ONLY update is_valid if tmp_is_being_written is true; if tmp_is_being_written is false (i.e., key is NOT being written now), we still keep original is_valid, as the value MUST be stale if is_being_written was true before
                is_being_written = true;
                is_valid = false;
            }

            // NOTE: we need to notify placement processor of the current local/remote beacon edge node for non-blocking placement deployment of local placement notification to avoid blocking subsequent placement calculation (similar as CoveredCacheServer::notifyBeaconForPlacementAfterHybridFetchInternal_() invoked by sender edge node)

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker / beacon server for subsequent placement calculation
            bool is_successful = getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, tmp_is_neighbor_cached, is_valid, extra_common_msghdr));
            assert(is_successful);

            /* (OBSOLETE for non-blocking placement deployment)
            // Perform cache admission for local edge cache (equivalent to local placement notification)
            admitLocalEdgeCache_(key, value, is_valid);

            // Perform background cache eviction if necessary in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
            // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
            is_finish = evictForCapacity_(key, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr, is_background);
            */
        }

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        //edge_background_counter_for_beacon_server_.updateBandwidthUsgae(total_bandwidth_usage);
        //edge_background_counter_for_beacon_server_.addEvents(event_list);

        //return is_finish;

        return;
    }

    // (7.3) For beacon-based cached metadata update (non-blocking notification-based)
    void CoveredEdgeWrapper::processMetadataUpdateRequirementInternal_(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        checkPointers_();
        MYASSERT(currentIsBeacon(key));

        const bool is_from_single_to_multiple = metadata_update_requirement.isFromSingleToMultiple();
        const bool is_from_multiple_to_single = metadata_update_requirement.isFromMultipleToSingle();
        const uint32_t notify_edge_idx = metadata_update_requirement.getNotifyEdgeIdx();
        assert(!(is_from_single_to_multiple && is_from_multiple_to_single)); // Cannot be both true

        const bool need_update = is_from_single_to_multiple || is_from_multiple_to_single;
        if (need_update)
        {
            bool is_neighbor_cached = false;
            if (is_from_single_to_multiple)
            {
                is_neighbor_cached = true; // Enable is_neighbor_cached flag
            }
            else
            {
                is_neighbor_cached = false; // Disable is_neighbor_cached flag
            }

            const uint32_t current_edge_idx = getNodeIdx();
            if (notify_edge_idx == current_edge_idx) // If beacon itself is the first/last cache copy
            {
                // Directly enable/disable is_neighbor_cached flag locally
                UpdateIsNeighborCachedFlagFuncParam tmp_param(key, is_neighbor_cached);
                edge_cache_ptr_->customFunc(UpdateIsNeighborCachedFlagFuncParam::FUNCNAME, &tmp_param);
            }
            else // The first/last cache copy is a neighbor edge node
            {
                // // Prepare victim syncset for piggybacking-based victim synchronization
                // const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(notify_edge_idx, getCacheMarginBytes());

                // Prepare metadata update request
                // NOTE: edge_beacon_server_recvreq_source_addr_for_placement_ will NOT be used by edge cache server metadata update processor due to no response
                // MessageBase* covered_metadata_update_request_ptr = new CoveredMetadataUpdateRequest(key, is_neighbor_cached, victim_syncset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, extra_common_msghdr);
                MessageBase* covered_metadata_update_request_ptr = new CoveredMetadataUpdateRequest(key, is_neighbor_cached, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, extra_common_msghdr);
                assert(covered_metadata_update_request_ptr != NULL);

                // Push the control request into edge-to-edge propagation simulator to the remote edge node
                NetworkAddr remote_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(DirectoryInfo(notify_edge_idx)); // Send to cache server of the remote target edge node for cache server metadata update processor
                bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_metadata_update_request_ptr, remote_edge_cache_server_recvreq_dst_addr);
                assert(is_successful);
                covered_metadata_update_request_ptr = NULL; // NOTE: covered_metadata_update_request_ptr will be released by edge-to-edge propagation simulator
            }
        }

        return;
    }

    // (7.4) Reward calculation for local reward and cache placement (delta reward of admission benefit / eviction cost)

    Reward CoveredEdgeWrapper::calcLocalCachedRewardInternal_(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies) const
    {
        // Get current weight information from weight tuner
        WeightInfo cur_weight_info = weight_tuner_.getWeightInfo();
        const Weight local_hit_weight = cur_weight_info.getLocalHitWeight();
        const Weight cooperative_hit_weight = cur_weight_info.getCooperativeHitWeight();

        // Calculte local reward or eviction cost (i.e., decreased reward)
        Reward tmp_reward = 0.0;
        if (is_last_copies) // Key is the last cache copies
        {
            // Local cache hits become global cache misses for the victim edge node(s), while redirected cache hits become global cache misses for other edge nodes
            tmp_reward = static_cast<Reward>(Util::popularityMultiply(local_hit_weight, local_cached_popularity)) + static_cast<Reward>(Util::popularityMultiply(cooperative_hit_weight, redirected_cached_popularity)); // w1 * local_cached_popularity + w2 * redirected_cached_popularity
        }
        else // Key is NOT the last cache copies
        {
            // Local cache hits become redirected cache hits for the victim edge node(s)
            const Weight w1_minus_w2 = Util::popularityNonegMinus(local_hit_weight, cooperative_hit_weight);
            tmp_reward = static_cast<Reward>(Util::popularityMultiply(w1_minus_w2, local_cached_popularity)); // (w1 - w2) * local_cached_popularity
        }

        return tmp_reward;
    }

    Reward CoveredEdgeWrapper::calcLocalUncachedRewardInternal_(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity) const
    {
        // Get current weight information from weight tuner
        WeightInfo cur_weight_info = weight_tuner_.getWeightInfo();
        const Weight local_hit_weight = cur_weight_info.getLocalHitWeight();
        const Weight cooperative_hit_weight = cur_weight_info.getCooperativeHitWeight();

        // Calculte local reward or admission benefit (i.e., increased reward)
        Reward tmp_reward = 0.0;
        if (is_global_cached) // Key is global cached
        {
            // Redirected cache hits become local cache hits for the placement edge node(s)
            const Weight w1_minus_w2 = Util::popularityNonegMinus(local_hit_weight, cooperative_hit_weight);
            tmp_reward = Util::popularityMultiply(w1_minus_w2, local_uncached_popularity); // w1 - w2

            #ifdef ENABLE_COMPLETE_DUPLICATION_AVOIDANCE_FOR_DEBUG
            tmp_reward = 0.0;
            #endif
        }
        else // Key is NOT global cached
        {
            // Global cache misses become local cache hits for the placement edge node(s)
            Reward tmp_reward0 = Util::popularityMultiply(local_hit_weight, local_uncached_popularity); // w1

            // Global cache misses become redirected cache hits for other edge nodes
            // NOTE: redirected_uncached_popularity MUST be zero for local reward of local uncached objects
            Reward tmp_reward1 = Util::popularityMultiply(cooperative_hit_weight, redirected_uncached_popularity); // w2

            tmp_reward = Util::popularityAdd(tmp_reward0, tmp_reward1);
        }

        return tmp_reward;
    }

    // (7.5) Helper functions after local/remote directory lookup/admission/eviction and writelock acquire/release

    bool CoveredEdgeWrapper::afterDirectoryLookupHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool need_placement_calculation = true;
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr, fast_path_hint_ptr); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node (source = current if sender is beacon)
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // NOTE: (i) if sender is beacon, need_hybrid_fetching with best_placement_edgeset is processed in CacheServerWorkerBase::processLocalGetRequest_(), as we do NOT have value yet when lookuping directory information; (ii) otherwise, need_hybrid_fetching with best_placement_edgeset is processed in CoveredBeaconServer::getRspToLookupLocalDirectory_() to issue corresponding control response message for hybrid data fetching

        return is_finish;
    }

    void CoveredEdgeWrapper::afterDirectoryAdmissionHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        const bool is_admit = true;

        // Issue local/remote metadata update request for beacon-based cached metadata update if necessary (for local/remote directory admission)
        processMetadataUpdateRequirementInternal_(key, metadata_update_requirement, extra_common_msghdr);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim (here we update victim dirinfo in victim tracker before access popularity aggregator)
        covered_cache_manager_ptr_->updateVictimTrackerForLocalBeaconedVictimDirinfo(key, is_admit, directory_info);

        // Clear preserved edge nodes for the given key at the source edge node for metadata releasing after local/remote admission notification
        covered_cache_manager_ptr_->clearPopularityAggregatorForPreservedEdgesetAfterAdmission(key, source_edge_idx);

        return;
    }

    bool CoveredEdgeWrapper::afterDirectoryEvictionHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool is_admit = false;
        bool need_placement_calculation = !is_admit; // MUST be true here for is_admit = false
        if (is_background) // If local/remote directory eviction is triggered by background cache placement
        {
            // NOTE: DISABLE recursive cache placement
            need_placement_calculation = false;
        }
        const bool is_source_cached = false; // NOTE: source edge node MUST NOT cache the object after directory eviction for itself
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        // NOTE: here sender_is_beacon for non-blocking placement deployment focuses on the victim key, which is different from the key triggering eviction, e.g., update invalid/valid value by local get/put, indepdent admission, local placement notification at local/remote beacon edge node, remote placement nofication
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Issue local/remote metadata update request for beacon-based cached metadata update if necessary (for local/remote directory eviction)
        processMetadataUpdateRequirementInternal_(key, metadata_update_requirement, extra_common_msghdr);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        covered_cache_manager_ptr_->updateVictimTrackerForLocalBeaconedVictimDirinfo(key, is_admit, directory_info);

        // NOTE: there may exist a parallel message embedded with a local uncached popularity from the source edge node after the object is evicted, which updates the aggregated uncached popularity before assertNoLocalUncachedPopularity() for directory eviction request
        // (OBSOLETE) NOTE: MUST NO old local uncached popularity for the newly evicted object at the source edge node before popularity aggregation
        // covered_cache_manager_ptr_->assertNoLocalUncachedPopularity(key, source_edge_idx);

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        if (is_background) // Background local/remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
        {
            // NOTE: background local/remote directory eviction MUST NOT need hybrid data fetching due to NO need for placement calculation (we DISABLE recursive cache placement)
            assert(!is_finish);
            assert(best_placement_edgeset.size() == 0);
            assert(!need_hybrid_fetching);
        }

        return is_finish;
    }

    bool CoveredEdgeWrapper::afterWritelockAcquireHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid even after cache placement
        const bool need_placement_calculation = false;
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
    
        // NOTE: local/remote acquire writelock MUST NOT need hybrid data fetching due to NO need for placement calculation (newly-admitted cache copies will still be invalid after cache placement)
        assert(!is_finish); // NO victim fetching due to NO placement calculation
        assert(best_placement_edgeset.size() == 0); // NO placement deployment due to NO placement calculation
        assert(!need_hybrid_fetching); // Also NO hybrid data fetching

        return is_finish;
    }

    bool CoveredEdgeWrapper::afterWritelockReleaseHelperInternal_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool is_global_cached = true; // NOTE: receiving local/remote release writelock means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        const bool need_placement_calculation = true;
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, extra_common_msghdr); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is finished
        }
        
        return is_finish;
    }

    // (8) Evaluation-related functions

    void CoveredEdgeWrapper::dumpEdgeSnapshot_(std::fstream* fs_ptr) const
    {
        checkPointers_();

        assert(fs_ptr != NULL);

        // Dump cache wrapper
        edge_cache_ptr_->dumpCacheSnapshot(fs_ptr);

        // Dump cooperation wrapper
        cooperation_wrapper_ptr_->dumpCooperationSnapshot(fs_ptr);

        // Dump covered cache manager
        covered_cache_manager_ptr_->dumpCoveredCacheManagerSnapshot(fs_ptr);

        // Dump weight tuner
        weight_tuner_.dumpWeightTunerSnapshot(fs_ptr);

        return;
    }

    void CoveredEdgeWrapper::loadEdgeSnapshot_(std::fstream* fs_ptr)
    {
        checkPointers_();

        assert(fs_ptr != NULL);

        // Load cache wrapper
        edge_cache_ptr_->loadCacheSnapshot(fs_ptr);

        // Load cooperation wrapper
        cooperation_wrapper_ptr_->loadCooperationSnapshot(fs_ptr);

        // Load covered cache manager
        covered_cache_manager_ptr_->loadCoveredCacheManagerSnapshot(fs_ptr);

        // Load weight tuner
        weight_tuner_.loadWeightTunerSnapshot(fs_ptr);

        return;
    }
}
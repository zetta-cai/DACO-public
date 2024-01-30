#include "edge/covered_edge_custom_func_param.h"

namespace covered
{
    // (1) For edge beacon server

    // ProcessRspToRedirectGetForPlacementFuncParam

    const std::string ProcessRspToRedirectGetForPlacementFuncParam::kClassName("ProcessRspToRedirectGetForPlacementFuncParam");

    const std::string ProcessRspToRedirectGetForPlacementFuncParam::FUNCNAME("process_rsp_to_redirect_get_for_placement");

    ProcessRspToRedirectGetForPlacementFuncParam::ProcessRspToRedirectGetForPlacementFuncParam(MessageBase* message_ptr) : EdgeCustomFuncParamBase()
    {
        message_ptr_ = message_ptr;
    }

    ProcessRspToRedirectGetForPlacementFuncParam::~ProcessRspToRedirectGetForPlacementFuncParam()
    {}

    MessageBase* ProcessRspToRedirectGetForPlacementFuncParam::getMessagePtr() const
    {
        return message_ptr_;
    }

    // ProcessRspToAccessCloudForPlacementFuncParam

    const std::string ProcessRspToAccessCloudForPlacementFuncParam::kClassName("ProcessRspToAccessCloudForPlacementFuncParam");

    const std::string ProcessRspToAccessCloudForPlacementFuncParam::FUNCNAME("process_rsp_to_access_cloud_for_placement");

    ProcessRspToAccessCloudForPlacementFuncParam::ProcessRspToAccessCloudForPlacementFuncParam(MessageBase* message_ptr) : EdgeCustomFuncParamBase()
    {
        message_ptr_ = message_ptr;
    }

    ProcessRspToAccessCloudForPlacementFuncParam::~ProcessRspToAccessCloudForPlacementFuncParam()
    {}

    MessageBase* ProcessRspToAccessCloudForPlacementFuncParam::getMessagePtr() const
    {
        return message_ptr_;
    }

    // (2) For edge cache server worker

    // TryToTriggerCachePlacementForGetrspFuncParam

    const std::string TryToTriggerCachePlacementForGetrspFuncParam::kClassName("TryToTriggerCachePlacementForGetrspFuncParam");

    const std::string TryToTriggerCachePlacementForGetrspFuncParam::FUNCNAME("try_to_trigger_cache_placement_for_getrsp");

    TryToTriggerCachePlacementForGetrspFuncParam::TryToTriggerCachePlacementForGetrspFuncParam(const Key& key, const Value& value, const CollectedPopularity& collected_popularity_after_fetch_value, const FastPathHint& fast_path_hint, const bool& is_global_cached, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), value_const_ref_(value), collected_popularity_const_ref_(collected_popularity_after_fetch_value), fast_path_hint_const_ref_(fast_path_hint), is_global_cached_(is_global_cached), bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    TryToTriggerCachePlacementForGetrspFuncParam::~TryToTriggerCachePlacementForGetrspFuncParam()
    {}

    const Key& TryToTriggerCachePlacementForGetrspFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Value& TryToTriggerCachePlacementForGetrspFuncParam::getValueConstRef() const
    {
        return value_const_ref_;
    }

    const CollectedPopularity& TryToTriggerCachePlacementForGetrspFuncParam::getCollectedPopularityConstRef() const
    {
        return collected_popularity_const_ref_;
    }

    const FastPathHint& TryToTriggerCachePlacementForGetrspFuncParam::getFastPathHintConstRef() const
    {
        return fast_path_hint_const_ref_;
    }

    bool TryToTriggerCachePlacementForGetrspFuncParam::isGlobalCached() const
    {
        return is_global_cached_;
    }

    BandwidthUsage& TryToTriggerCachePlacementForGetrspFuncParam::getTotalBandwidthUsageRef() const
    {
        return bandwidth_usage_ref_;
    }

    EventList& TryToTriggerCachePlacementForGetrspFuncParam::getEventListRef() const
    {
        return event_list_ref_;
    }

    bool TryToTriggerCachePlacementForGetrspFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool& TryToTriggerCachePlacementForGetrspFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& TryToTriggerCachePlacementForGetrspFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }

    // TryToTriggerPlacementNotificationAfterHybridFetchFuncParam

    const std::string TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::kClassName("TryToTriggerPlacementNotificationAfterHybridFetchFuncParam");

    const std::string TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::FUNCNAME("try_to_trigger_placement_notification_after_hybrid_fetch");

    TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::TryToTriggerPlacementNotificationAfterHybridFetchFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), value_const_ref_(value), best_placement_edgeset_const_ref_(best_placement_edgeset), bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::~TryToTriggerPlacementNotificationAfterHybridFetchFuncParam()
    {}

    const Key& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Value& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::getValueConstRef() const
    {
        return value_const_ref_;
    }

    const Edgeset& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::getBestPlacementEdgesetConstRef() const
    {
        return best_placement_edgeset_const_ref_;
    }

    BandwidthUsage& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::getTotalBandwidthUsageRef() const
    {
        return bandwidth_usage_ref_;
    }

    EventList& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::getEventListRef() const
    {
        return event_list_ref_;
    }

    bool TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }

    // (3) For edge cache server

    // NotifyBeaconForPlacementAfterHybridFetchFuncParam

    const std::string NotifyBeaconForPlacementAfterHybridFetchFuncParam::kClassName("NotifyBeaconForPlacementAfterHybridFetchFuncParam");

    const std::string NotifyBeaconForPlacementAfterHybridFetchFuncParam::FUNCNAME("notify_beacon_for_placement_after_hybrid_fetch");

    NotifyBeaconForPlacementAfterHybridFetchFuncParam::NotifyBeaconForPlacementAfterHybridFetchFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : TryToTriggerPlacementNotificationAfterHybridFetchFuncParam(key, value, best_placement_edgeset, total_bandwidth_usage, event_list, skip_propagation_latency), recvrsp_source_addr_(recvrsp_source_addr), recvrsp_socket_server_ptr_(recvrsp_socket_server_ptr)
    {
    }

    NotifyBeaconForPlacementAfterHybridFetchFuncParam::~NotifyBeaconForPlacementAfterHybridFetchFuncParam()
    {}

    const NetworkAddr& NotifyBeaconForPlacementAfterHybridFetchFuncParam::getRecvrspSourceAddrConstRef() const
    {
        return recvrsp_source_addr_;
    }

    UdpMsgSocketServer* NotifyBeaconForPlacementAfterHybridFetchFuncParam::getRecvrspSocketServerPtr() const
    {
        return recvrsp_socket_server_ptr_;
    }

    // (4) For edge wrapper

    // UpdateCacheManagerForLocalSyncedVictimsFuncParam

    const std::string UpdateCacheManagerForLocalSyncedVictimsFuncParam::kClassName("UpdateCacheManagerForLocalSyncedVictimsFuncParam");

    const std::string UpdateCacheManagerForLocalSyncedVictimsFuncParam::FUNCNAME("update_cache_manager_for_local_synced_victims");

    UpdateCacheManagerForLocalSyncedVictimsFuncParam::UpdateCacheManagerForLocalSyncedVictimsFuncParam(const bool& affect_victim_tracker) : EdgeCustomFuncParamBase(), affect_victim_tracker_(affect_victim_tracker)
    {
    }

    UpdateCacheManagerForLocalSyncedVictimsFuncParam::~UpdateCacheManagerForLocalSyncedVictimsFuncParam()
    {}

    bool UpdateCacheManagerForLocalSyncedVictimsFuncParam::isAffectVictimTracker() const
    {
        return affect_victim_tracker_;
    }

    // UpdateCacheManagerForNeighborVictimSyncsetFuncParam

    const std::string UpdateCacheManagerForNeighborVictimSyncsetFuncParam::kClassName("UpdateCacheManagerForNeighborVictimSyncsetFuncParam");

    const std::string UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME("update_cache_manager_for_neighbor_victim_syncset");

    UpdateCacheManagerForNeighborVictimSyncsetFuncParam::UpdateCacheManagerForNeighborVictimSyncsetFuncParam(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset) : EdgeCustomFuncParamBase(), source_edge_idx_(source_edge_idx), victim_syncset_const_ref_(neighbor_victim_syncset)
    {
    }

    UpdateCacheManagerForNeighborVictimSyncsetFuncParam::~UpdateCacheManagerForNeighborVictimSyncsetFuncParam()
    {}

    uint32_t UpdateCacheManagerForNeighborVictimSyncsetFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const VictimSyncset& UpdateCacheManagerForNeighborVictimSyncsetFuncParam::getVictimSyncsetConstRef() const
    {
        return victim_syncset_const_ref_;
    }

    // NonblockDataFetchForPlacementFuncParam

    const std::string NonblockDataFetchForPlacementFuncParam::kClassName("NonblockDataFetchForPlacementFuncParam");

    const std::string NonblockDataFetchForPlacementFuncParam::FUNCNAME("nonblock_data_fetch_for_placement");

    NonblockDataFetchForPlacementFuncParam::NonblockDataFetchForPlacementFuncParam(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) : EdgeCustomFuncParamBase(), key_const_ref_(key), best_placement_edgeset_const_ref_(best_placement_edgeset), skip_propagation_latency_(skip_propagation_latency), sender_is_beacon_(sender_is_beacon), need_hybrid_fetching_ref_(need_hybrid_fetching)
    {
    }

    NonblockDataFetchForPlacementFuncParam::~NonblockDataFetchForPlacementFuncParam()
    {}

    const Key& NonblockDataFetchForPlacementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Edgeset& NonblockDataFetchForPlacementFuncParam::getBestPlacementEdgesetConstRef() const
    {
        return best_placement_edgeset_const_ref_;
    }

    bool NonblockDataFetchForPlacementFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool NonblockDataFetchForPlacementFuncParam::isSenderBeacon() const
    {
        return sender_is_beacon_;
    }

    bool& NonblockDataFetchForPlacementFuncParam::needHybridFetchingRef()
    {
        return need_hybrid_fetching_ref_;
    }

    // NonblockDataFetchFromCloudForPlacement

    const std::string NonblockDataFetchFromCloudForPlacementFuncParam::kClassName("NonblockDataFetchFromCloudForPlacementFuncParam");

    const std::string NonblockDataFetchFromCloudForPlacementFuncParam::FUNCNAME("nonblock_data_fetch_from_cloud_for_placement");

    NonblockDataFetchFromCloudForPlacementFuncParam::NonblockDataFetchFromCloudForPlacementFuncParam(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), best_placement_edgeset_const_ref_(best_placement_edgeset), skip_propagation_latency_(skip_propagation_latency)
    {
    }

    NonblockDataFetchFromCloudForPlacementFuncParam::~NonblockDataFetchFromCloudForPlacementFuncParam()
    {}

    const Key& NonblockDataFetchFromCloudForPlacementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Edgeset& NonblockDataFetchFromCloudForPlacementFuncParam::getBestPlacementEdgesetConstRef() const
    {
        return best_placement_edgeset_const_ref_;
    }

    bool NonblockDataFetchFromCloudForPlacementFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    // NonblockNotifyForPlacementFuncParam

    const std::string NonblockNotifyForPlacementFuncParam::kClassName("NonblockNotifyForPlacementFuncParam");

    const std::string NonblockNotifyForPlacementFuncParam::FUNCNAME("nonblock_notify_for_placement");

    NonblockNotifyForPlacementFuncParam::NonblockNotifyForPlacementFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), value_const_ref_(value), best_placement_edgeset_const_ref_(best_placement_edgeset), skip_propagation_latency_(skip_propagation_latency)
    {
    }

    NonblockNotifyForPlacementFuncParam::~NonblockNotifyForPlacementFuncParam()
    {}

    const Key& NonblockNotifyForPlacementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Value& NonblockNotifyForPlacementFuncParam::getValueConstRef() const
    {
        return value_const_ref_;
    }

    const Edgeset& NonblockNotifyForPlacementFuncParam::getBestPlacementEdgesetConstRef() const
    {
        return best_placement_edgeset_const_ref_;
    }

    bool NonblockNotifyForPlacementFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    // ProcessMetadataUpdateRequirementFuncParam

    const std::string ProcessMetadataUpdateRequirementFuncParam::kClassName("ProcessMetadataUpdateRequirementFuncParam");

    const std::string ProcessMetadataUpdateRequirementFuncParam::FUNCNAME("process_metadata_update_requirement");

    ProcessMetadataUpdateRequirementFuncParam::ProcessMetadataUpdateRequirementFuncParam(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), metadata_update_requirement_const_ref_(metadata_update_requirement), skip_propagation_latency_(skip_propagation_latency)
    {
    }

    ProcessMetadataUpdateRequirementFuncParam::~ProcessMetadataUpdateRequirementFuncParam()
    {}

    const Key& ProcessMetadataUpdateRequirementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const MetadataUpdateRequirement& ProcessMetadataUpdateRequirementFuncParam::getMetadataUpdateRequirementConstRef() const
    {
        return metadata_update_requirement_const_ref_;
    }

    bool ProcessMetadataUpdateRequirementFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    // CalcLocalCachedRewardFuncParam

    const std::string CalcLocalCachedRewardFuncParam::kClassName("CalcLocalCachedRewardFuncParam");

    const std::string CalcLocalCachedRewardFuncParam::FUNCNAME("calc_local_cached_reward");

    CalcLocalCachedRewardFuncParam::CalcLocalCachedRewardFuncParam(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies) : EdgeCustomFuncParamBase(), local_cached_popularity_const_ref_(local_cached_popularity), redirected_cached_popularity_const_ref_(redirected_cached_popularity), is_last_copies_(is_last_copies)
    {
        reward_ = 0;
    }

    CalcLocalCachedRewardFuncParam::~CalcLocalCachedRewardFuncParam()
    {}

    const Popularity& CalcLocalCachedRewardFuncParam::getLocalCachedPopularityConstRef() const
    {
        return local_cached_popularity_const_ref_;
    }

    const Popularity& CalcLocalCachedRewardFuncParam::getRedirectedCachedPopularityConstRef() const
    {
        return redirected_cached_popularity_const_ref_;
    }

    bool CalcLocalCachedRewardFuncParam::isLastCopies() const
    {
        return is_last_copies_;
    }

    Reward& CalcLocalCachedRewardFuncParam::getRewardRef()
    {
        return reward_;
    }

    const Reward& CalcLocalCachedRewardFuncParam::getRewardConstRef() const
    {
        return reward_;
    }

    // CalcLocalUncachedRewardFuncParam

    const std::string CalcLocalUncachedRewardFuncParam::kClassName("CalcLocalUncachedRewardFuncParam");

    const std::string CalcLocalUncachedRewardFuncParam::FUNCNAME("calc_local_uncached_reward");

    CalcLocalUncachedRewardFuncParam::CalcLocalUncachedRewardFuncParam(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity) : EdgeCustomFuncParamBase(), local_uncached_popularity_const_ref_(local_uncached_popularity), is_global_cached_(is_global_cached), redirected_uncached_popularity_const_ref_(redirected_uncached_popularity)
    {
        reward_ = 0;
    }

    CalcLocalUncachedRewardFuncParam::~CalcLocalUncachedRewardFuncParam()
    {}

    const Popularity& CalcLocalUncachedRewardFuncParam::getLocalUncachedPopularityConstRef() const
    {
        return local_uncached_popularity_const_ref_;
    }

    bool CalcLocalUncachedRewardFuncParam::isGlobalCached() const
    {
        return is_global_cached_;
    }

    const Popularity& CalcLocalUncachedRewardFuncParam::getRedirectedUncachedPopularityConstRef() const
    {
        return redirected_uncached_popularity_const_ref_;
    }

    Reward& CalcLocalUncachedRewardFuncParam::getRewardRef()
    {
        return reward_;
    }

    const Reward& CalcLocalUncachedRewardFuncParam::getRewardConstRef() const
    {
        return reward_;
    }

    // AfterDirectoryLookupHelperFuncParam

    const std::string AfterDirectoryLookupHelperFuncParam::kClassName("AfterDirectoryLookupHelperFuncParam");

    const std::string AfterDirectoryLookupHelperFuncParam::FUNCNAME("after_directory_lookup_helper");

    AfterDirectoryLookupHelperFuncParam::AfterDirectoryLookupHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), source_edge_idx_(source_edge_idx), collected_popularity_const_ref_(collected_popularity), is_global_cached_(is_global_cached), is_source_cached_(is_source_cached), best_placement_edgeset_ref_(best_placement_edgeset), need_hybrid_fetching_ref_(need_hybrid_fetching), fast_path_hint_ptr_(fast_path_hint_ptr), recvrsp_socket_server_ptr_(recvrsp_socket_server_ptr), recvrsp_source_addr_(recvrsp_source_addr), total_bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    AfterDirectoryLookupHelperFuncParam::~AfterDirectoryLookupHelperFuncParam()
    {}

    const Key& AfterDirectoryLookupHelperFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    uint32_t AfterDirectoryLookupHelperFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const CollectedPopularity& AfterDirectoryLookupHelperFuncParam::getCollectedPopularityConstRef() const
    {
        return collected_popularity_const_ref_;
    }

    bool AfterDirectoryLookupHelperFuncParam::isGlobalCached() const
    {
        return is_global_cached_;
    }

    bool AfterDirectoryLookupHelperFuncParam::isSourceCached() const
    {
        return is_source_cached_;
    }

    Edgeset& AfterDirectoryLookupHelperFuncParam::getBestPlacementEdgesetRef()
    {
        return best_placement_edgeset_ref_;
    }

    bool& AfterDirectoryLookupHelperFuncParam::needHybridFetchingRef()
    {
        return need_hybrid_fetching_ref_;
    }

    FastPathHint* AfterDirectoryLookupHelperFuncParam::getFastPathHintPtr() const
    {
        return fast_path_hint_ptr_;
    }

    UdpMsgSocketServer* AfterDirectoryLookupHelperFuncParam::getRecvrspSocketServerPtr() const
    {
        return recvrsp_socket_server_ptr_;
    }

    const NetworkAddr& AfterDirectoryLookupHelperFuncParam::getRecvrspSourceAddrConstRef() const
    {
        return recvrsp_source_addr_;
    }

    BandwidthUsage& AfterDirectoryLookupHelperFuncParam::getTotalBandwidthUsageRef()
    {
        return total_bandwidth_usage_ref_;
    }

    EventList& AfterDirectoryLookupHelperFuncParam::getEventListRef()
    {
        return event_list_ref_;
    }

    bool AfterDirectoryLookupHelperFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool& AfterDirectoryLookupHelperFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& AfterDirectoryLookupHelperFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }

    // AfterDirectoryAdmissionHelperFuncParam

    const std::string AfterDirectoryAdmissionHelperFuncParam::kClassName("AfterDirectoryAdmissionHelperFuncParam");

    const std::string AfterDirectoryAdmissionHelperFuncParam::FUNCNAME("after_directory_admission_helper");

    AfterDirectoryAdmissionHelperFuncParam::AfterDirectoryAdmissionHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), source_edge_idx_(source_edge_idx), metadata_update_requirement_const_ref_(metadata_update_requirement), directory_info_const_ref_(directory_info), skip_propagation_latency_(skip_propagation_latency)
    {
    }

    AfterDirectoryAdmissionHelperFuncParam::~AfterDirectoryAdmissionHelperFuncParam()
    {}

    const Key& AfterDirectoryAdmissionHelperFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    uint32_t AfterDirectoryAdmissionHelperFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const MetadataUpdateRequirement& AfterDirectoryAdmissionHelperFuncParam::getMetadataUpdateRequirementConstRef() const
    {
        return metadata_update_requirement_const_ref_;
    }

    const DirectoryInfo& AfterDirectoryAdmissionHelperFuncParam::getDirectoryInfoConstRef() const
    {
        return directory_info_const_ref_;
    }

    bool AfterDirectoryAdmissionHelperFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    // AfterDirectoryEvictionHelperFuncParam

    const std::string AfterDirectoryEvictionHelperFuncParam::kClassName("AfterDirectoryEvictionHelperFuncParam");

    const std::string AfterDirectoryEvictionHelperFuncParam::FUNCNAME("after_directory_eviction_helper");

    AfterDirectoryEvictionHelperFuncParam::AfterDirectoryEvictionHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) : EdgeCustomFuncParamBase(), key_const_ref_(key), source_edge_idx_(source_edge_idx), metadata_update_requirement_const_ref_(metadata_update_requirement), directory_info_const_ref_(directory_info), collected_popularity_const_ref_(collected_popularity), is_global_cached_(is_global_cached), best_placement_edgeset_ref_(best_placement_edgeset), need_hybrid_fetching_ref_(need_hybrid_fetching), recvrsp_socket_server_ptr_(recvrsp_socket_server_ptr), recvrsp_source_addr_(recvrsp_source_addr), total_bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency), is_background_(is_background)
    {
        is_finish_ = false;
    }

    AfterDirectoryEvictionHelperFuncParam::~AfterDirectoryEvictionHelperFuncParam()
    {}

    const Key& AfterDirectoryEvictionHelperFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    uint32_t AfterDirectoryEvictionHelperFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const MetadataUpdateRequirement& AfterDirectoryEvictionHelperFuncParam::getMetadataUpdateRequirementConstRef() const
    {
        return metadata_update_requirement_const_ref_;
    }

    const DirectoryInfo& AfterDirectoryEvictionHelperFuncParam::getDirectoryInfoConstRef() const
    {
        return directory_info_const_ref_;
    }

    const CollectedPopularity& AfterDirectoryEvictionHelperFuncParam::getCollectedPopularityConstRef() const
    {
        return collected_popularity_const_ref_;
    }

    bool AfterDirectoryEvictionHelperFuncParam::isGlobalCached() const
    {
        return is_global_cached_;
    }

    Edgeset& AfterDirectoryEvictionHelperFuncParam::getBestPlacementEdgesetRef()
    {
        return best_placement_edgeset_ref_;
    }

    bool& AfterDirectoryEvictionHelperFuncParam::needHybridFetchingRef()
    {
        return need_hybrid_fetching_ref_;
    }

    UdpMsgSocketServer* AfterDirectoryEvictionHelperFuncParam::getRecvrspSocketServerPtr() const
    {
        return recvrsp_socket_server_ptr_;
    }

    const NetworkAddr& AfterDirectoryEvictionHelperFuncParam::getRecvrspSourceAddrConstRef() const
    {
        return recvrsp_source_addr_;
    }

    BandwidthUsage& AfterDirectoryEvictionHelperFuncParam::getTotalBandwidthUsageRef()
    {
        return total_bandwidth_usage_ref_;
    }

    EventList& AfterDirectoryEvictionHelperFuncParam::getEventListRef()
    {
        return event_list_ref_;
    }

    bool AfterDirectoryEvictionHelperFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool AfterDirectoryEvictionHelperFuncParam::isBackground() const
    {
        return is_background_;
    }

    bool& AfterDirectoryEvictionHelperFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& AfterDirectoryEvictionHelperFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }

    // AfterWritelockAcquireHelperFuncParam

    const std::string AfterWritelockAcquireHelperFuncParam::kClassName("AfterWritelockAcquireHelperFuncParam");

    const std::string AfterWritelockAcquireHelperFuncParam::FUNCNAME("after_writelock_acquire_helper");

    AfterWritelockAcquireHelperFuncParam::AfterWritelockAcquireHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), source_edge_idx_(source_edge_idx), collected_popularity_const_ref_(collected_popularity), is_global_cached_(is_global_cached), is_source_cached_(is_source_cached), recvrsp_socket_server_ptr_(recvrsp_socket_server_ptr), recvrsp_source_addr_(recvrsp_source_addr), total_bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    AfterWritelockAcquireHelperFuncParam::~AfterWritelockAcquireHelperFuncParam()
    {}

    const Key& AfterWritelockAcquireHelperFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    uint32_t AfterWritelockAcquireHelperFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const CollectedPopularity& AfterWritelockAcquireHelperFuncParam::getCollectedPopularityConstRef() const
    {
        return collected_popularity_const_ref_;
    }

    bool AfterWritelockAcquireHelperFuncParam::isGlobalCached() const
    {
        return is_global_cached_;
    }

    bool AfterWritelockAcquireHelperFuncParam::isSourceCached() const
    {
        return is_source_cached_;
    }

    UdpMsgSocketServer* AfterWritelockAcquireHelperFuncParam::getRecvrspSocketServerPtr() const
    {
        return recvrsp_socket_server_ptr_;
    }

    const NetworkAddr& AfterWritelockAcquireHelperFuncParam::getRecvrspSourceAddrConstRef() const
    {
        return recvrsp_source_addr_;
    }

    BandwidthUsage& AfterWritelockAcquireHelperFuncParam::getTotalBandwidthUsageRef()
    {
        return total_bandwidth_usage_ref_;
    }

    EventList& AfterWritelockAcquireHelperFuncParam::getEventListRef()
    {
        return event_list_ref_;
    }

    bool AfterWritelockAcquireHelperFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool& AfterWritelockAcquireHelperFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& AfterWritelockAcquireHelperFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }

    // AfterWritelockReleaseHelperFuncParam

    const std::string AfterWritelockReleaseHelperFuncParam::kClassName("AfterWritelockReleaseHelperFuncParam");

    const std::string AfterWritelockReleaseHelperFuncParam::FUNCNAME("after_writelock_release_helper");

    AfterWritelockReleaseHelperFuncParam::AfterWritelockReleaseHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : EdgeCustomFuncParamBase(), key_const_ref_(key), source_edge_idx_(source_edge_idx), collected_popularity_const_ref_(collected_popularity), is_source_cached_(is_source_cached), best_placement_edgeset_ref_(best_placement_edgeset), need_hybrid_fetching_ref_(need_hybrid_fetching), recvrsp_socket_server_ptr_(recvrsp_socket_server_ptr), recvrsp_source_addr_(recvrsp_source_addr), total_bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    AfterWritelockReleaseHelperFuncParam::~AfterWritelockReleaseHelperFuncParam()
    {}

    const Key& AfterWritelockReleaseHelperFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    uint32_t AfterWritelockReleaseHelperFuncParam::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const CollectedPopularity& AfterWritelockReleaseHelperFuncParam::getCollectedPopularityConstRef() const
    {
        return collected_popularity_const_ref_;
    }

    bool AfterWritelockReleaseHelperFuncParam::isSourceCached() const
    {
        return is_source_cached_;
    }

    Edgeset& AfterWritelockReleaseHelperFuncParam::getBestPlacementEdgesetRef()
    {
        return best_placement_edgeset_ref_;
    }

    bool& AfterWritelockReleaseHelperFuncParam::needHybridFetchingRef()
    {
        return need_hybrid_fetching_ref_;
    }

    UdpMsgSocketServer* AfterWritelockReleaseHelperFuncParam::getRecvrspSocketServerPtr() const
    {
        return recvrsp_socket_server_ptr_;
    }

    const NetworkAddr& AfterWritelockReleaseHelperFuncParam::getRecvrspSourceAddrConstRef() const
    {
        return recvrsp_source_addr_;
    }

    BandwidthUsage& AfterWritelockReleaseHelperFuncParam::getTotalBandwidthUsageRef()
    {
        return total_bandwidth_usage_ref_;
    }

    EventList& AfterWritelockReleaseHelperFuncParam::getEventListRef()
    {
        return event_list_ref_;
    }

    bool AfterWritelockReleaseHelperFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool& AfterWritelockReleaseHelperFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& AfterWritelockReleaseHelperFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }
}
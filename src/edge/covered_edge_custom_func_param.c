#include "edge/covered_edge_custom_func_param.h"

namespace covered
{
    // ProcessRspToRedirectGetForPlacementFuncParam for edge beacon server

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

    // ProcessRspToAccessCloudForPlacementFuncParam for edge beacon server

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

    // TryToTriggerCachePlacementForGetrspFuncParam for edge cache server

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

    bool TryToTriggerCachePlacementForGetrspFuncParam::isFinish() const
    {
        return is_finish_;
    }

    void TryToTriggerCachePlacementForGetrspFuncParam::setIsFinish(const bool& is_finish)
    {
        is_finish_ = is_finish;
        return;
    }

    // TryToTriggerPlacementNotificationAfterHybridFetchFuncParam for edge cache server

    const std::string TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::kClassName("TryToTriggerPlacementNotificationAfterHybridFetchFuncParam");

    const std::string TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::FUNCNAME("try_to_trigger_cache_placement_for_getrsp");

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

    bool TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::isFinish() const
    {
        return is_finish_;
    }

    void TryToTriggerPlacementNotificationAfterHybridFetchFuncParam::setIsFinish(const bool& is_finish)
    {
        is_finish_ = is_finish;
        return;
    }
}
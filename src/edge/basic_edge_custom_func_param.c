#include "edge/basic_edge_custom_func_param.h"

namespace covered
{
    // TriggerBestGuessPlacementFuncParam for edge cache server of BestGuess

    const std::string TriggerBestGuessPlacementFuncParam::kClassName("TriggerBestGuessPlacementFuncParam");

    const std::string TriggerBestGuessPlacementFuncParam::FUNCNAME("trigger_best_guess_placement");

    TriggerBestGuessPlacementFuncParam::TriggerBestGuessPlacementFuncParam(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) : key_const_ref_(key), value_const_ref_(value), bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), skip_propagation_latency_(skip_propagation_latency)
    {
        is_finish_ = false;
    }

    TriggerBestGuessPlacementFuncParam::~TriggerBestGuessPlacementFuncParam()
    {
    }

    const Key& TriggerBestGuessPlacementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Value& TriggerBestGuessPlacementFuncParam::getValueConstRef() const
    {
        return value_const_ref_;
    }

    BandwidthUsage& TriggerBestGuessPlacementFuncParam::getTotalBandwidthUsageRef() const
    {
        return bandwidth_usage_ref_;
    }

    EventList& TriggerBestGuessPlacementFuncParam::getEventListRef() const
    {
        return event_list_ref_;
    }

    bool TriggerBestGuessPlacementFuncParam::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool TriggerBestGuessPlacementFuncParam::isFinish() const
    {
        return is_finish_;
    }

    void TriggerBestGuessPlacementFuncParam::setIsFinish(const bool& is_finish)
    {
        is_finish_ = is_finish;
        return;
    }
}
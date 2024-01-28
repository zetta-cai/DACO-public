/*
 * Different custom function parameters of baselines for edge module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef BASIC_EDGE_CUSTOM_FUNC_PARAM_H
#define BASIC_EDGE_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "common/bandwidth_usage.h"
#include "common/key.h"
#include "common/value.h"
#include "edge/edge_custom_func_param_base.h"
#include "event/event_list.h"
#include "message/message_base.h"

namespace covered
{
    // TriggerBestGuessPlacementFuncParam for edge cache server of BestGuess

    class TriggerBestGuessPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // trigger best-guess placement/replacement policy

        TriggerBestGuessPlacementFuncParam(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        virtual ~TriggerBestGuessPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef() const;
        EventList& getEventListRef() const;
        bool isSkipPropagationLatency() const;

        bool isFinish() const;
        void setIsFinish(const bool& is_finish);
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Value& value_const_ref_;
        BandwidthUsage& bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };
}

#endif
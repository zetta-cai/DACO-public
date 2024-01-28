/*
 * Different custom function parameters of COVERED for edge module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef COVERED_EDGE_CUSTOM_FUNC_PARAM_H
#define COVERED_EDGE_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "core/popularity/collected_popularity.h"
#include "core/popularity/edgeset.h"
#include "core/popularity/fast_path_hint.h"
#include "edge/edge_custom_func_param_base.h"
#include "message/message_base.h"

namespace covered
{
    // ProcessRspToRedirectGetForPlacementFuncParam for edge beacon server

    class ProcessRspToRedirectGetForPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // process redirected get response of non-blocking data fetching for non-blocking placement deployment

        ProcessRspToRedirectGetForPlacementFuncParam(MessageBase* message_ptr);
        virtual ~ProcessRspToRedirectGetForPlacementFuncParam();

        MessageBase* getMessagePtr() const;
    private:
        static const std::string kClassName;

        MessageBase* message_ptr_;
    };

    // ProcessRspToAccessCloudForPlacementFuncParam for edge beacon server

    class ProcessRspToAccessCloudForPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // process global get response of non-blocking data fetching for non-blocking placement deployment

        ProcessRspToAccessCloudForPlacementFuncParam(MessageBase* message_ptr);
        virtual ~ProcessRspToAccessCloudForPlacementFuncParam();

        MessageBase* getMessagePtr() const;
    private:
        static const std::string kClassName;

        MessageBase* message_ptr_;
    };

    // TryToTriggerCachePlacementForGetrspFuncParam for edge cache server

    class TryToTriggerCachePlacementForGetrspFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // try to trigger fast-path cache placement for get response

        TryToTriggerCachePlacementForGetrspFuncParam(const Key& key, const Value& value, const CollectedPopularity& collected_popularity_after_fetch_value, const FastPathHint& fast_path_hint, const bool& is_global_cached, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~TryToTriggerCachePlacementForGetrspFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        const CollectedPopularity& getCollectedPopularityConstRef() const;
        const FastPathHint& getFastPathHintConstRef() const;
        bool isGlobalCached() const;
        BandwidthUsage& getTotalBandwidthUsageRef() const;
        EventList& getEventListRef() const;
        bool isSkipPropagationLatency() const;

        bool isFinish() const;
        void setIsFinish(const bool& is_finish);
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Value& value_const_ref_;
        const CollectedPopularity& collected_popularity_const_ref_;
        const FastPathHint& fast_path_hint_const_ref_;
        const bool is_global_cached_;
        BandwidthUsage& bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };

    // TryToTriggerPlacementNotificationAfterHybridFetchFuncParam for edge cache server

    class TryToTriggerPlacementNotificationAfterHybridFetchFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // try to trigger fast-path cache placement for get response

        TryToTriggerPlacementNotificationAfterHybridFetchFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~TryToTriggerPlacementNotificationAfterHybridFetchFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef() const;
        EventList& getEventListRef() const;
        bool isSkipPropagationLatency() const;

        bool isFinish() const;
        void setIsFinish(const bool& is_finish);
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Value& value_const_ref_;
        const Edgeset& best_placement_edgeset_const_ref_;
        BandwidthUsage& bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };
}

#endif
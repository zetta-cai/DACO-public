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
#include "core/victim/victim_syncset.h"
#include "edge/edge_custom_func_param_base.h"
#include "message/message_base.h"

namespace covered
{
    // (1) For edge beacon server

    // ProcessRspToRedirectGetForPlacementFuncParam

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

    // ProcessRspToAccessCloudForPlacementFuncParam

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

    // (2) For edge cache server

    // TryToTriggerCachePlacementForGetrspFuncParam

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

    // TryToTriggerPlacementNotificationAfterHybridFetchFuncParam

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

    // (3) For edge wrapper

    // UpdateCacheManagerForLocalSyncedVictimsFuncParam

    class UpdateCacheManagerForLocalSyncedVictimsFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Update covered cache manager for local synced victims

        UpdateCacheManagerForLocalSyncedVictimsFuncParam(const bool& affect_victim_tracker);
        ~UpdateCacheManagerForLocalSyncedVictimsFuncParam();

        bool isAffectVictimTracker() const;
    private:
        static const std::string kClassName;

        const bool affect_victim_tracker_;
    };

    // UpdateCacheManagerForNeighborVictimSyncsetFuncParam

    class UpdateCacheManagerForNeighborVictimSyncsetFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Update covered cache manager for received neighbor victim syncset

        UpdateCacheManagerForNeighborVictimSyncsetFuncParam(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset);
        ~UpdateCacheManagerForNeighborVictimSyncsetFuncParam();

        uint32_t getSourceEdgeIdx() const;
        const VictimSyncset& getVictimSyncsetConstRef() const;
    private:
        static const std::string kClassName;

        const uint32_t source_edge_idx_;
        const VictimSyncset& victim_syncset_const_ref_;
    };

    // NonblockDataFetchForPlacementFuncParam

    class NonblockDataFetchForPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Non-blocking data fetching for non-blocking placement deployment

        NonblockDataFetchForPlacementFuncParam(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching);
        ~NonblockDataFetchForPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        bool isSkipPropagationLatency() const;
        bool isSenderBeacon() const;
        bool& getNeedHybridFetchingRef() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Edgeset& best_placement_edgeset_const_ref_;
        const bool skip_propagation_latency_;
        const bool sender_is_beacon_;
        bool& need_hybrid_fetching_ref_;
    };

    // NonblockDataFetchFromCloudForPlacement

    // TODO: END HERE
}

#endif
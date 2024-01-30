/*
 * Different custom function parameters of COVERED for edge module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef COVERED_EDGE_CUSTOM_FUNC_PARAM_H
#define COVERED_EDGE_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "cooperation/directory/metadata_update_requirement.h"
#include "core/popularity/collected_popularity.h"
#include "core/popularity/edgeset.h"
#include "core/popularity/fast_path_hint.h"
#include "core/victim/victim_syncset.h"
#include "edge/edge_custom_func_param_base.h"
#include "message/message_base.h"
#include "network/udp_msg_socket_server.h"

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

    // (2) For edge cache server worker

    // TryToTriggerCachePlacementForGetrspFuncParam

    class TryToTriggerCachePlacementForGetrspFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // try to trigger fast-path cache placement calculation for get response (or trigger normal placement if current is beacon)

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

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
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
        static const std::string FUNCNAME; // try to trigger non-blocking placement deployment after hybrid data fetching (or directly issue placement notifications if current is beacon)

        TryToTriggerPlacementNotificationAfterHybridFetchFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~TryToTriggerPlacementNotificationAfterHybridFetchFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef() const;
        EventList& getEventListRef() const;
        bool isSkipPropagationLatency() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
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

    // (3) For edge cache server

    // NotifyBeaconForPlacementAfterHybridFetchFuncParam

    class NotifyBeaconForPlacementAfterHybridFetchFuncParam : public TryToTriggerPlacementNotificationAfterHybridFetchFuncParam
    {
    public:
        static const std::string FUNCNAME; // trigger non-blocking placement deployment after hybrid data fetching

        NotifyBeaconForPlacementAfterHybridFetchFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~NotifyBeaconForPlacementAfterHybridFetchFuncParam();

        const NetworkAddr& getRecvrspSourceAddrConstRef() const;
        UdpMsgSocketServer* getRecvrspSocketServerPtr() const;
    private:
        static const std::string kClassName;

        const NetworkAddr& recvrsp_source_addr_;
        UdpMsgSocketServer* recvrsp_socket_server_ptr_;
    };

    // (4) For edge wrapper

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
        static const std::string FUNCNAME; // Non-blocking data fetching from local (beacon) or neighbor for non-blocking placement deployment

        NonblockDataFetchForPlacementFuncParam(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching);
        ~NonblockDataFetchForPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        bool isSkipPropagationLatency() const;
        bool isSenderBeacon() const;
        bool& needHybridFetchingRef();
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Edgeset& best_placement_edgeset_const_ref_;
        const bool skip_propagation_latency_;
        const bool sender_is_beacon_;
        bool& need_hybrid_fetching_ref_;
    };

    // NonblockDataFetchFromCloudForPlacementFuncParam

    class NonblockDataFetchFromCloudForPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Non-blocking data fetching from cloud for non-blocking placement deployment

        NonblockDataFetchFromCloudForPlacementFuncParam(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency);
        ~NonblockDataFetchFromCloudForPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        bool isSkipPropagationLatency() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Edgeset& best_placement_edgeset_const_ref_;
        const bool skip_propagation_latency_;
    };

    // NonblockNotifyForPlacementFuncParam

    class NonblockNotifyForPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Non-blocking placement notification for non-blocking placement deployment

        NonblockNotifyForPlacementFuncParam(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency);
        ~NonblockNotifyForPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        const Edgeset& getBestPlacementEdgesetConstRef() const;
        bool isSkipPropagationLatency() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Value& value_const_ref_;
        const Edgeset& best_placement_edgeset_const_ref_;
        const bool skip_propagation_latency_;
    };

    // ProcessMetadataUpdateRequirementFuncParam

    class ProcessMetadataUpdateRequirementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Trigger metadata update (beacon-based cache status synchronization) if necessary

        ProcessMetadataUpdateRequirementFuncParam(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const bool& skip_propagation_latency);
        ~ProcessMetadataUpdateRequirementFuncParam();

        const Key& getKeyConstRef() const;
        const MetadataUpdateRequirement& getMetadataUpdateRequirementConstRef() const;
        bool isSkipPropagationLatency() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const MetadataUpdateRequirement& metadata_update_requirement_const_ref_;
        const bool skip_propagation_latency_;
    };

    // CalcLocalCachedRewardFuncParam

    class CalcLocalCachedRewardFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Calculate local cached reward

        CalcLocalCachedRewardFuncParam(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies);
        ~CalcLocalCachedRewardFuncParam();

        const Popularity& getLocalCachedPopularityConstRef() const;
        const Popularity& getRedirectedCachedPopularityConstRef() const;
        bool isLastCopies() const;

        Reward& getRewardRef();
        const Reward& getRewardConstRef() const;
    private:
        static const std::string kClassName;

        const Popularity& local_cached_popularity_const_ref_;
        const Popularity& redirected_cached_popularity_const_ref_;
        const bool is_last_copies_;

        Reward reward_;
    };

    // CalcLocalUncachedRewardFuncParam

    class CalcLocalUncachedRewardFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Calculate local uncached reward

        CalcLocalUncachedRewardFuncParam(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity = 0.0);
        ~CalcLocalUncachedRewardFuncParam();

        const Popularity& getLocalUncachedPopularityConstRef() const;
        bool isGlobalCached() const;
        const Popularity& getRedirectedUncachedPopularityConstRef() const;

        Reward& getRewardRef();
        const Reward& getRewardConstRef() const;
    private:
        static const std::string kClassName;

        const Popularity& local_uncached_popularity_const_ref_;
        const bool is_global_cached_;
        const Popularity& redirected_uncached_popularity_const_ref_;

        Reward reward_;
    };

    // AfterDirectoryLookupHelperFuncParam

    class AfterDirectoryLookupHelperFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Helper function after directory lookup

        AfterDirectoryLookupHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~AfterDirectoryLookupHelperFuncParam();

        const Key& getKeyConstRef() const;
        uint32_t getSourceEdgeIdx() const;
        const CollectedPopularity& getCollectedPopularityConstRef() const;
        bool isGlobalCached() const;
        bool isSourceCached() const;
        Edgeset& getBestPlacementEdgesetRef();
        bool& needHybridFetchingRef();
        FastPathHint* getFastPathHintPtr() const;
        UdpMsgSocketServer* getRecvrspSocketServerPtr() const;
        const NetworkAddr& getRecvrspSourceAddrConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef();
        EventList& getEventListRef();
        bool isSkipPropagationLatency() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const uint32_t source_edge_idx_;
        const CollectedPopularity& collected_popularity_const_ref_;
        const bool is_global_cached_;
        const bool is_source_cached_;
        Edgeset& best_placement_edgeset_ref_;
        bool& need_hybrid_fetching_ref_;
        FastPathHint* fast_path_hint_ptr_;
        UdpMsgSocketServer* recvrsp_socket_server_ptr_;
        const NetworkAddr& recvrsp_source_addr_;
        BandwidthUsage& total_bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };

    // AfterDirectoryAdmissionHelperFuncParam

    class AfterDirectoryAdmissionHelperFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Helper function after directory admission

        AfterDirectoryAdmissionHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const bool& skip_propagation_latency);
        ~AfterDirectoryAdmissionHelperFuncParam();

        const Key& getKeyConstRef() const;
        uint32_t getSourceEdgeIdx() const;
        const MetadataUpdateRequirement& getMetadataUpdateRequirementConstRef() const;
        const DirectoryInfo& getDirectoryInfoConstRef() const;
        bool isSkipPropagationLatency() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const uint32_t source_edge_idx_;
        const MetadataUpdateRequirement& metadata_update_requirement_const_ref_;
        const DirectoryInfo& directory_info_const_ref_;
        const bool skip_propagation_latency_;
    };

    // AfterDirectoryEvictionHelperFuncParam

    class AfterDirectoryEvictionHelperFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Helper function after directory eviction

        AfterDirectoryEvictionHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background);
        ~AfterDirectoryEvictionHelperFuncParam();

        const Key& getKeyConstRef() const;
        uint32_t getSourceEdgeIdx() const;
        const MetadataUpdateRequirement& getMetadataUpdateRequirementConstRef() const;
        const DirectoryInfo& getDirectoryInfoConstRef() const;
        const CollectedPopularity& getCollectedPopularityConstRef() const;
        bool isGlobalCached() const;
        Edgeset& getBestPlacementEdgesetRef();
        bool& needHybridFetchingRef();
        UdpMsgSocketServer* getRecvrspSocketServerPtr() const;
        const NetworkAddr& getRecvrspSourceAddrConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef();
        EventList& getEventListRef();
        bool isSkipPropagationLatency() const;
        bool isBackground() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const uint32_t source_edge_idx_;
        const MetadataUpdateRequirement& metadata_update_requirement_const_ref_;
        const DirectoryInfo& directory_info_const_ref_;
        const CollectedPopularity& collected_popularity_const_ref_;
        const bool is_global_cached_;
        Edgeset& best_placement_edgeset_ref_;
        bool& need_hybrid_fetching_ref_;
        UdpMsgSocketServer* recvrsp_socket_server_ptr_;
        const NetworkAddr& recvrsp_source_addr_;
        BandwidthUsage& total_bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;
        const bool is_background_;

        bool is_finish_;
    };

    // AfterWritelockAcquireHelperFuncParam

    class AfterWritelockAcquireHelperFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Helper function after acquiring writelock

        AfterWritelockAcquireHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~AfterWritelockAcquireHelperFuncParam();

        const Key& getKeyConstRef() const;
        uint32_t getSourceEdgeIdx() const;
        const CollectedPopularity& getCollectedPopularityConstRef() const;
        bool isGlobalCached() const;
        bool isSourceCached() const;
        UdpMsgSocketServer* getRecvrspSocketServerPtr() const;
        const NetworkAddr& getRecvrspSourceAddrConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef();
        EventList& getEventListRef();
        bool isSkipPropagationLatency() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const uint32_t source_edge_idx_;
        const CollectedPopularity& collected_popularity_const_ref_;
        const bool is_global_cached_;
        const bool is_source_cached_;
        UdpMsgSocketServer* recvrsp_socket_server_ptr_;
        const NetworkAddr& recvrsp_source_addr_;
        BandwidthUsage& total_bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };

    // AfterWritelockReleaseHelperFuncParam

    class AfterWritelockReleaseHelperFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Helper function after releasing writelock

        AfterWritelockReleaseHelperFuncParam(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency);
        ~AfterWritelockReleaseHelperFuncParam();

        const Key& getKeyConstRef() const;
        uint32_t getSourceEdgeIdx() const;
        const CollectedPopularity& getCollectedPopularityConstRef() const;
        bool isSourceCached() const;
        Edgeset& getBestPlacementEdgesetRef();
        bool& needHybridFetchingRef();
        UdpMsgSocketServer* getRecvrspSocketServerPtr() const;
        const NetworkAddr& getRecvrspSourceAddrConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef();
        EventList& getEventListRef();
        bool isSkipPropagationLatency() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const uint32_t source_edge_idx_;
        const CollectedPopularity& collected_popularity_const_ref_;
        const bool is_source_cached_;
        Edgeset& best_placement_edgeset_ref_;
        bool& need_hybrid_fetching_ref_;
        UdpMsgSocketServer* recvrsp_socket_server_ptr_;
        const NetworkAddr& recvrsp_source_addr_;
        BandwidthUsage& total_bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const bool skip_propagation_latency_;

        bool is_finish_;
    };
}

#endif
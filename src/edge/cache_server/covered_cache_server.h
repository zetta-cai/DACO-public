/*
 * CoveredCacheServer: edge cache server for baselines.
 * 
 * By Siyuan Sheng (2024.01.30).
 */

#ifndef COVERED_CACHE_SERVER_H
#define COVERED_CACHE_SERVER_H

#include "edge/cache_server/cache_server_base.h"

namespace covered
{
    class CoveredCacheServer : public CacheServerBase
    {
    public:
        CoveredCacheServer(EdgeComponentParam* edge_component_ptr);
        virtual ~CoveredCacheServer();

        // (1) For local edge cache admission and remote directory admission (invoked by edge cache server worker for independent admission; or by placement processor for remote placement notification)
        virtual void admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid) const override;
        virtual MessageBase* getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const override;
        virtual void processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_neighbor_cached, const bool& is_background, const uint32_t& directory_admit_cross_edge_latency_us) const override;

        // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
        virtual void evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const override; // Evict local edge cache
        virtual bool evictLocalDirectory_(const Key& key, const Value& value, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const override; // Evict directory info from current edge node (return if edge node is finished)
        virtual MessageBase* getReqToEvictBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background) const override; // Send directory update request with is_admit = false to remove dirinfo of evicted victim
        virtual bool processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr, const bool& is_background, const uint32_t& directory_evict_cross_edge_latency_us = 0) const override;

        // (3) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const override;

        // Trigger non-blocking placement notification
        // NOTE: recvrsp_source_addr and recvrsp_socket_server_ptr refer to some edge cache server worker
        bool notifyBeaconForPlacementAfterHybridFetchInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const; // Sender is NOT beacon; return if edge node is finished
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
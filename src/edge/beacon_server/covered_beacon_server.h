/*
 * CoveredBeaconServer: cache server in edge for COVERED.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef COVERED_BEACON_SERVER_H
#define COVERED_BEACON_SERVER_H

#include <string>

#include "edge/beacon_server/beacon_server_base.h"

namespace covered
{
    class CoveredBeaconServer : public BeaconServerBase
    {
    public:
        CoveredBeaconServer(EdgeComponentParam* edge_beacon_server_param_ptr);
        virtual ~CoveredBeaconServer();
    private:
        static const std::string kClassName;

        // (1) Access content directory information

        virtual bool processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const override; // Return if edge node is finished
        virtual MessageBase* getRspToLookupLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        virtual bool processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, bool& is_neighbor_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_neighbor_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        // (2) Process writes and unblock for MSI protocol

        virtual bool processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToAcquireLocalWritelock_(MessageBase* control_request_ptr, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        virtual bool processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToReleaseLocalWritelock_(MessageBase* control_request_ptr, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) override;

        // (5) Cache-method-specific custom functions

        virtual void customFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) override;
        // Process placement trigger request for writes on global uncached objects
        bool processPlacementTriggerRequestForCoveredInternal_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);
        // Process redirected/global get response for non-blocking placement deployment
        void processRspToRedirectGetForPlacementInternal_(MessageBase* redirected_get_response_ptr);
        void processRspToAccessCloudForPlacementInternal_(MessageBase* global_get_response_ptr);

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif
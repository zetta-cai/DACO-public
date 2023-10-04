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
        CoveredBeaconServer(EdgeWrapper* edge_wrapper_ptr);
        virtual ~CoveredBeaconServer();
    private:
        static const std::string kClassName;

        // (1) Access content directory information

        virtual bool processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const override; // Return if edge node is finished
        virtual MessageBase* getRspToLookupLocalDirectory_(const Key& key, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const override;

        virtual bool processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        // (2) Process writes and unblock for MSI protocol

        virtual bool processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToAcquireLocalWritelock_(const Key& key, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const override;

        virtual bool processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list) override; // Return if edge node is finished
        virtual MessageBase* getRspToReleaseLocalWritelock_(const Key& key, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) const override;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) override;

        // (4) Process redirected/global get response for non-blocking placement deployment (ONLY for COVERED)

        virtual void processRspToRedirectGetForPlacement_(MessageBase* redirected_get_response_ptr) override;
        virtual void processRspToAccessCloudForPlacement_(MessageBase* global_get_response_ptr) override;

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif
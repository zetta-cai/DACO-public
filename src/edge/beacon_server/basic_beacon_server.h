/*
 * BasicBeaconServer: basic cache server in edge for baselines.
 * 
 * By Siyuan Sheng (2023.06.21).
 */

#ifndef BASIC_BEACON_SERVER_H
#define BASIC_BEACON_SERVER_H

#include <string>

#include "edge/beacon_server/beacon_server_base.h"

namespace covered
{
    class BasicBeaconServer : public BeaconServerBase
    {
    public:
        BasicBeaconServer(EdgeWrapper* edge_wrapper_ptr);
        virtual ~BasicBeaconServer();
    private:
        static const std::string kClassName;

        // (1) Access content directory information

        virtual void lookupCooperationLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_source_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        virtual bool updateCooperationLocalDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info) override; // Return if key is being written

        // (2) Process writes and unblock for MSI protocol

        virtual bool processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) override;
        virtual bool processReleaseWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) override;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) override;

        // Member variables

        std::string instance_name_;
    };
}

#endif
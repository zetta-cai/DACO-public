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

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const override;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) override;

        // (2) Process writes and unblock for MSI protocol

        virtual bool processAcquireWritelockRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) override;
        virtual void sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const override;
        virtual bool processReleaseWritelockRequest_(MessageBase* control_request_ptr) override;

        // (3) Process other control requests

        // Return if edge node is finished
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) override;

        // Member variables

        std::string instance_name_;
    };
}

#endif
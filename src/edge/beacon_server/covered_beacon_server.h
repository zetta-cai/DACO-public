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

        // Control requests

        // Return if edge node is finished
        virtual bool processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const override;
        virtual bool processDirectoryUpdateRequest_(MessageBase* control_request_ptr) override;
        virtual bool processOtherControlRequest_(MessageBase* control_request_ptr) override;

        // Const variable
        std::string instance_name_;
    };
}

#endif
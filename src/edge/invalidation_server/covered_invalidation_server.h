/*
 * CoveredInvalidationServer: invalidation server in edge for COVERED.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef COVERED_INVALIDATION_SERVER_H
#define COVERED_INVALIDATION_SERVER_H

#include <string>

#include "edge/invalidation_server/invalidation_server_base.h"

namespace covered
{
    class CoveredInvalidationServer : public InvalidationServerBase
    {
    public:
        CoveredInvalidationServer(EdgeWrapper* edge_wrapper_ptr);
        virtual ~CoveredInvalidationServer();
    private:
        static const std::string kClassName;

        virtual void processReqForInvalidation_(MessageBase* control_request_ptr) override;
        virtual MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) override;

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif
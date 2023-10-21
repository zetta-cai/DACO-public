/*
 * BasicInvalidationServer: basic invalidation server in edge for baselines.
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef BASIC_INVALIDATION_SERVER_H
#define BASIC_INVALIDATION_SERVER_H

#include <string>

#include "edge/invalidation_server/invalidation_server_base.h"

namespace covered
{
    class BasicInvalidationServer : public InvalidationServerBase
    {
    public:
        BasicInvalidationServer(EdgeWrapper* edge_wrapper_ptr);
        virtual ~BasicInvalidationServer();
    private:
        static const std::string kClassName;

        virtual void processReqForInvalidation_(MessageBase* control_request_ptr) override;
        virtual MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) override;

        // Member variables

        std::string instance_name_;
    };
}

#endif
/*
 * BasicCacheServerInvalidationProcessor: the class for baselines to process cache invalidation requests from beacon nodes.
 * 
 * By Siyuan Sheng (2024.01.31).
 */

#ifndef BASIC_CACHE_SERVER_INVALIDATION_PROCESSOR_H
#define BASIC_CACHE_SERVER_INVALIDATION_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_invalidation_processor_base.h"

namespace covered
{
    class BasicCacheServerInvalidationProcessor : public CacheServerInvalidationProcessorBase
    {
    public:
        BasicCacheServerInvalidationProcessor(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr);
        virtual ~BasicCacheServerInvalidationProcessor();

        virtual void processReqForInvalidation_(MessageBase* control_request_ptr) override;
        virtual MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) override;
    private:
        static const std::string kClassName;

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif
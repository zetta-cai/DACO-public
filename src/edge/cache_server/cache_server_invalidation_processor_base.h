/*
 * CacheServerInvalidationProcessorBase: the base class to process cache invalidation requests from beacon nodes.
 * 
 * By Siyuan Sheng (2023.12.19).
 */

#ifndef CACHE_SERVER_INVALIDATION_PROCESSOR_BASE_H
#define CACHE_SERVER_INVALIDATION_PROCESSOR_BASE_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerInvalidationProcessorBase
    {
    public:
        static void* launchCacheServerInvalidationProcessor(void* cache_server_invalidation_processor_param_ptr);
    
        CacheServerInvalidationProcessorBase(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr);
        virtual ~CacheServerInvalidationProcessorBase();

        void start();
    protected:
        void checkPointers_() const;

        // Const variable
        CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr_;
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processInvalidationRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process metadata update request
        virtual void processReqForInvalidation_(MessageBase* control_request_ptr) = 0;
        virtual MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) = 0;

        // Member variables

        // Const variable
        std::string base_instance_name_;
    };
}

#endif
/*
 * CacheServerInvalidationProcessor: process cache invalidation requests from beacon ndoes.
 * 
 * By Siyuan Sheng (2023.12.19).
 */

#ifndef CACHE_SERVER_INVALIDATION_PROCESSOR_H
#define CACHE_SERVER_INVALIDATION_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerInvalidationProcessor
    {
    public:
        static void* launchCacheServerInvalidationProcessor(void* cache_server_invalidation_processor_param_ptr);
    
        CacheServerInvalidationProcessor(CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr);
        virtual ~CacheServerInvalidationProcessor();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processInvalidationRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process metadata update request
        void processReqForInvalidation_(MessageBase* control_request_ptr);
        MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list);

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string instance_name_;
        CacheServerProcessorParam* cache_server_invalidation_processor_param_ptr_;
    };
}

#endif
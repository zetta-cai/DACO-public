/*
 * CacheServerRedirectionProcessorBase: the base class to process redirected requests (sent by neighbor edge nodes) partitioned from cache server.
 * 
 * By Siyuan Sheng (2023.10.25).
 */

#ifndef CACHE_SERVER_REDIRECTION_PROCESSOR_BASE_H
#define CACHE_SERVER_REDIRECTION_PROCESSOR_BASE_H

#include <string>

#include "edge/cache_server/cache_server_processor_param.h"

namespace covered
{
    class CacheServerRedirectionProcessorBase
    {
    public:
        static void* launchCacheServerRedirectionProcessor(void* cache_server_redirection_processor_param_ptr);
    
        CacheServerRedirectionProcessorBase(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr);
        virtual ~CacheServerRedirectionProcessorBase();

        void start();
    protected:  
        void checkPointers_() const;

        // Const variable
        CacheServerProcessorParam* cache_server_redirection_processor_param_ptr_;
    private:
        static const std::string kClassName;

        // Process redirected requests

        // Return if edge node is finished
        bool processRedirectedDataRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process redirected data request
        bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const;
        virtual void processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const = 0;
        virtual MessageBase* getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const = 0;

        // Member variables

        // Const variable
        std::string base_instance_name_;
    };
}

#endif
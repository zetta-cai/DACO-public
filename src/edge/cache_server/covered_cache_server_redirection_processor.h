/*
 * CoveredCacheServerRedirectionProcessor: the class for COVERED to process redirected requests (sent by neighbor edge nodes) partitioned from cache server.
 * 
 * By Siyuan Sheng (2024.01.31).
 */

#ifndef COVERED_CACHE_SERVER_REDIRECTION_PROCESSOR_H
#define COVERED_CACHE_SERVER_REDIRECTION_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_redirection_processor_base.h"

namespace covered
{
    class CoveredCacheServerRedirectionProcessor : public CacheServerRedirectionProcessorBase
    {
    public:
        CoveredCacheServerRedirectionProcessor(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr);
        virtual ~CoveredCacheServerRedirectionProcessor();
    private:
        static const std::string kClassName;

        virtual void processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const override;
        virtual MessageBase* getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const override;

        // Member variables

        // Const variable
        std::string instance_name_;
    };
}

#endif
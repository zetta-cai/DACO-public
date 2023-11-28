/*
 * CacheServerRedirectionProcessor: process redirected requests (sent by neighbor edge nodes) partitioned from cache server.
 * 
 * By Siyuan Sheng (2023.10.25).
 */

#ifndef CACHE_SERVER_REDIRECTION_PROCESSOR_H
#define CACHE_SERVER_REDIRECTION_PROCESSOR_H

#include <string>

#include "edge/cache_server/cache_server_redirection_processor_param.h"

namespace covered
{
    class CacheServerRedirectionProcessor
    {
    public:
        static void* launchCacheServerRedirectionProcessor(void* cache_server_redirection_processor_param_ptr);
    
        CacheServerRedirectionProcessor(CacheServerRedirectionProcessorParam* cache_server_redirection_processor_param_ptr);
        virtual ~CacheServerRedirectionProcessor();

        void start();
    private:
        static const std::string kClassName;

        // Process redirected requests

        // Return if edge node is finished
        bool processRedirectedDataRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr); // Process redirected data request
        bool processRedirectedGetRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr) const;
        void processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const;
        MessageBase* getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const;

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string instance_name_;
        const CacheServerRedirectionProcessorParam* cache_server_redirection_processor_param_ptr_;

        // NOTE: destination addresses for sending control requests come from beacon edge index

        // For receiving control responses
        //NetworkAddr edge_cache_server_redirection_processor_recvrsp_source_addr_; // Used by cache server redirection processor to send back redirected data responses (const individual variable)
        //UdpMsgSocketServer* edge_cache_server_redirection_processor_recvrsp_socket_server_ptr_; // Used by cache server redirection processor to receive responses from beacon server (non-const individual variable)
    };
}

#endif
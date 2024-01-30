/*
 * BasicCacheServer: edge cache server for baselines.
 * 
 * By Siyuan Sheng (2024.01.30).
 */

#ifndef BASIC_CACHE_SERVER_H
#define BASIC_CACHE_SERVER_H

#include "edge/cache_server/cache_server_base.h"

namespace covered
{
    class BasicCacheServer : public CacheServerBase
    {
    public:
        BasicCacheServer(EdgeWrapperBase* edge_wrapper_ptr);
        virtual ~BasicCacheServer();

        // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
        virtual bool processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const override;

        // (3) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const override;
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
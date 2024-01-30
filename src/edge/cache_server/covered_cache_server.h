/*
 * CoveredCacheServer: edge cache server for baselines.
 * 
 * By Siyuan Sheng (2024.01.30).
 */

#ifndef COVERED_CACHE_SERVER_H
#define COVERED_CACHE_SERVER_H

#include "edge/cache_server/cache_server_base.h"

namespace covered
{
    class CoveredCacheServer : public CacheServerBase
    {
    public:
        CoveredCacheServer(EdgeWrapperBase* edge_wrapper_ptr);
        virtual ~CoveredCacheServer();

        // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
        virtual bool processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const override;

        // (3) Method-specific functions
        virtual void constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const override;

        // Trigger non-blocking placement notification
        // NOTE: recvrsp_source_addr and recvrsp_socket_server_ptr refer to some edge cache server worker
        bool notifyBeaconForPlacementAfterHybridFetchInternal_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const; // Sender is NOT beacon; return if edge node is finished
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
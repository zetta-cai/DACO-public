/*
 * CooperativeCacheWrapperBase: the base class for cooperative edge caching.
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from neighbor by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of local edge cache.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATIVE_CACHE_WRAPPER_BASE_H
#define COOPERATIVE_CACHE_WRAPPER_BASE_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/dht_wrapper.h"
#include "edge/edge_param.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    enum CooperativeCacheType
    {
        kBasicCooperativeCache = 1, // for all baselines
        kCoveredCooperativeCache // only for COVERED
    };

    class CooperativeCacheWrapperBase
    {
    public:
        static std::string cooperativeCacheTypeToString(const CooperativeCacheType& cooperative_cache_type);
        static CooperativeCacheWrapperBase* getCooperativeCacheWrapper(const CooperativeCacheType& cooperative_cache_type);

        CooperativeCacheWrapperBase(EdgeParam* edge_param_ptr);
        ~CooperativeCacheWrapperBase();

        // Return if edge node is finished
        // NOTE: get() cannot be const due to changing remote address of edge_sendreq_toneighbor_socket_client_ptr_
        virtual bool get(const Key& key, Value& value, bool& is_cooperative_cached);
    private:
        static const std::string kClassName;
    protected:
        void locateBeaconNode_(const Key& key);

        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase

        DhtWrapper* dht_wrapper_ptr_;
        UdpSocketWrapper* edge_sendreq_toneighbor_socket_client_ptr_;
        // TODO: maintain directory information
    };
}

#endif
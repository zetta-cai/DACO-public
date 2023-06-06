#include "cooperation/cooperative_cache_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CooperativeCacheWrapperBase::kClassName("CooperativeCacheWrapperBase");

    std::string CooperativeCacheWrapperBase::cooperativeCacheTypeToString(const CooperativeCacheType& cooperative_cache_type)
    {
        std::string cooperative_cache_type_str = "";
        switch (cooperative_cache_type)
        {
            case CooperativeCacheType::kBasicCooperativeCache:
            {
                cooperative_cache_type_str = "kBasicCooperativeCache";
                break;
            }
            case CooperativeCacheType::kCoveredCooperativeCache:
            {
                cooperative_cache_type_str = "kCoveredCooperativeCache";
                break;
            }
            default:
            {
                cooperative_cache_type_str = std::to_string(static_cast<uint32_t>(cooperative_cache_type));
                break;
            }
        }
        return cooperative_cache_type_str;
    }

    CooperativeCacheWrapperBase* CooperativeCacheWrapperBase::getCooperativeCacheWrapper(const CooperativeCacheType& cooperative_cache_type)
    {
        CooperativeCacheWrapperBase* cooperative_cache_wrapper_ptr = NULL;

        switch (cooperative_cache_type)
        {
            case CooperativeCacheType::kBasicCooperativeCache:
            {
                //cooperative_cache_wrapper_ptr = new BasicCooperativeCacheWrapper();
                break;
            }
            case CooperativeCacheType::kCoveredCooperativeCache:
            {
                //cooperative_cache_wrapper_ptr = new CoveredCooperativeCacheWrapper();
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "cooperative cache " << cooperativeCacheTypeToString(cooperative_cache_type) << " is not supported!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
                break;
            }
        }

        assert(cooperative_cache_wrapper_ptr != NULL);
        return cooperative_cache_wrapper_ptr;
    }

    CooperativeCacheWrapperBase::CooperativeCacheWrapperBase(EdgeParam* edge_param_ptr)
    {
        assert(edge_param_ptr != NULL);
        edge_param_ptr_ = edge_param_ptr;

        dht_wrapper_ptr_ = new DhtWrapper(Param::getHashName());
        assert(dht_wrapper_ptr_ != NULL);

        // NOTE: we use edge0 as default remote address, but we will reset remote address of the socket client as the beacon node based on the key later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_port = Util::getEdgeRecvreqPort(0);
        NetworkAddr edge0_addr(edge0_ipstr, edge0_port);
        edge_sendreq_toneighbor_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_addr);
        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);
    }

    CooperativeCacheWrapperBase::~CooperativeCacheWrapperBase()
    {
        // NOTE: no need to release edge_param_ptr_ which is maintained outside CooperativeCacheWrapperBase

        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);
        delete edge_sendreq_toneighbor_socket_client_ptr_;
        edge_sendreq_toneighbor_socket_client_ptr_ = NULL;
    }

    void CooperativeCacheWrapperBase::locateBeaconNode_(const Key& key)
    {
        assert(dht_wrapper_ptr_ != NULL);
        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);

        // Set remote address such that the current edge node can communicate with the beacon node for the key
        std::string beacon_edge_ipstr = dht_wrapper_ptr_->getBeaconEdgeIpstr(key);
        uint16_t beacon_edge_port = dht_wrapper_ptr_->getBeaconEdgeRecvreqPort(key);
        NetworkAddr beacon_edge_addr(beacon_edge_ipstr, beacon_edge_port);
        edge_sendreq_toneighbor_socket_client_ptr_->setRemoteAddrForClient(beacon_edge_addr);

        return;
    }
}
#include "cooperation/cooperative_cache_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cooperation/basic_cooperative_cache_wrapper.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CooperativeCacheWrapperBase::kClassName("CooperativeCacheWrapperBase");

    CooperativeCacheWrapperBase* CooperativeCacheWrapperBase::getCooperativeCacheWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr)
    {
        CooperativeCacheWrapperBase* cooperative_cache_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            //cooperative_cache_wrapper_ptr = new CoveredCooperativeCacheWrapper(hash_name, edge_param_ptr);
        }
        else
        {
            cooperative_cache_wrapper_ptr = new BasicCooperativeCacheWrapper(hash_name, edge_param_ptr);
        }

        assert(cooperative_cache_wrapper_ptr != NULL);
        return cooperative_cache_wrapper_ptr;
    }

    CooperativeCacheWrapperBase::CooperativeCacheWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr)
    {
        assert(edge_param_ptr != NULL);
        edge_param_ptr_ = edge_param_ptr;

        dht_wrapper_ptr_ = new DhtWrapper(hash_name);
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
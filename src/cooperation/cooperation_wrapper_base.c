#include "cooperation/cooperation_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_wrapper.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CooperationWrapperBase::kClassName("CooperationWrapperBase");

    CooperationWrapperBase* CooperationWrapperBase::getCooperationWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr)
    {
        CooperationWrapperBase* cooperation_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            //cooperation_wrapper_ptr = new CoveredCooperationWrapper(hash_name, edge_param_ptr);
        }
        else
        {
            cooperation_wrapper_ptr = new BasicCooperationWrapper(hash_name, edge_param_ptr);
        }

        assert(cooperation_wrapper_ptr != NULL);
        return cooperation_wrapper_ptr;
    }

    CooperationWrapperBase::CooperationWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr)
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

        directory_hashtable_.clear();
    }

    CooperationWrapperBase::~CooperationWrapperBase()
    {
        // NOTE: no need to release edge_param_ptr_ which is maintained outside CooperationWrapperBase

        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);
        delete edge_sendreq_toneighbor_socket_client_ptr_;
        edge_sendreq_toneighbor_socket_client_ptr_ = NULL;
    }

    void CooperationWrapperBase::lookupLocalDirectory(const Key& key, bool& is_directory_exist, std::vector<uint32_t>& edge_idxes) const
    {
        std::unordered_map<Key, std::vector<uint32_t>, KeyHasher>::const_iterator iter = directory_hashtable_.find(key);
        if (iter != directory_hashtable_.end())
        {
            is_directory_exist = true;
            edge_idxes = iter->second;
        }
        else
        {
            is_directory_exist = false;
        }
        return;
    }

    bool CooperationWrapperBase::get(const Key& key, Value& value, bool& is_cooperative_cached)
    {
        assert(dht_wrapper_ptr_ != NULL);
        assert(edge_sendreq_toneighbor_socket_client_ptr_ != NULL);

        bool is_finish = false;

        // Update remote address of edge_sendreq_toneighbor_socket_client_ptr_ as the beacon node for the key
        locateBeaconNode_(key);

        // Lookup directory information at the beacon node
        bool is_directory_exist = false;
        uint32_t neighbor_edge_idx = 0;
        is_finish = lookupBeaconDirectory_(key, is_directory_exist, neighbor_edge_idx);
        if (is_finish) // Edge is NOT running
        {
            return is_finish;
        }

        if (is_directory_exist)
        {
            // TODO: Get data from the neighbor node based on the directory information and set is_cooperative_cached = true
        }
        else
        {
            // TODO: Or set is_cooperative_cached = false and return is_finish
        }

        return is_finish;
    }

    void CooperationWrapperBase::locateBeaconNode_(const Key& key)
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
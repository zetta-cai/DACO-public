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

        // NOTE: we use edge0 as default remote address, but we will reset remote address of the socket clients based on the key later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_port = Util::getEdgeRecvreqPort(0);
        NetworkAddr edge0_addr(edge0_ipstr, edge0_port);

        edge_sendreq_tobeacon_socket_client_ptr_  = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_addr);
        assert(edge_sendreq_tobeacon_socket_client_ptr_  != NULL);
        edge_sendreq_totarget_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_addr);
        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);

        // Allocate directory information
        directory_table_ptr_ = new DirectoryTable(edge_param_ptr_->getEdgeIdx());
        assert(directory_table_ptr_ != NULL);
    }

    CooperationWrapperBase::~CooperationWrapperBase()
    {
        // NOTE: no need to release edge_param_ptr_ which is maintained outside CooperationWrapperBase

        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);
        delete edge_sendreq_tobeacon_socket_client_ptr_ ;
        edge_sendreq_tobeacon_socket_client_ptr_ = NULL;

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);
        delete edge_sendreq_totarget_socket_client_ptr_;
        edge_sendreq_totarget_socket_client_ptr_ = NULL;

        // Release directory information
        assert(directory_table_ptr_ != NULL);
        delete directory_table_ptr_;
        directory_table_ptr_ = NULL;
    }

    bool CooperationWrapperBase::get(const Key& key, Value& value, bool& is_cooperative_cached)
    {
        bool is_finish = false;

        // Update remote address of edge_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = false;
        locateBeaconNode_(key, current_is_beacon);

        // Check if beacon node is the current edge node and lookup directory information
        bool is_directory_exist = false;
        DirectoryInfo directory_info;
        if (current_is_beacon) // Get target edge index from local directory information
        {
            lookupLocalDirectory(key, is_directory_exist, directory_info);
        }
        else // Get target edge index from remote directory information at the beacon node
        {
            is_finish = lookupBeaconDirectory_(key, is_directory_exist, directory_info);
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        }

        if (is_directory_exist) // The object is cached by some target edge node
        {
            // NOTE: the target node should not be the current edge node, as CooperationWrapperBase::get() can only be invoked if is_local_cached = false (i.e., the current edge node does not cache the object and hence is_directory_exist should be false)
            assert(edge_param_ptr_ != NULL);
            uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
            if (directory_info.getTargetEdgeIdx() == current_edge_idx)
            {
                std::ostringstream oss;
                oss << "current edge node " << current_edge_idx << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                Util::dumpWarnMsg(kClassName, oss.str());
                return is_finish; // NOTE: is_finish is still false, as edge is STILL running
            }

            // Update remote address of edge_sendreq_totarget_socket_client_ptr_ as the target edge node
            locateTargetNode_(directory_info);

            // Get data from the target edge node if any and update is_cooperative_cached
            is_finish = redirectGetToTarget_(key, value, is_cooperative_cached);
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        }
        else // The object is not cached by any target edge node
        {
            is_cooperative_cached = false;
        }

        return is_finish;
    }

    void CooperationWrapperBase::lookupLocalDirectory(const Key& key, bool& is_directory_exist, DirectoryInfo& directory_info)
    {
        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        assert(directory_table_ptr_ != NULL);
        directory_table_ptr_->lookup(key, is_directory_exist, directory_info);
        return;
    }

    bool CooperationWrapperBase::updateDirectory(const Key& key, const bool& is_admit)
    {
        bool is_finish = false;

        // Update remote address of edge_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = false;
        locateBeaconNode_(key, current_is_beacon);

        // Check if beacon node is the current edge node and update directory information
        assert(edge_param_ptr_ != NULL);
        DirectoryInfo directory_info(edge_param_ptr_->getEdgeIdx());
        if (current_is_beacon) // Update target edge index of local directory information
        {
            updateLocalDirectory(key, is_admit, directory_info);
        }
        else // Update remote directory information at the beacon node
        {
            is_finish = updateBeaconDirectory_(key, is_admit, directory_info);
        }

        return is_finish;
    }

    void CooperationWrapperBase::updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info)
    {
        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        assert(directory_table_ptr_ != NULL);
        directory_table_ptr_->update(key, is_admit, directory_info);
        return;
    }

    void CooperationWrapperBase::locateBeaconNode_(const Key& key, bool& current_is_beacon)
    {
        assert(dht_wrapper_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);

        // Check if the current edge node is the beacon node for the key
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        if (beacon_edge_idx == current_edge_idx)
        {
            current_is_beacon = true;
        }
        else
        {
            current_is_beacon = false;

            // Set remote address such that current edge node can communicate with the beacon node for the key
            std::string beacon_edge_ipstr = dht_wrapper_ptr_->getBeaconEdgeIpstr(key);
            uint16_t beacon_edge_port = dht_wrapper_ptr_->getBeaconEdgeRecvreqPort(key);
            NetworkAddr beacon_edge_addr(beacon_edge_ipstr, beacon_edge_port);
            edge_sendreq_tobeacon_socket_client_ptr_->setRemoteAddrForClient(beacon_edge_addr);
        }

        return;
    }

    void CooperationWrapperBase::locateTargetNode_(const DirectoryInfo& directory_info)
    {
        // The current edge node must NOT be the target node
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(directory_info.getTargetEdgeIdx() != current_edge_idx);

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);

        // Set remote address such that the current edge node can communicate with the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx);
        uint16_t target_edge_port = Util::getEdgeRecvreqPort(target_edge_idx);
        NetworkAddr target_edge_addr(target_edge_ipstr, target_edge_port);
        edge_sendreq_totarget_socket_client_ptr_->setRemoteAddrForClient(target_edge_addr);
        
        return;
    }

    void CooperationWrapperBase::verifyCurrentIsBeacon_(const Key& key) const
    {
        // The current edge node must be the beacon node for the key
        assert(dht_wrapper_ptr_ != NULL);
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(beacon_edge_idx == current_edge_idx);
        return;
    }

    void CooperationWrapperBase::verifyCurrentIsNotBeacon_(const Key& key) const
    {
        // The current edge node must be the beacon node for the key
        assert(dht_wrapper_ptr_ != NULL);
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(beacon_edge_idx != current_edge_idx);
        return;
    }
}
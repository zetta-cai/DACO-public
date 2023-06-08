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

        directory_hashtable_.clear();

        // Allocate randgen to choose target edge node from directory information
        directory_randgen_ptr_ = new std::mt19937_64(edge_param_ptr_->getEdgeIdx());
        assert(directory_randgen_ptr_ != NULL);
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

        // Release randgen
        assert(directory_randgen_ptr_ != NULL);
        delete directory_randgen_ptr_;
        directory_randgen_ptr_ = NULL;
    }

    bool CooperationWrapperBase::get(const Key& key, Value& value, bool& is_cooperative_cached)
    {
        bool is_finish = false;

        // Update remote address of edge_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = false;
        locateBeaconNode_(key, current_is_beacon);

        // Check if beacon node is the current edge node and lookup directory information
        bool is_directory_exist = false;
        uint32_t target_edge_idx = 0;
        if (current_is_beacon) // Get target edge index from local directory information
        {
            getTargetEdgeIdxFromLocalDirectory(key, is_directory_exist, target_edge_idx);
        }
        else // Get target edge index from remote directory information at the beacon node
        {
            is_finish = lookupBeaconDirectory_(key, is_directory_exist, target_edge_idx);
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
            if (target_edge_idx == current_edge_idx)
            {
                std::ostringstream oss;
                oss << "current edge node " << current_edge_idx << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                Util::dumpWarnMsg(kClassName, oss.str());
                return is_finish; // NOTE: is_finish is still false, as edge is STILL running
            }

            // Update remote address of edge_sendreq_totarget_socket_client_ptr_ as the target edge node
            locateTargetNode_(target_edge_idx);

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

    void CooperationWrapperBase::getTargetEdgeIdxFromLocalDirectory(const Key& key, bool& is_directory_exist, uint32_t& target_edge_idx)
    {
        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        // Check directory information cooperation_wrapper_ptr_
        std::unordered_set<uint32_t> edge_idxes;
        lookupLocalDirectory_(key, is_directory_exist, edge_idxes);

        // Get the target edge index
        if (is_directory_exist)
        {
            // Randomly select an edge node as the target edge node
            std::uniform_int_distribution<uint32_t> uniform_dist(0, edge_idxes.size() - 1); // Range from 0 to (# of edge indexes - 1)
            uint32_t random_number = uniform_dist(*directory_randgen_ptr_);
            assert(random_number < edge_idxes.size());
            uint32_t i = 0;
            for (std::unordered_set<uint32_t>::const_iterator iter = edge_idxes.begin(); iter != edge_idxes.end(); iter++)
            {
                if (i == random_number)
                {
                    target_edge_idx = *iter;
                    break;
                }
                i++;
            }
        }
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
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        if (current_is_beacon) // Update target edge index of local directory information
        {
            updateLocalDirectory_(key, is_admit, current_edge_idx);
        }
        else // Update remote directory information at the beacon node
        {
            is_finish = updateBeaconDirectory_(key, is_admit, current_edge_idx);
        }

        return is_finish;
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

    void CooperationWrapperBase::lookupLocalDirectory_(const Key& key, bool& is_directory_exist, std::unordered_set<uint32_t>& edge_idxes) const
    {
        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>::const_iterator iter = directory_hashtable_.find(key);
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

    void CooperationWrapperBase::locateTargetNode_(const uint32_t& target_edge_idx)
    {
        // The current edge node must NOT be the target node
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(target_edge_idx != current_edge_idx);

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);

        // Set remote address such that the current edge node can communicate with the target edge node
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx);
        uint16_t target_edge_port = Util::getEdgeRecvreqPort(target_edge_idx);
        NetworkAddr target_edge_addr(target_edge_ipstr, target_edge_port);
        edge_sendreq_totarget_socket_client_ptr_->setRemoteAddrForClient(target_edge_addr);
        
        return;
    }

    void CooperationWrapperBase::updateLocalDirectory_(const Key& key, const bool& is_admit, const uint32_t& target_edge_idx)
    {
        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>::iterator iter = directory_hashtable_.find(key);
        if (is_admit) // Add a new directory info
        {
            if (iter == directory_hashtable_.end()) // key does not exist
            {
                std::unordered_set<uint32_t> tmp_edge_idxes;
                tmp_edge_idxes.insert(target_edge_idx);
                directory_hashtable_.insert(std::pair<Key, std::unordered_set<uint32_t>>(key, tmp_edge_idxes));
            }
            else // key already exists
            {
                std::unordered_set<uint32_t>& tmp_edge_idxes = iter->second;
                if (tmp_edge_idxes.find(target_edge_idx) != tmp_edge_idxes.end()) // target_edge_idx exists for key
                {
                    std::ostringstream oss;
                    oss << "target edge index " << target_edge_idx << " already exists for key " << key.getKeystr() << " in updateLocalDirectory_() with is_admit = true!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
                else // target_edge_idx does not exist for key
                {
                    tmp_edge_idxes.insert(target_edge_idx);
                }
            }
        } // End of (is_admit == true)
        else // Delete an existing directory info
        {
            std::ostringstream oss;
            oss << "target edge index " << target_edge_idx << " does NOT exist for key " << key.getKeystr() << " in updateLocalDirectory_() with is_admit = false!";
            if (iter != directory_hashtable_.end()) // key already exists
            {
                std::unordered_set<uint32_t>& tmp_edge_idxes = iter->second;
                if (tmp_edge_idxes.find(target_edge_idx) != tmp_edge_idxes.end()) // target_edge_idx already exists
                {
                    tmp_edge_idxes.erase(target_edge_idx);
                }
                else // target_edge_idx does NOT exist
                {
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
            }
            else // key does NOT exist
            {
                Util::dumpWarnMsg(kClassName, oss.str());
            }
        } // ENd of (is_admit == false)
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
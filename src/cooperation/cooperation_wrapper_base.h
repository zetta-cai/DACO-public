/*
 * CooperationWrapperBase: the base class for cooperative edge caching.
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from target by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of local edge cache.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <unordered_map>
#include <random> // std::mt19937_64
#include <string>
#include <vector>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/dht_wrapper.h"
#include "edge/edge_param.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class CooperationWrapperBase
    {
    public:
        static CooperationWrapperBase* getCooperationWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr);

        CooperationWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~CooperationWrapperBase();

        // Return if edge node is finished
        // NOTE: get() cannot be const due to changing remote address of edge_sendreq_totarget_socket_client_ptr_
        bool get(const Key& key, Value& value, bool& is_cooperative_cached); // Get data from target edge ndoe
        void getTargetEdgeIdxForDirectoryLookupRequest(const Key& key, bool& is_directory_exist, uint32_t& target_edge_idx);
    private:
        static const std::string kClassName;
    protected:
        // Return if edge node is finished
        void locateBeaconNode_(const Key& key, bool& current_is_beacon);
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_directory_exist, uint32_t& target_edge_idx) = 0; // Check remote directory information at the beacon node
        void lookupLocalDirectory_(const Key& key, bool& is_directory_exist, std::vector<uint32_t>& edge_idxes) const; // Check local directory information
        void locateTargetNode_(const uint32_t& target_edge_idx);
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached) = 0;

        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase

        DhtWrapper* dht_wrapper_ptr_;
        UdpSocketWrapper* edge_sendreq_tobeacon_socket_client_ptr_;
        UdpSocketWrapper* edge_sendreq_totarget_socket_client_ptr_;
        std::unordered_map<Key, std::vector<uint32_t>, KeyHasher> directory_hashtable_; // Maintain directory information (not need ordered map)
        std::mt19937_64* directory_randgen_ptr_;
    };
}

#endif
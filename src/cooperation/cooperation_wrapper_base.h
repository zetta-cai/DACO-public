/*
 * CooperationWrapperBase: the base class for cooperative edge caching.
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from neighbor by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of local edge cache.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <unordered_map>
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

        // Directory information
        void lookupLocalDirectory(const Key& key, bool& is_directory_exist, std::vector<uint32_t>& edge_idxes) const; // Check if any edge node caches the key

        // Return if edge node is finished
        // NOTE: get() cannot be const due to changing remote address of edge_sendreq_toneighbor_socket_client_ptr_
        bool get(const Key& key, Value& value, bool& is_cooperative_cached); // Get data from neighbor edge ndoe
    private:
        static const std::string kClassName;
    protected:
        void locateBeaconNode_(const Key& key);
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_directory_exist, uint32_t& neighbor_edge_idx) = 0;

        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase

        DhtWrapper* dht_wrapper_ptr_;
        UdpSocketWrapper* edge_sendreq_toneighbor_socket_client_ptr_;
        std::unordered_map<Key, std::vector<uint32_t>, KeyHasher> directory_hashtable_; // Maintain directory information (not need ordered map)
    };
}

#endif
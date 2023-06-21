/*
 * DhtWrapper: a simple version of distributed hash table.
 *
 * NOTE: we assume that each edge node (DHT node) has IP addresses of all others (i.e., a Chord DHT with sufficient predecessors and successors to cover all DHT nodes)
 * 
 * By Siyuan Sheng (2023.06.05).
 */

#ifndef DHT_WRAPPER_H
#define DHT_WRAPPER_H

#include <string>

#include "common/key.h"
#include "edge/edge_param.h"
#include "hash/hash_wrapper_base.h"
#include "network/network_addr.h"

namespace covered
{
    class DhtWrapper
    {
    public:
        DhtWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr);
        ~DhtWrapper();

        uint32_t getBeaconEdgeIdx(const Key& key) const;
        std::string getBeaconEdgeIpstr(const Key& key) const;
        uint16_t getBeaconEdgeBeaconServerRecvreqPort(const Key& key) const;
    private:
        static const std::string kClassName;
        static const uint32_t DHT_HASH_RING_LENGTH;

        // DhtWrapper only uses edge index to specify instance_name_, yet not need to check if edge is running due to no network communication -> no need to maintain edge_param_ptr_
        std::string instance_name_;

        HashWrapperBase* hash_wrapper_ptr_;
    };
}

#endif
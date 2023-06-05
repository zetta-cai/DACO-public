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
#include "hash/hash_wrapper_base.h"
#include "network/network_addr.h"

namespace covered
{
    class DhtWrapper
    {
    public:
        DhtWrapper(const std::string& hash_name);
        ~DhtWrapper();

        uint32_t getBeaconEdgeIdx(const Key& key) const;
        std::string getBeaconEdgeIpstr(const Key& key) const;
        uint16_t getBeaconEdgeRecvreqPort(const Key& key) const;
    private:
        static const std::string kClassName;
        static const uint32_t DHT_HASH_RING_LENGTH;

        HashWrapperBase* hash_wrapper_ptr_;
    };
}

#endif
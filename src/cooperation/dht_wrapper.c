#include "cooperation/dht_wrapper.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string DhtWrapper::kClassName("DhtWrapper");
    const uint32_t DhtWrapper::DHT_HASH_RING_LENGTH = 1024 * 1024;

    DhtWrapper::DhtWrapper(const std::string& hash_name)
    {
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapper(hash_name);
        assert(hash_wrapper_ptr_ != NULL);
    }
    
    DhtWrapper::~DhtWrapper()
    {
        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    uint32_t DhtWrapper::getBeaconEdgeIdx(const Key& key) const
    {
        // Hash the key
        assert(hash_wrapper_ptr_ != NULL);
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t hash_ring_value = hash_value % DHT_HASH_RING_LENGTH;

        // Map edgecnt edge nodes into DHT hash ring
        uint32_t edgecnt = Param::getEdgecnt();
        uint32_t peredge_hash_ring_length = DHT_HASH_RING_LENGTH / edgecnt;

        // Calculate beacon node edge idx
        uint32_t beacon_edge_idx = hash_ring_value / peredge_hash_ring_length;
        if (beacon_edge_idx >= edgecnt)
        {
            beacon_edge_idx = edgecnt - 1; // Map the tail hash ring values to the last edge node
        }
        return beacon_edge_idx;
    }

    std::string DhtWrapper::getBeaconEdgeIpstr(const Key& key) const
    {
        uint32_t beacon_edge_idx = getBeaconEdgeIdx(key);
        return Config::getEdgeIpstr(beacon_edge_idx);
    }

    uint16_t DhtWrapper::getBeaconEdgeRecvreqPort(const Key& key) const
    {
        uint32_t beacon_edge_idx = getBeaconEdgeIdx(key);
        return Util::getEdgeRecvreqPort(beacon_edge_idx);
    }
}
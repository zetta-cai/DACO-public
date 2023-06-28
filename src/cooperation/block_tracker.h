/*
 * BlockTracker: track per-key cooperation metadata for MSI blocking (thread safe).
 *
 * NOTE: all non-const shared variables in BlockTracker and derived classes should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.19).
 */

#ifndef BLOCK_TRACKER_H
#define BLOCK_TRACKER_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "common/key.h"
#include "edge/edge_param.h"
#include "network/network_addr.h"

namespace covered
{
    class BlockTracker
    {
    public:
        BlockTracker(EdgeParam* edge_param_ptr);
        ~BlockTracker();

        // (1) Access per-key write flag

        bool isBeingWritten(const Key& key) const; // Return if key is being written
        bool checkAndSetWriteflag(const Key& key); // Atomically mark key is being written if not
        void resetWriteflag(const Key& key); // Atomically mark key is not being written

        // (2) Access per-key blocklist

        void block(const Key& key, const NetworkAddr& network_addr); // Add closest edge node into block list for the given key
        std::unordered_set<NetworkAddr, NetworkAddrHasher> unblock(const Key& key); // Pop all closest edge nodes from block list if key is not being written

        // (3) Get size for capacity check

        uint32_t getSizeForCapacity() const;
    private:
        typedef std::unordered_map<Key, bool, KeyHasher> perkey_writeflag_t;
        typedef std::unordered_map<Key, std::unordered_set<NetworkAddr, NetworkAddrHasher>, KeyHasher> perkey_edge_blocklist_t;

        static const std::string kClassName;

        // Const shared variables
        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase (thread safe)
        std::string instance_name_;

        // Non-const shared variables
        // NOTE: we do NOT merge write flags and blocklist as a single per-key metadata for easy development and debugging
        // NOTE: BOTH perkey_writeflags_ and perkey_edge_blocklist_ do NOT count key size as the managed keys have been counted in DirectoryTable
        perkey_writeflag_t perkey_writeflags_; // whether key is being written
        perkey_edge_blocklist_t perkey_edge_blocklist_; // a list of blocked closest edge nodes waiting for writes of each given key
    };
}

#endif
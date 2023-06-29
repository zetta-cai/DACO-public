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

#include "concurrency/concurrent_hashtable_impl.h"
#include "common/key.h"
#include "cooperation/msi/msi_metadata.h"
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

        bool isBeingWrittenForKey(const Key& key) const; // Return if key is being written
        // Atomically mark key is being written if not
        bool checkAndSetWriteflagForKey(const Key& key); // Add an MSI metadata with writeflag_ = true if key NOT exist
        // Atomically mark key is not being written
        void resetWriteflagForKeyIfExist(const Key& key); // Reset write flag only if key exists (NOT add an MSI metadata with writeflag_ = false is key NOT exist)

        // (2) Access per-key write flag and blocklist

        void blockEdgeForKeyIfExistAndBeingWritten(const Key& key, const NetworkAddr& network_addr, bool& is_being_written); // Add edge into block list for key only if key exists and being written
        std::unordered_set<NetworkAddr, NetworkAddrHasher> unblockAllEdgesForKeyIfNotWritten(const Key& key); // Pop all closest edge nodes from block list if key is not being written (TODO: trigger erase)

        // (3) Get size for capacity check

        uint32_t getSizeForCapacity() const;
    private:
        typedef std::unordered_map<Key, bool, KeyHasher> perkey_writeflag_t;
        typedef std::unordered_map<Key, std::unordered_set<NetworkAddr, NetworkAddrHasher>, KeyHasher> perkey_edge_blocklist_t;

        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Non-const shared variables
        // NOTE: perkey_msimetadata_ does NOT count key size, as the managed keys have been counted in DirectoryTable
        ConcurrentHashtable<MsiMetadata> perkey_msimetadata_; // per-key metadata for MSI protocol (thread safe)
    };
}

#endif
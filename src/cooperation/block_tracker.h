/*
 * BlockTracker: track per-key cooperation metadata for MSI blocking (thread safe).
 *
 * NOTE: BlockTracers assists with DirectoryTable in CooperationWrapperBase.
 * 
 * By Siyuan Sheng (2023.06.19).
 */

#ifndef BLOCK_TRACKER_H
#define BLOCK_TRACKER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "common/key.h"
#include "common/ring_buffer_impl.h"
#include "edge/edge_param.h"
#include "lock/rwlock.h"
#include "network/network_addr.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class BlockTracker
    {
    public:
        BlockTracker(EdgeParam* edge_param_ptr);
        ~BlockTracker();

        bool isBeingWritten(const Key& key) const; // Return if key is being written
        void block(const Key& key, const NetworkAddr& network_addr); // Add closest edge node into block list for the given key
        bool unblock(const Key& key); // Notify all closest edge nodes in block list to finish blocking for writes, and clear them from block list; return if edge is finished

        uint32_t getSizeForCapacity() const;
    private:
        typedef std::unordered_map<Key, bool, KeyHasher> perkey_writeflag_t;
        typedef std::unordered_map<Key, RingBuffer<NetworkAddr>, KeyHasher> perkey_edge_blocklist_t;

        static const std::string kClassName;

        // For unlock()
        bool notifyEdgesToFinishBlock_(const Key& key, const std::vector<NetworkAddr>& closest_edges) const; // Return if edge is finished
        void sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const;

        // Const shared variables
        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase
        std::string instance_name_;

        // NOTE: serializability for writes of the same key has been guaranteed in EdgeWrapperBase
        // Guarantee the atomicity of cooperation metadata (e.g., writes of different keys to update perkey_writeflags_)
        mutable Rwlock* rwlock_for_cooperation_metadata_ptr_;

        // Non-const shared variables
        // NOTE: we do NOT merge write flags and blocklist as a single per-key metadata for easy development and debugging
        // NOTE: BOTH perkey_writeflags_ and perkey_edge_blocklist_ do NOT count key size as the managed keys have been counted in DirectoryTable
        perkey_writeflag_t perkey_writeflags_; // whether key is being written
        perkey_edge_blocklist_t perkey_edge_blocklist_; // a list of blocked closest edge nodes waiting for writes of each given key

        // Non-const individual variables
        UdpSocketWrapper* edge_sendreq_toclosest_cache_server_socket_client_ptr_;
    };
}

#endif
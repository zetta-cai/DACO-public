/*
 * CooperationWrapperBase: the base class for cooperative edge caching (thread safe).
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from a target edge node by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of the closest edge cache.
 * 
 * NOTE: all non-const shared variables in CooperationWrapperBase and derived classes should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <string>
#include <unordered_set>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/block_tracker.h"
#include "cooperation/dht_wrapper.h"
#include "cooperation/directory_info.h"
#include "cooperation/directory_table.h"
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

        // (1) Get data from target edge node

        bool get(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) const; // Return if edge node is finished
        void lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const; // Check local directory information

        // (2) Update content directory information

        bool updateDirectory(const Key& key, const bool& is_admit, bool& is_being_written); // Return if edge node is finished
        void updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written); // Update local directory information

        // (3) Blocking for MSI protocol

        // Buffer closest edge nodes waiting for writes
        // NOTE: the blocked edge nodes will be notified after writes
        void addEdgeIntoBlocklist(const Key& key, const NetworkAddr& network_addr);
        void tryToNotifyEdgesFromBlocklist(const Key& key);

        // (4) Get size for capacity check

        uint32_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // (1) Get data from target edge node

        // Return if edge node is finished
        void locateBeaconNode_(const Key& key, bool& current_is_beacon) const;
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0; // Check remote directory information at the beacon node
        void locateTargetNode_(const DirectoryInfo& directory_info) const;
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const = 0;

        // (2) Update content directory information

        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) = 0;

        // (3) Blocking for MSI protocols

        bool blockForWritesByInterruption_(const Key& key) const;
        bool notifyEdgesToFinishBlock_(const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& closest_edges) const; // Return if edge is finished
        virtual void sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const = 0;

        // Const shared variables
        std::string base_instance_name_;
        DhtWrapper* dht_wrapper_ptr_;

        // Non-const shared variables (cooperation metadata)
        DirectoryTable* directory_table_ptr_; // per-key content directory infos (thread safe)
        BlockTracker block_tracker_; // per-key cooperation metadata (thread safe)
        
    protected:
        // (5) Verification

        // Edge index verification
        void verifyCurrentIsBeacon_(const Key& key) const;
        void verifyCurrentIsNotBeacon_(const Key& key) const;

        // Const shared variables
        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase (thread safe)

        // Non-const individual variables (NOTE: NOT thread safe -> TODO: END HERE)
        UdpSocketWrapper* edge_sendreq_cache_server_tobeacon_socket_client_ptr_;
        UdpSocketWrapper* edge_sendreq_cache_server_totarget_socket_client_ptr_;
        UdpSocketWrapper* edge_sendreq_beacon_server_toclosest_socket_client_ptr_;
    };
}

#endif
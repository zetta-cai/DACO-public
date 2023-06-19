/*
 * CooperationWrapperBase: the base class for cooperative edge caching (thread safe).
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from a target edge node by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of the closest edge cache.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <string>

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

        // Return if edge node is finished
        bool get(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) const; // Get data from target edge ndoe
        void lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const; // Check local directory information
        bool updateDirectory(const Key& key, const bool& is_admit, bool& is_being_written); // Update remote directory info at beacon node
        void updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written);

        // Buffer closest edge nodes waiting for writes
        // NOTE: the blocked edge nodes will be notified after writes
        void addEdgeIntoBlocklist(const Key& key, const NetworkAddr& network_addr);
        void tryToNotifyEdgesFromBlocklist(const Key& key);

        uint32_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;
        
        std::string base_instance_name_;

        // For get()
        // Return if edge node is finished
        void locateBeaconNode_(const Key& key, bool& current_is_beacon) const;
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const = 0; // Check remote directory information at the beacon node
        bool blockForWritesByInterruption_(const Key& key) const; // TODO: END HERE
        void locateTargetNode_(const DirectoryInfo& directory_info) const;
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const = 0;

        // For updateDirectory()
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) = 0; // TODO: implement in basic

        // Const shared variables
        DhtWrapper* dht_wrapper_ptr_;

        // Non-const shared variables (cooperation metadata)
        DirectoryTable* directory_table_ptr_; // per-key content directory infos (thread safe)
        BlockTracker block_tracker_; // per-key cooperation metadata (thread safe)
        
    protected:
        // Edge index verification
        void verifyCurrentIsBeacon_(const Key& key) const;
        void verifyCurrentIsNotBeacon_(const Key& key) const;

        // Const shared variables
        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase

        // Non-const individual variables
        UdpSocketWrapper* edge_sendreq_tobeacon_socket_client_ptr_;
        UdpSocketWrapper* edge_sendreq_totarget_socket_client_ptr_;
    };
}

#endif
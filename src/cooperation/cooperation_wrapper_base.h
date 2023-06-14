/*
 * CooperationWrapperBase: the base class for cooperative edge caching.
 *
 * Basic or COVERED CooperativeCacheWrapper is responsible for checking directory information at beacon node located by DhtWrapper, getting data from a target edge node by request redirection, and synchronizing directory information at beacon node after cache admission/eviction of the closest edge cache.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef COOPERATION_WRAPPER_BASE_H
#define COOPERATION_WRAPPER_BASE_H

#include <string>
#include <vector>

#include "common/key.h"
#include "common/value.h"
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
        // NOTE: get() cannot be const due to changing remote address of edge_sendreq_totarget_socket_client_ptr_
        bool get(const Key& key, Value& value, bool& is_cooperative_cached_and_valid); // Get data from target edge ndoe
        void lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info); // Check local directory information
        bool updateDirectory(const Key& key, const bool& is_admit); // Update remote directory info at beacon node
        void updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info);
    private:
        static const std::string kClassName;
        
        std::string base_instance_name_;
    protected:
        // For get()
        // Return if edge node is finished
        void locateBeaconNode_(const Key& key, bool& current_is_beacon);
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) = 0; // Check remote directory information at the beacon node
        void locateTargetNode_(const DirectoryInfo& directory_info);
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) = 0;

        // For updateDirectory()
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info) = 0; // TODO: implement in basic

        // Edge index verification
        void verifyCurrentIsBeacon_(const Key& key) const;
        void verifyCurrentIsNotBeacon_(const Key& key) const;

        EdgeParam* edge_param_ptr_; // Maintained outside CooperativeCacheWrapperBase

        DhtWrapper* dht_wrapper_ptr_;
        UdpSocketWrapper* edge_sendreq_tobeacon_socket_client_ptr_;
        UdpSocketWrapper* edge_sendreq_totarget_socket_client_ptr_;
        DirectoryTable* directory_table_ptr_;
    };
}

#endif
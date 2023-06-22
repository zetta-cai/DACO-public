/*
 * BasicCooperationWrapper: basic cooperative caching framework for baselines.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef BASIC_COOPERATION_WRAPPER_H
#define BASIC_COOPERATION_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "cooperation/directory_info.h"

namespace covered
{
    class BasicCooperationWrapper : public CooperationWrapperBase
    {
    public:
        BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~BasicCooperationWrapper();
    private:
        static const std::string kClassName;

        // (1) Get data from target edge node
        
        virtual bool lookupBeaconDirectory_(UdpSocketWrapper* sendreq_tobeacon_socket_client_ptr, const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const override;
        virtual bool redirectGetToTarget_(UdpSocketWrapper* sendreq_totarget_socket_client_ptr, const Key& key, Value& value, bool& is_cooperative_cached, bool& is_valid) const override;

        // (2) Update content directory information

        virtual bool updateBeaconDirectory_(UdpSocketWrapper* sendreq_tobeacon_socket_client_ptr, const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written) override;

        // (3) Blocking for MSI protocols

        virtual void sendFinishBlockRequest_(UdpSocketWrapper* sendreq_toclosest_socket_client_ptr, const Key& key, const NetworkAddr& closest_edge_addr) const override;

        std::string instance_name_;
    };
}

#endif
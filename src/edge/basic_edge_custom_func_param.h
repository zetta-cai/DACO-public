/*
 * Different custom function parameters of baselines for edge module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef BASIC_EDGE_CUSTOM_FUNC_PARAM_H
#define BASIC_EDGE_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "common/bandwidth_usage.h"
#include "common/key.h"
#include "common/value.h"
#include "cooperation/directory/directory_info.h"
#include "edge/edge_custom_func_param_base.h"
#include "event/event_list.h"
#include "message/extra_common_msghdr.h"
#include "message/message_base.h"

namespace covered
{
    // TriggerBestGuessPlacementFuncParam for edge cache server worker of BestGuess

    class TriggerBestGuessPlacementFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // trigger best-guess placement/replacement policy

        TriggerBestGuessPlacementFuncParam(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr);
        virtual ~TriggerBestGuessPlacementFuncParam();

        const Key& getKeyConstRef() const;
        const Value& getValueConstRef() const;
        BandwidthUsage& getTotalBandwidthUsageRef() const;
        EventList& getEventListRef() const;
        ExtraCommonMsghdr getExtraCommonMsghdr() const;

        bool isFinish() const;
        void setIsFinish(const bool& is_finish);
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const Value& value_const_ref_;
        BandwidthUsage& bandwidth_usage_ref_;
        EventList& event_list_ref_;
        const ExtraCommonMsghdr extra_common_msghdr_;

        bool is_finish_;
    };

    // ProcessPlacementTriggerRequestForBestGuessFuncParam for edge beacon server of BestGuess

    class ProcessPlacementTriggerRequestForBestGuessFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // process placement trigger request for best-guess placement/replacement policy

        ProcessPlacementTriggerRequestForBestGuessFuncParam(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr);
        ~ProcessPlacementTriggerRequestForBestGuessFuncParam();

        MessageBase* getControlRequestPtr() const;
        const NetworkAddr& getEdgeCacheServerWorkerRecvRspDstAddrConstRef() const;

        bool& isFinishRef();
        const bool& isFinishConstRef() const;
    private:
        static const std::string kClassName;

        MessageBase* control_request_ptr_;
        const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr_const_ref_;

        bool is_finish_;
    };

    // ClusterForMagnetFuncParam for edge cache server worker and edge beacon server of MagNet

    class ClusterForMagnetFuncParam : public EdgeCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // perform clustering for object accesses for MagNet

        ClusterForMagnetFuncParam(const Key& key, const std::list<DirectoryInfo>& dirinfo_set, const bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info);
        ~ClusterForMagnetFuncParam();

        const Key& getKeyConstRef() const;
        const std::list<DirectoryInfo>& getDirinfoSetConstRef() const;
        const bool& isBeingWrittenConstRef() const;
        bool& isValidDirectoryExistRef();
        DirectoryInfo& getDirectoryInfoRef();
    private:
        static const std::string kClassName;

        const Key& key_const_ref_;
        const std::list<DirectoryInfo>& dirinfo_set_const_ref_;
        const bool& is_being_written_const_ref_;

        bool& is_valid_directory_exist_ref_;
        DirectoryInfo& directory_info_ref_;
    };
}

#endif
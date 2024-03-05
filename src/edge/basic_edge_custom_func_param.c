#include "edge/basic_edge_custom_func_param.h"

namespace covered
{
    // TriggerBestGuessPlacementFuncParam for edge cache server of BestGuess

    const std::string TriggerBestGuessPlacementFuncParam::kClassName("TriggerBestGuessPlacementFuncParam");

    const std::string TriggerBestGuessPlacementFuncParam::FUNCNAME("trigger_best_guess_placement");

    TriggerBestGuessPlacementFuncParam::TriggerBestGuessPlacementFuncParam(const Key& key, const Value& value, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : EdgeCustomFuncParamBase(), key_const_ref_(key), value_const_ref_(value), bandwidth_usage_ref_(total_bandwidth_usage), event_list_ref_(event_list), extra_common_msghdr_(extra_common_msghdr)
    {
        is_finish_ = false;
    }

    TriggerBestGuessPlacementFuncParam::~TriggerBestGuessPlacementFuncParam()
    {
    }

    const Key& TriggerBestGuessPlacementFuncParam::getKeyConstRef() const
    {
        return key_const_ref_;
    }

    const Value& TriggerBestGuessPlacementFuncParam::getValueConstRef() const
    {
        return value_const_ref_;
    }

    BandwidthUsage& TriggerBestGuessPlacementFuncParam::getTotalBandwidthUsageRef() const
    {
        return bandwidth_usage_ref_;
    }

    EventList& TriggerBestGuessPlacementFuncParam::getEventListRef() const
    {
        return event_list_ref_;
    }

    ExtraCommonMsghdr TriggerBestGuessPlacementFuncParam::getExtraCommonMsghdr() const
    {
        return extra_common_msghdr_;
    }

    bool TriggerBestGuessPlacementFuncParam::isFinish() const
    {
        return is_finish_;
    }

    void TriggerBestGuessPlacementFuncParam::setIsFinish(const bool& is_finish)
    {
        is_finish_ = is_finish;
        return;
    }

    // ProcessPlacementTriggerRequestForBestGuessFuncParam for edge beacon server of BestGuess

    const std::string ProcessPlacementTriggerRequestForBestGuessFuncParam::kClassName("ProcessPlacementTriggerRequestForBestGuessFuncParam");

    const std::string ProcessPlacementTriggerRequestForBestGuessFuncParam::FUNCNAME("process_placement_trigger_request_for_best_guess");

    ProcessPlacementTriggerRequestForBestGuessFuncParam::ProcessPlacementTriggerRequestForBestGuessFuncParam(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr) : EdgeCustomFuncParamBase(), control_request_ptr_(control_request_ptr), edge_cache_server_worker_recvrsp_dst_addr_const_ref_(edge_cache_server_worker_recvrsp_dst_addr)
    {
        is_finish_ = false;
    }

    ProcessPlacementTriggerRequestForBestGuessFuncParam::~ProcessPlacementTriggerRequestForBestGuessFuncParam()
    {}

    MessageBase* ProcessPlacementTriggerRequestForBestGuessFuncParam::getControlRequestPtr() const
    {
        return control_request_ptr_;
    }

    const NetworkAddr& ProcessPlacementTriggerRequestForBestGuessFuncParam::getEdgeCacheServerWorkerRecvRspDstAddrConstRef() const
    {
        return edge_cache_server_worker_recvrsp_dst_addr_const_ref_;
    }

    bool& ProcessPlacementTriggerRequestForBestGuessFuncParam::isFinishRef()
    {
        return is_finish_;
    }

    const bool& ProcessPlacementTriggerRequestForBestGuessFuncParam::isFinishConstRef() const
    {
        return is_finish_;
    }
}
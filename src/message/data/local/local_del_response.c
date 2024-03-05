#include "message/data/local/local_del_response.h"

namespace covered
{
    const std::string LocalDelResponse::kClassName("LocalDelResponse");

    LocalDelResponse::LocalDelResponse(const Key& key, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyHitflagUtilizationMessage(key, hitflag, cache_size_bytes, cache_capacity_bytes, MessageType::kLocalDelResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    LocalDelResponse::LocalDelResponse(const DynamicArray& msg_payload) : KeyHitflagUtilizationMessage(msg_payload)
    {
    }

    LocalDelResponse::~LocalDelResponse() {}

    /*Hitflag LocalDelResponse::getHitflag() const
    {
        uint8_t byte = getByte_();
        Hitflag hitflag = static_cast<Hitflag>(byte);
        return hitflag;
    }*/
}
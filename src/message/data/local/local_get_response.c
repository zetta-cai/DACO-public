#include "message/data/local/local_get_response.h"

namespace covered
{
    const std::string LocalGetResponse::kClassName("LocalGetResponse");

    LocalGetResponse::LocalGetResponse(const Key& key, const Value& value, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) : KeyValueHitflagUtilizationMessage(key, value, hitflag, cache_size_bytes, cache_capacity_bytes, MessageType::kLocalGetResponse, source_index, source_addr, bandwidth_usage, event_list, extra_common_msghdr)
    {
    }

    LocalGetResponse::LocalGetResponse(const DynamicArray& msg_payload) : KeyValueHitflagUtilizationMessage(msg_payload)
    {
    }

    LocalGetResponse::~LocalGetResponse() {}
}
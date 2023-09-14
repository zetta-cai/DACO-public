#include "message/data/local/local_put_response.h"

namespace covered
{
    const std::string LocalPutResponse::kClassName("LocalPutResponse");

    LocalPutResponse::LocalPutResponse(const Key& key, const Hitflag& hitflag, const uint64_t& cache_size_bytes, const uint64_t& cache_capacity_bytes, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency) : KeyHitflagUtilizationMessage(key, hitflag, cache_size_bytes, cache_capacity_bytes, MessageType::kLocalPutResponse, source_index, source_addr, event_list, skip_propagation_latency)
    {
    }

    LocalPutResponse::LocalPutResponse(const DynamicArray& msg_payload) : KeyHitflagUtilizationMessage(msg_payload)
    {
    }

    LocalPutResponse::~LocalPutResponse() {}

    /*Hitflag LocalPutResponse::getHitflag() const
    {
        uint8_t byte = getByte_();
        Hitflag hitflag = static_cast<Hitflag>(byte);
        return hitflag;
    }*/
}
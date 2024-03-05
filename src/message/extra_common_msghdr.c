#include "message/extra_common_msghdr.h"

#include "common/util.h"

namespace covered
{
    const std::string ExtraCommonMsghdr::kClassName("ExtraCommonMsghdr");

    ExtraCommonMsghdr::ExtraCommonMsghdr()
    {
        skip_propagation_latency_ = false;
        is_monitored_ = false;
    }

    ExtraCommonMsghdr::ExtraCommonMsghdr(const ExtraCommonMsghdr &other)
    {
        *this = other;
    }

    ExtraCommonMsghdr::ExtraCommonMsghdr(const bool& skip_propagation_latency, const bool& is_monitored)
    {
        skip_propagation_latency_ = skip_propagation_latency;
        is_monitored_ = is_monitored;
    }

    ExtraCommonMsghdr::~ExtraCommonMsghdr()
    {
    }

    std::string ExtraCommonMsghdr::toString() const
    {
        std::ostringstream oss;
        oss << "skip propagation latency = " << Util::toString(skip_propagation_latency_) << "; is monitored = " << Util::toString(is_monitored_);
        return oss.str();
    }

    bool ExtraCommonMsghdr::isSkipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    bool ExtraCommonMsghdr::isMonitored() const
    {
        return is_monitored_;
    }

    uint32_t ExtraCommonMsghdr::getExtraCommonMsghdrPayloadSize() const
    {
        // skip propagation latency flag + is monitored flag
        return sizeof(bool) + sizeof(bool);
    }

    uint32_t ExtraCommonMsghdr::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&skip_propagation_latency_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.deserialize(size, (const char*)&is_monitored_, sizeof(bool));
        size += sizeof(bool);
        return size - position;
    }

    uint32_t ExtraCommonMsghdr::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char *)&skip_propagation_latency_, sizeof(bool));
        size += sizeof(bool);
        msg_payload.serialize(size, (char *)&is_monitored_, sizeof(bool));
        size += sizeof(bool);
        return size - position;
    }
}
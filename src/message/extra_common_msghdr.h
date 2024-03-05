/*
 * ExtraCommonMsghdr: the base class for extra common message headers, e.g., skip propagation latency flag and is monitored flag.
 * 
 * By Siyuan Sheng (2024.03.05).
 */

#ifndef EXTRA_COMMON_MSGHDR_H
#define EXTRA_COMMON_MSGHDR_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class ExtraCommonMsghdr
    {
    public:
        ExtraCommonMsghdr();
        ExtraCommonMsghdr(const ExtraCommonMsghdr &other);
        ExtraCommonMsghdr(const bool& skip_propagation_latency, const bool& is_monitored);
        ~ExtraCommonMsghdr();

        std::string toString() const;

        bool isSkipPropagationLatency() const;
        bool isMonitored() const;

        uint32_t getExtraCommonMsghdrPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const ExtraCommonMsghdr& operator=(const ExtraCommonMsghdr& other);
    private:
        static const std::string kClassName;

        bool skip_propagation_latency_; // NOT simulate propagation latency for warmup speedup (also skip disk I/O latency in cloud)
        bool is_monitored_; // Whether the message is monitored (i.e., need to dump information for debugging)
    };
}

#endif
/*
 * Value: a general value with variable length to encapsulate underlying workload generators.
 *
 * NOTE: we embed value content up to MAX_VALUE_CONTENT_SIZE bytes into network packets to avoid UDP buffer overflow, which is just an impl trick and acceptable due to the following reasons:
 * (i) Too large value sizes are extremely rare in real-world traces, while most value sizes are just in units of KiB (see real-world traces);
 * (ii) System performance bottleneck of geo-distributed tiered storage is propagation latency instead of emission latency for most objects and thus most hot objects -> NOT affect evaluation performance results -> actually if emission latency is bottleneck, both edge caching and cooperative caching make NO sense;
 * (iii) We still use original value size to calculate bandwidth cost just like transmitting complete value content -> NOT affect evaluation bandwidth results;
 * (iv) Using other streaming-based network protocols (e.g., TCP or HTTPS) can solve this issue, yet it is just time-consuming engineering work and orthogonal with our problem design (leave in the future work).
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef VALUE_H
#define VALUE_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class Value
    {
    public:
        static const uint32_t MAX_VALUE_CONTENT_SIZE; // See NOTE in the header file

        Value();
        Value(const uint32_t& valuesize);
        ~Value();

        bool isDeleted() const;
        //void remove();
        
        uint32_t getValuesize() const;
        std::string generateValuestrForStorage() const; // Generate value string as value content for edge/cloud storage (valuesize_ bytes)

        // Offset of value (position) is dynamically changed for different keys in message payload
        // -> If is_space_efficient = true (to dump/load dataset/workload file of replayed traces), value content will NOT consume space
        // -> If is_space_efficient = false (to serialize/deserialize network packets), value content will consume space (up to MAX_VALUE_CONTENT_SIZE bytes)
        uint32_t getValuePayloadSize(const bool& is_space_efficient = false) const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient = false) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient = false);
        uint32_t deserialize(std::fstream* fs_ptr, const bool& is_space_efficient = false);

        const Value& operator=(const Value& other);
    private:
        static const std::string kClassName;

        std::string generateValuestrForNetwork_() const; // Generate value string as value content for network packets (up to MAX_VALUE_CONTENT_SIZE bytes)

        static uint32_t getValuesizeForNetwork_(const uint32_t& value_size); // Get value size for network packets (up to MAX_VALUE_CONTENT_SIZE bytes)
        static std::string generateValuestr_(const uint32_t& value_size); // Generate value string as value content

        bool is_deleted_;
        uint32_t valuesize_;
    };
}

#endif
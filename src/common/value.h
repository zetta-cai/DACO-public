/*
 * Value: a general value with variable length to encapsulate underlying workload generators.
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
        Value();
        Value(const uint32_t& valuesize);
        ~Value();

        bool isDeleted() const;
        //void remove();
        
        uint32_t getValuesize() const;
        std::string generateValuestr() const; // Generate value string as value content for serialization

        // Offset of value (position) is dynamically changed for different keys in message payload
        // NOTE: value content will NOT consume space if is_space_efficient = true
        uint32_t getValuePayloadSize(const bool& is_space_efficient = false) const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient = false) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position, const bool& is_space_efficient = false);
        uint32_t deserialize(std::fstream* fs_ptr, const bool& is_space_efficient = false);

        const Value& operator=(const Value& other);
    private:
        static const std::string kClassName;

        bool is_deleted_;
        uint32_t valuesize_;
    };
}

#endif
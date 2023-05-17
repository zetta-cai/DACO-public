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

        // Offset of value (position) is dynamically changed for different keys in message payload
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position);
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);
    private:
        static const std::string kClassName;

        bool is_deleted_;
        uint32_t valuesize_;
    };
}

#endif
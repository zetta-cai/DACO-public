/*
 * Key: a general key with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef KEY_H
#define KEY_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class Key
    {
    public:
        Key();
        Key(const std::string& keystr);
        ~Key();

        std::string getKeystr() const;

        // Offset of key must be 0 in message payload
        uint32_t serialize(DynamicArray& msg_payload);
        uint32_t deserialize(const DynamicArray& msg_payload);
    private:
        static const std::string kClassName;

        std::string keystr_;
    };
}

#endif
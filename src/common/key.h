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
        uint32_t getKeyPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        bool operator<(const Key& other) const; // To be used as key in std::map
        bool operator==(const Key& other) const; // To be used by key in std::unordered_map
        Key& operator=(const Key& other);
    private:
        static const std::string kClassName;

        std::string keystr_;
    };

    // To be used by key in std::unordered_map
    class KeyHasher
    {
    public:
        size_t operator()(const Key& key) const
        {
            return std::hash<std::string>{}(key.getKeystr());
        }
    };
}

#endif
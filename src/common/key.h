/*
 * Key: a general key with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef KEY_H
#define KEY_H

#include <list>
#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class Key
    {
    public:
        static std::list<std::pair<uint32_t, std::list<Key>>>::iterator findKeyListForEdge(const uint32_t& edge_idx, std::list<std::pair<uint32_t, std::list<Key>>>& peredge_keyset);
        static std::list<std::pair<uint32_t, std::list<Key>>>::const_iterator findKeyListForEdge(const uint32_t& edge_idx, const std::list<std::pair<uint32_t, std::list<Key>>>& peredge_keyset);

        Key();
        Key(const std::string& keystr);
        ~Key();

        uint32_t getKeyLength() const;
        std::string getKeystr() const;

        uint32_t getKeyPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        bool operator<(const Key& other) const; // To be used as key in std::map and std::set
        bool operator==(const Key& other) const; // To be used as key in std::unordered_map
        bool operator!=(const Key& other) const; // To be used as key in TommyDS
        const Key& operator=(const Key& other);
    private:
        static const std::string kClassName;

        std::string keystr_;
    };

    // To be used by key in std::unordered_map
    class KeyHasher
    {
    public:
        size_t operator()(const Key& key) const;
    };
}

#endif
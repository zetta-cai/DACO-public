/*
 * Edgeset: a set of edge nodes for placement.
 *
 * By Siyuan Sheng (2023.09.24).
 */

#ifndef EDGESET_H
#define EDGESET_H

#include <list>
#include <string>
#include <unordered_set>

#include "common/dynamic_array.h"
#include "common/key.h"

namespace covered
{
    class Edgeset
    {
    public:
        static std::list<std::pair<Key, Edgeset>>::iterator findEdgesetForKey(const Key& key, std::list<std::pair<Key, Edgeset>>& perkey_edgeset);
        static std::list<std::pair<Key, Edgeset>>::const_iterator findEdgesetForKey(const Key& key, const std::list<std::pair<Key, Edgeset>>& perkey_edgeset);

        Edgeset();
        Edgeset(const std::unordered_set<uint32_t>& edgeset);
        ~Edgeset();

        //const std::unordered_set<uint32_t>& getEdgesetRef() const;

        uint32_t size() const;
        std::unordered_set<uint32_t>::iterator find(const uint32_t& edge_idx);
        std::unordered_set<uint32_t>::iterator begin();
        std::unordered_set<uint32_t>::iterator end();
        std::unordered_set<uint32_t>::const_iterator find(const uint32_t& edge_idx) const;
        std::unordered_set<uint32_t>::const_iterator begin() const;
        std::unordered_set<uint32_t>::const_iterator end() const;

        void clear();
        std::pair<std::unordered_set<uint32_t>::iterator, bool> insert(const uint32_t& edge_idx);
        void erase(const uint32_t& edge_idx);
        void erase(std::unordered_set<uint32_t>::const_iterator iter);

        uint32_t getEdgesetPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        std::string toString() const;

        const Edgeset& operator=(const Edgeset& other);
    private:
        static const std::string kClassName;

        std::unordered_set<uint32_t> edgeset_;
    };
}

#endif
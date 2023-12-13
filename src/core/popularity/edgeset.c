#include "core/popularity/edgeset.h"

#include <sstream>

namespace covered
{
    const std::string Edgeset::kClassName("Edgeset");

    Edgeset::Edgeset()
    {
        edgeset_.clear();
    }

    Edgeset::Edgeset(const std::unordered_set<uint32_t>& edgeset)
    {
        edgeset_ = edgeset;
    }

    Edgeset::~Edgeset() {}

    /*const std::unordered_set<uint32_t>& Edgeset::getEdgesetRef() const
    {
        return edgeset_;
    }*/

    uint32_t Edgeset::size() const
    {
        return edgeset_.size();
    }

    std::unordered_set<uint32_t>::iterator Edgeset::find(const uint32_t& edge_idx)
    {
        return edgeset_.find(edge_idx);
    }

    std::unordered_set<uint32_t>::iterator Edgeset::begin()
    {
        return edgeset_.begin();
    }

    std::unordered_set<uint32_t>::iterator Edgeset::end()
    {
        return edgeset_.end();
    }

    std::unordered_set<uint32_t>::const_iterator Edgeset::find(const uint32_t& edge_idx) const
    {
        return edgeset_.find(edge_idx);
    }

    std::unordered_set<uint32_t>::const_iterator Edgeset::begin() const
    {
        return edgeset_.begin();
    }

    std::unordered_set<uint32_t>::const_iterator Edgeset::end() const
    {
        return edgeset_.end();
    }

    void Edgeset::clear()
    {
        edgeset_.clear();
        return;
    }

    std::pair<std::unordered_set<uint32_t>::iterator, bool> Edgeset::insert(const uint32_t& edge_idx)
    {
        std::pair<std::unordered_set<uint32_t>::iterator, bool> result = edgeset_.insert(edge_idx);
        return result;
    }

    void Edgeset::erase(const uint32_t& edge_idx)
    {
        edgeset_.erase(edge_idx);
        return;
    }

    void Edgeset::erase(std::unordered_set<uint32_t>::const_iterator iter)
    {
        edgeset_.erase(iter);
        return;
    }

    uint32_t Edgeset::getEdgesetPayloadSize() const
    {
        uint32_t edgeset_payload_size = sizeof(uint32_t) + edgeset_.size() * sizeof(uint32_t);
        return edgeset_payload_size;
    }

    uint32_t Edgeset::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t edgeset_size = edgeset_.size();
        msg_payload.deserialize(size, (const char*)&edgeset_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        for (std::unordered_set<uint32_t>::const_iterator edgeset_const_iter = edgeset_.begin(); edgeset_const_iter != edgeset_.end(); edgeset_const_iter++)
        {
            uint32_t edge_idx = *edgeset_const_iter;
            msg_payload.deserialize(size, (const char*)&edge_idx, sizeof(uint32_t));
            size += sizeof(uint32_t);
        }
        return size - position;
    }

    uint32_t Edgeset::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t edgeset_size = 0;
        msg_payload.serialize(size, (char*)&edgeset_size, sizeof(uint32_t));
        size += sizeof(uint32_t);
        for (uint32_t i = 0; i < edgeset_size; i++)
        {
            uint32_t edge_idx = 0;
            msg_payload.serialize(size, (char*)&edge_idx, sizeof(uint32_t));
            size += sizeof(uint32_t);
            edgeset_.insert(edge_idx);
        }
        return size - position;
    }

    std::string Edgeset::toString() const
    {
        std::ostringstream oss;
        oss << "edgeset size: " << edgeset_.size();
        uint32_t i = 0;
        for (std::unordered_set<uint32_t>::const_iterator edgeset_const_iter = edgeset_.begin(); edgeset_const_iter != edgeset_.end(); edgeset_const_iter++)
        {
            uint32_t edge_idx = *edgeset_const_iter;
            oss << "; edge[" << i << "]: " << edge_idx;
            i++;
        }
        return oss.str();
    }

    const Edgeset& Edgeset::operator=(const Edgeset& other)
    {
        edgeset_ = other.edgeset_;
        return *this;
    }
}
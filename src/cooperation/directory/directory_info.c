#include "cooperation/directory/directory_info.h"

#include <arpa/inet.h> // htonl ntohl

namespace covered
{
    const std::string DirectoryInfo::kClassName("DirectoryInfo");

    std::list<DirectoryInfo>::iterator DirectoryInfo::findDirinfoFromList(const DirectoryInfo& dirinfo, std::list<DirectoryInfo>& dirinfo_list)
    {
        std::list<DirectoryInfo>::iterator iter = dirinfo_list.begin();
        for (; iter != dirinfo_list.end(); iter++)
        {
            if (*iter == dirinfo)
            {
                break;
            }
        }
        return iter;
    }

    std::list<DirectoryInfo>::const_iterator DirectoryInfo::findDirinfoFromList(const DirectoryInfo& dirinfo, const std::list<DirectoryInfo>& dirinfo_list)
    {
        std::list<DirectoryInfo>::const_iterator iter = dirinfo_list.begin();
        for (; iter != dirinfo_list.end(); iter++)
        {
            if (*iter == dirinfo)
            {
                break;
            }
        }
        return iter;
    }

    DirectoryInfo::DirectoryInfo()
    {
        target_edge_idx_ = 0;
    }

    DirectoryInfo::DirectoryInfo(const uint32_t& target_edge_idx)
    {
        setTargetEdgeIdx(target_edge_idx);
    }
    
    DirectoryInfo::~DirectoryInfo() {}

    uint32_t DirectoryInfo::getTargetEdgeIdx() const
    {
        return target_edge_idx_;
    }

    void DirectoryInfo::setTargetEdgeIdx(const uint32_t& target_edge_idx)
    {
        target_edge_idx_ = target_edge_idx;
    }

    uint32_t DirectoryInfo::getDirectoryInfoPayloadSize() const
    {
        // target edge index
        return sizeof(uint32_t);
    }

    uint32_t DirectoryInfo::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        uint32_t bigendian_target_edge_idx = htonl(target_edge_idx_);
        msg_payload.deserialize(size, (const char*)&bigendian_target_edge_idx, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t DirectoryInfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        uint32_t bigendian_target_edge_idx = 0;
        msg_payload.serialize(size, (char *)&bigendian_target_edge_idx, sizeof(uint32_t));
        target_edge_idx_ = ntohl(bigendian_target_edge_idx);
        size += sizeof(uint32_t);
        return size - position;
    }

    uint64_t DirectoryInfo::getSizeForCapacity() const
    {
        return sizeof(uint32_t);
    }

    const DirectoryInfo& DirectoryInfo::operator=(const DirectoryInfo& other)
    {
        target_edge_idx_ = other.target_edge_idx_;
        return *this;
    }

    bool DirectoryInfo::operator==(const DirectoryInfo& other) const
    {
        return target_edge_idx_ == other.target_edge_idx_;
    }
}
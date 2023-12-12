/*
 * DirectoryInfo: directory information for DHT-based content discovery (building blocks for DirectoryTable).
 * 
 * By Siyuan Sheng (2023.06.08).
 */

#ifndef DIRECTORY_INFO_H
#define DIRECTORY_INFO_H

#include <list>
#include <string>
#include <unordered_set>

#include "common/dynamic_array.h"

namespace covered
{
    class DirectoryInfo
    {
    public:
        static std::list<DirectoryInfo>::iterator findDirinfoFromList(const DirectoryInfo& dirinfo, std::list<DirectoryInfo>& dirinfo_list);
        static std::list<DirectoryInfo>::const_iterator findDirinfoFromList(const DirectoryInfo& dirinfo, const std::list<DirectoryInfo>& dirinfo_list);

        DirectoryInfo();
        DirectoryInfo(const uint32_t& target_edge_idx);
        ~DirectoryInfo();

        uint32_t getTargetEdgeIdx() const;
        void setTargetEdgeIdx(const uint32_t& target_edge_idx);

        uint32_t getDirectoryInfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const DirectoryInfo& operator=(const DirectoryInfo& other);
        bool operator==(const DirectoryInfo& other) const; // To be used by DirectoryInfo in std::unordered_map
    private:
        static const std::string kClassName;

        uint32_t target_edge_idx_; // Metadata for cooperation
    };

    // To be used by DirectoryInfo in std::unordered_map
    class DirectoryInfoHasher
    {
    public:
        size_t operator()(const DirectoryInfo& directory_info) const
        {
            return std::hash<uint32_t>{}(directory_info.getTargetEdgeIdx());
        }
    };
}

#endif
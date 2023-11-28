/*
 * MetadataUpdateRequirement: whether we need to trigger beacon-based metadata update after directory admission/eviction.
 *
 * NOTE: if the object is from the single to multiple cache copies, we need to enable is_neighbor_cached in the first edge node; if the object is from multiple to the single cache copy, we need to disable is_neighbor_cached in the last edge node.
 *
 * By Siyuan Sheng (2023.11.28).
 */

#ifndef METADATA_UPDATE_REQUIREMENT_H
#define METADATA_UPDATE_REQUIREMENT_H

#include <string>

namespace covered
{
    class MetadataUpdateRequirement
    {
    public:
        MetadataUpdateRequirement();
        MetadataUpdateRequirement(const bool& is_from_single_to_multiple, const bool& is_from_multiple_to_single, const uint32_t& notify_edge_idx);
        ~MetadataUpdateRequirement();

        bool isFromSingleToMultiple() const;
        bool isFromMultipleToSingle() const;
        uint32_t getNotifyEdgeIdx() const;

        const MetadataUpdateRequirement& operator=(const MetadataUpdateRequirement& other);
    private:
        static const std::string kClassName;

        bool is_from_single_to_multiple_;
        bool is_from_multiple_to_single_;
        uint32_t notify_edge_idx_; // The first edge node if from single to multiple, or the last edge node if from multiple to single
    };
}

#endif
#include "cooperation/directory/metadata_update_requirement.h"

#include <assert.h>

namespace covered
{
    const std::string MetadataUpdateRequirement::kClassName("MetadataUpdateRequirement");

    MetadataUpdateRequirement::MetadataUpdateRequirement()
    {
        is_from_single_to_multiple_ = false;
        is_from_multiple_to_single_ = false;
        notify_edge_idx_ = 0;
    }

    MetadataUpdateRequirement::MetadataUpdateRequirement(const bool& is_from_single_to_multiple, const bool& is_from_multiple_to_single, const uint32_t& notify_edge_idx)
    {
        assert(!(is_from_single_to_multiple && is_from_multiple_to_single));

        is_from_single_to_multiple_ = is_from_single_to_multiple;
        is_from_multiple_to_single_ = is_from_multiple_to_single;
        notify_edge_idx_ = notify_edge_idx;
    }

    MetadataUpdateRequirement::~MetadataUpdateRequirement() {}

    bool MetadataUpdateRequirement::isFromSingleToMultiple() const
    {
        return is_from_single_to_multiple_;
    }

    bool MetadataUpdateRequirement::isFromMultipleToSingle() const
    {
        return is_from_multiple_to_single_;
    }

    uint32_t MetadataUpdateRequirement::getNotifyEdgeIdx() const
    {
        return notify_edge_idx_;
    }

    const MetadataUpdateRequirement& MetadataUpdateRequirement::operator=(const MetadataUpdateRequirement& other)
    {
        is_from_single_to_multiple_ = other.is_from_single_to_multiple_;
        is_from_multiple_to_single_ = other.is_from_multiple_to_single_;
        notify_edge_idx_ = other.notify_edge_idx_;
        return *this;
    }
}
#include "cache/covered/hetero_key_level_metadata.h"

#include <assert.h>

namespace covered
{
    const std::string HeteroKeyLevelMetadata::kClassName("HeteroKeyLevelMetadata");

    HeteroKeyLevelMetadata::HeteroKeyLevelMetadata(const GroupId& group_id, const bool& is_neighbor_cached) : KeyLevelMetadataBase(group_id, is_neighbor_cached)
    {
        redirected_frequency_ = 0;
        redirected_popularity_ = 0.0;

        #ifdef ENABLE_BEACON_BASED_CACHED_METADATA_UPDATE
        is_neighbor_cached_ = is_neighbor_cached;
        #endif
    }

    HeteroKeyLevelMetadata::HeteroKeyLevelMetadata(const HeteroKeyLevelMetadata& other) : KeyLevelMetadataBase(other)
    {
        redirected_frequency_ = other.redirected_frequency_;
        redirected_popularity_ = other.redirected_popularity_;
    }

    HeteroKeyLevelMetadata::~HeteroKeyLevelMetadata() {}

    void HeteroKeyLevelMetadata::updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached)
    {
        assert(is_global_cached == true); // Local cached objects MUST be global cached
        
        if (is_redirected)
        {
            redirected_frequency_++;
        }
        else
        {
            KeyLevelMetadataBase::updateNoValueDynamicMetadata_();
        }

        return;
    }

    void HeteroKeyLevelMetadata::updateRedirectedPopularity(const Popularity& redirected_popularity)
    {
        redirected_popularity_ = redirected_popularity;

        return;
    }

    Frequency HeteroKeyLevelMetadata::getRedirectedFrequency() const
    {
        return redirected_frequency_;
    }

    Popularity HeteroKeyLevelMetadata::getRedirectedPopularity() const
    {
        return redirected_popularity_;
    }

    #ifdef ENABLE_BEACON_BASED_CACHED_METADATA_UPDATE
    void HeteroKeyLevelMetadata::enableIsNeighborCached()
    {
        is_neighbor_cached_ = true;
        return;
    }

    void HeteroKeyLevelMetadata::disableIsNeighborCached()
    {
        is_neighbor_cached_ = false;
        return;
    }
    #endif

    uint64_t HeteroKeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = KeyLevelMetadataBase::getSizeForCapacity();
        total_size += sizeof(Frequency) + sizeof(Popularity);
        return total_size;
    }
}
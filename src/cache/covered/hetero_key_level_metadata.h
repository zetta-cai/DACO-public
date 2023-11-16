/*
 * HeteroKeyLevelMetadata: heterogeneous key-level metadata for each local cached object in local edge cache.
 * 
 * By Siyuan Sheng (2023.11.16).
 */

#ifndef HETERO_KEY_LEVEL_METADATA_H
#define HETERO_KEY_LEVEL_METADATA_H

#include <string>

#include "cache/covered/homo_key_level_metadata.h"
#include "common/covered_common_header.h"

namespace covered
{
    class HeteroKeyLevelMetadata : public HomoKeyLevelMetadata
    {
    public:
        HeteroKeyLevelMetadata(const GroupId& group_id);
        HeteroKeyLevelMetadata(const HeteroKeyLevelMetadata& other);
        ~HeteroKeyLevelMetadata();

        void updateNoValueDynamicMetadata(const bool& is_redirected); // For getreq with redirected hits, update object-level value-unrelated metadata
        void updateRedirectedPopularity(const Popularity& redirected_popularity);

        Frequency getRedirectedFrequency() const;
        Popularity getRedirectedPopularity() const; // Get redirected popularity for redirected hits of local cached objectds

        static uint64_t getSizeForCapacity();
    private:
        static const std::string kClassName;

        // For redirected hits
        Frequency redirected_frequency_;
        Popularity redirected_popularity_;
    };
}

#endif
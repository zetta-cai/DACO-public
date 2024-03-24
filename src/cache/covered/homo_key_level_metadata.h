/*
 * HomoKeyLevelMetadata: homogeneous key-level metadata for each local uncached object in local edge cache.
 * 
 * By Siyuan Sheng (2023.11.18).
 */

#ifndef HOMO_KEY_LEVEL_METADATA_H
#define HOMO_KEY_LEVEL_METADATA_H

#include <string>

#include "cache/covered/key_level_metadata_base.h"
#include "common/covered_common_header.h"

namespace covered
{
    class HomoKeyLevelMetadata : public KeyLevelMetadataBase
    {
    public:
        HomoKeyLevelMetadata();
        HomoKeyLevelMetadata(const GroupId& group_id, const bool& is_neighbor_cached); // NOTE: is_neighbor_cached ONLY used by HeteroKeyLevelMetadata for local cached metadata
        HomoKeyLevelMetadata(const HomoKeyLevelMetadata& other);
        ~HomoKeyLevelMetadata();

        virtual void updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached) override; // For get/put/delreq w/ hit/miss, update object-level value-unrelated metadata (is_redirected MUST be false and will NOT be used)

        bool isGlobalCached() const;
        void updateIsGlobalCached(const bool& is_global_cached);

        static uint64_t getSizeForCapacity();

        // Dump/load key-level metadata for local cached/uncached metadata of cache metadata in cache snapshot
        virtual void dumpKeyLevelMetadata(std::fstream* fs_ptr) const override;
        virtual void loadKeyLevelMetadata(std::fstream* fs_ptr) override;
    private:
        static const std::string kClassName;

        // For heterogeneous local uncached objects
        bool is_global_cached_;
    };
}

#endif
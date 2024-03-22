#include "cache/covered/homo_key_level_metadata.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string HomoKeyLevelMetadata::kClassName("HomoKeyLevelMetadata");

    HomoKeyLevelMetadata::HomoKeyLevelMetadata(const GroupId& group_id, const bool& is_neighbor_cached) : KeyLevelMetadataBase(group_id, is_neighbor_cached)
    {
        is_global_cached_ = true; // Conservatively treat the object is already global cached
    }

    HomoKeyLevelMetadata::HomoKeyLevelMetadata(const HomoKeyLevelMetadata& other) : KeyLevelMetadataBase(other)
    {
        is_global_cached_ = other.is_global_cached_;
    }

    HomoKeyLevelMetadata::~HomoKeyLevelMetadata() {}

    void HomoKeyLevelMetadata::updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached)
    {
        assert(!is_redirected); // MUST be local request for local uncached objects

        KeyLevelMetadataBase::updateNoValueDynamicMetadata_();
        is_global_cached_ = is_global_cached;

        return;
    }

    bool HomoKeyLevelMetadata::isGlobalCached() const
    {
        return is_global_cached_;
    }

    void HomoKeyLevelMetadata::updateIsGlobalCached(const bool& is_global_cached)
    {
        is_global_cached_ = is_global_cached;
        return;
    }

    uint64_t HomoKeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = KeyLevelMetadataBase::getSizeForCapacity();
        total_size += sizeof(bool);
        return total_size;
    }

    // Dump/load key-level metadata for local cached/uncached metadata of cache metadata in cache snapshot
    
    void HomoKeyLevelMetadata::dumpKeyLevelMetadata(std::fstream* fs_ptr) const
    {
        assert(fs_ptr != NULL);

        // Dump key-level metadata in base class
        KeyLevelMetadataBase::dumpKeyLevelMetadata(fs_ptr);

        // Dump is global cached flag
        fs_ptr->write((const char*)&is_global_cached_, sizeof(bool));

        return;
    }

    void HomoKeyLevelMetadata::loadKeyLevelMetadata(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load key-level metadata in base class
        KeyLevelMetadataBase::loadKeyLevelMetadata(fs_ptr);

        // Load is global cached flag
        fs_ptr->read((char*)&is_global_cached_, sizeof(bool));

        return;
    }
}
#include "cache/greedydual/gdsize_cache.h"

namespace covered
{
    const std::string GDSizeCache::kClassName("GDSizeCache");

    GDSizeCache::GDSizeCache(const uint64_t& capacity_bytes) : GreedyDualBase(capacity_bytes)
    {
    }

    GDSizeCache::~GDSizeCache() {}

    long double GDSizeCache::ageValue_(const Key& key, const Value& value)
    {
        const uint64_t object_size = key.getKeyLength() + value.getValuesize();
        return _currentL + 1.0 / static_cast<double>(object_size);
    }
}
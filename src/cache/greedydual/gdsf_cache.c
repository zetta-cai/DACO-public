#include "cache/greedydual/gdsf_cache.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string GDSFCache::kClassName("GDSFCache");

    GDSFCache::GDSFCache(const uint64_t& capacity_bytes) : GreedyDualBase(capacity_bytes)
    {
        _refsMap.clear();
    }

    GDSFCache::~GDSFCache() {}

    bool GDSFCache::lookup(const Key& key, Value& value)
    {
        // NOTE: update frequency before GreedyDualBase::lookup(), which will use frequency to calculate hval
        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(key);
        if (cache_stats_map_iter != _refsMap.end()) // Key is cached
        {
            // Update frequency
            cache_stats_map_iter->second++;
        }

        bool is_hit = GreedyDualBase::lookup(key, value); // Update hval and get value
        if (cache_stats_map_iter != _refsMap.end()) // Key is cached
        {
            assert(is_hit == true); // Should be cache hit for cached key
        }

        return is_hit;
    }

    bool GDSFCache::update(const Key& key, const Value& value)
    {
        // NOTE: update frequency before GreedyDualBase::update(), which will use frequency to calculate hval
        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(key);
        if (cache_stats_map_iter != _refsMap.end()) // Key is cached
        {
            // Update frequency
            cache_stats_map_iter->second++;
        }

        bool is_hit = GreedyDualBase::update(key, value); // Update hval and value
        if (cache_stats_map_iter != _refsMap.end()) // Key is cached
        {
            assert(is_hit == true); // Should be cache hit for cached key
        }

        return is_hit;
    }

    void GDSFCache::admit(const Key& key, const Value& value)
    {
        // NOTE: initialize frequency before GreedyDualBase::admit(), which will use frequency to calculate hval
        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(key);
        assert(cache_stats_map_iter == _refsMap.end());

        // Add key and frequency into statsmap
        cache_stats_map_iter = _refsMap.insert(std::pair(key, 1)).first;
        assert(cache_stats_map_iter != _refsMap.end());
        _currentSize = Util::uint64Add(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(uint64_t)));

        GreedyDualBase::admit(key, value); // Admit hval-kvpair into valuemap; admit key-iterator into lookup table

        return;
    }

    bool GDSFCache::evict(const Key& key, Value& value)
    {
        bool is_evict = GreedyDualBase::evict(key, value); // Evict the given key if any; remove hval-kvpair from valuemap; remove key-iterator from lookup table

        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(key);
        if (is_evict)
        {
            assert(cache_stats_map_iter != _refsMap.end());

            // Remove key and frequency from statsmap
            _refsMap.erase(cache_stats_map_iter);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(uint64_t)));
        }

        return is_evict;
    }

    void GDSFCache::evict(Key& key, Value& value)
    {
        Key tmp_key;
        Value tmp_value;
        GreedyDualBase::evict(tmp_key, tmp_value); // Evict first list element (smallest hval); remove hval-kvpair from valuemap; remove key-iterator from lookup table
        UNUSED(tmp_value);

        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(tmp_key);
        assert(cache_stats_map_iter != _refsMap.end());

        // Remove key and frequency from statsmap
        _refsMap.erase(cache_stats_map_iter);
        _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(tmp_key.getKeyLength(), sizeof(uint64_t)));

        return;
    }

    long double GDSFCache::ageValue_(const Key& key, const Value& value)
    {
        const uint64_t object_size = key.getKeyLength() + value.getValuesize();

        CacheStatsMapType::iterator cache_stats_map_iter = _refsMap.find(key);
        assert(cache_stats_map_iter != _refsMap.end()); // NOTE: even if key is being admited, GDSF will initialize frequency first before calling ageValue_() in GreedyDualBase::admit()
        const uint64_t frequency = cache_stats_map_iter->second;

        return _currentL + static_cast<double>(frequency) / static_cast<double>(object_size);
    }
}
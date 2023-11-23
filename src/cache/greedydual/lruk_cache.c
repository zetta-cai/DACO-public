#include "cache/greedydual/lruk_cache.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string LRUKCache::kClassName("LRUKCache");

    LRUKCache::LRUKCache(const uint64_t& capacity_bytes) : GreedyDualBase(capacity_bytes)
    {
        _reqsMap.clear();
    }

    LRUKCache::~LRUKCache() {}

    bool LRUKCache::lookup(const Key& key, Value& value)
    {
        // Update current timepoint
        _curTime++;

        // NOTE: add new cache access history before GreedyDualBase::lookup(), which will use cache access history to calculate hval
        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        if (reqs_map_iter != _reqsMap.end()) // Key is cached
        {
            reqs_map_iter->second.push(_curTime); // Add new cache access history
            _currentSize = Util::uint64Add(_currentSize, sizeof(uint64_t));
        }

        bool is_hit = GreedyDualBase::lookup(key, value); // Update hval and get value
        if (reqs_map_iter != _reqsMap.end()) // Key is cached
        {
            assert(is_hit == true); // Should be cache hit for cached key
        }

        return is_hit;
    }

    bool LRUKCache::update(const Key& key, const Value& value)
    {
        // Update current timepoint
        _curTime++;

        // NOTE: add new cache access history before GreedyDualBase::update(), which will use cache access history to calculate hval
        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        if (reqs_map_iter != _reqsMap.end()) // Key is cached
        {
            reqs_map_iter->second.push(_curTime); // Add new cache access history
            _currentSize = Util::uint64Add(_currentSize, sizeof(uint64_t));
        }

        bool is_hit = GreedyDualBase::update(key, value); // Update hval and value
        if (reqs_map_iter != _reqsMap.end()) // Key is cached
        {
            assert(is_hit == true); // Should be cache hit for cached key
        }

        return is_hit;
    }

    void LRUKCache::admit(const Key& key, const Value& value)
    {
        // NOTE: initialize cache access history before GreedyDualBase::admit(), which will use cache access history to calculate hval
        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        assert(reqs_map_iter == _reqsMap.end());

        // Add key and the first cache access history of being-admited object into reqsmap
        reqs_map_iter = _reqsMap.insert(std::pair(key, std::queue<uint64_t>())).first;
        assert(reqs_map_iter != _reqsMap.end());
        reqs_map_iter->second.push(_curTime);
        _currentSize = Util::uint64Add(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(uint64_t)));

        GreedyDualBase::admit(key, value); // Admit hval-kvpair into valuemap; admit key-iterator into lookup table

        return;
    }

    bool LRUKCache::evict(const Key& key, Value& value)
    {
        bool is_evict = GreedyDualBase::evict(key, value); // Evict the given key if any; remove hval-kvpair from valuemap; remove key-iterator from lookup table

        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        if (is_evict)
        {
            assert(reqs_map_iter != _reqsMap.end());
            
            // Remove key and cache access history from statsmap
            const uint64_t history_cnt = reqs_map_iter->second.size();
            _reqsMap.erase(reqs_map_iter);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(key.getKeyLength(), history_cnt * sizeof(uint64_t)));
        }

        return is_evict;
    }

    void LRUKCache::evict(Key& key, Value& value)
    {
        Key tmp_key;
        Value tmp_value;
        GreedyDualBase::evict(tmp_key, tmp_value); // Evict first list element (smallest hval); remove hval-kvpair from valuemap; remove key-iterator from lookup table
        UNUSED(tmp_value);

        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        assert(reqs_map_iter != _reqsMap.end());

        // Remove key and cache access history from statsmap
        const uint64_t history_cnt = reqs_map_iter->second.size();
        _reqsMap.erase(reqs_map_iter);
        _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(tmp_key.getKeyLength(), history_cnt * sizeof(uint64_t)));

        return;
    }

    long double LRUKCache::ageValue_(const Key& key, const Value& value)
    {
        // NOTE: LRU-K ONLY uses cache access history instead of _currentL (aging factor) to calculate hval
        long double newVal = 0.0L;
        LrukMapType::iterator reqs_map_iter = _reqsMap.find(key);
        assert(reqs_map_iter != _reqsMap.end()); // NOTE: even if key is being admited, GDSF will initialize frequency first before calling ageValue_() in GreedyDualBase::admit()
        if (reqs_map_iter->second.size() >= _tk)
        {
            newVal = reqs_map_iter->second.front();
            reqs_map_iter->second.pop(); // Pop the oldest cache access history
            _currentSize = Util::uint64Minus(_currentSize, sizeof(uint64_t));
        }
        return newVal;
    }
}
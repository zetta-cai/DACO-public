#include "cache/greedydual/greedy_dual_base.h"

#include <cassert>
#include <sstream>
#include <unordered_map>

#include "common/util.h"

namespace covered
{
    const std::string GreedyDualBase::kClassName("GreedyDualBase");
    
    GreedyDualBase::GreedyDualBase(const uint64_t& capacity_bytes) : _cacheSize(capacity_bytes)
    {
        _currentSize = 0;
        _currentL = 0.0;
        _valueMap.clear();
        _cacheMap.clear();
    }

    GreedyDualBase::~GreedyDualBase() {}

    bool GreedyDualBase::exists(const Key& key)
    {
        bool is_exist = false;

        GdCacheMapType::const_iterator cache_map_const_iter = _cacheMap.find(key);
        if (cache_map_const_iter != _cacheMap.end())
        {
            is_exist = true;
        }

        return is_exist;
    }

    bool GreedyDualBase::lookup(const Key& key, Value& value)
    {
        bool is_hit = false;

        GdCacheMapType::const_iterator cache_map_const_iter = _cacheMap.find(key);
        if (cache_map_const_iter != _cacheMap.end())
        {
            hit_(key); // Update hval

            // Get value
            ValueMapType::const_iterator value_map_const_iter = cache_map_const_iter->second;
            assert(value_map_const_iter != _valueMap.end());
            value = value_map_const_iter->second.second;

            is_hit = true;
        }

        // NOTE: NO need to update _currentSize

        return is_hit;
    }

    bool GreedyDualBase::update(const Key& key, const Value& value)
    {
        bool is_hit = false;

        GdCacheMapType::iterator cache_map_iter = _cacheMap.find(key);
        if (cache_map_iter != _cacheMap.end())
        {
            // NOTE: we update value first before hit_(), which needs to use value to calculate hval
            ValueMapType::iterator value_map_iter = cache_map_iter->second;
            assert(value_map_iter != _valueMap.end());
            const Value original_value = value_map_iter->second.second;
            value_map_iter->second.second = value;

            hit_(key); // Update hval

            // Update _currentSize for value change
            _currentSize = Util::uint64Add(_currentSize, value.getValuesize());
            _currentSize = Util::uint64Minus(_currentSize, original_value.getValuesize());

            // Check eviction needed
            while (_currentSize > _cacheSize)
            {
                // NOTE: NEVER triggered due to over-provisioned capacity bytes for internal cache engine, while we perform cache eviction outside internal cache engine
                assert(false);

                Key tmp_key;
                Value tmp_value;
                evict(tmp_key, tmp_value); // Will update _currentL and reduce _currentSize
                UNUSED(tmp_key);
                UNUSED(tmp_value);
            }

            is_hit = true;
        }

        return is_hit;
    }

    void GreedyDualBase::admit(const Key& key, const Value& value)
    {
        assert(_cacheMap.find(key) == _cacheMap.end());

        const uint64_t object_size = key.getKeyLength() + value.getValuesize();

        // Check if object feasible to store?
        if (object_size >= _cacheSize)
        {
            std::ostringstream oss;
            oss << "Too large object size (" << object_size << " bytes) for key " << key.getKeyLength() << ", which exceeds capacity " << _cacheSize << " bytes!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
            return;
        }

        // Check eviction needed
        while (_currentSize + object_size > _cacheSize)
        {
            // NOTE: NEVER triggered due to over-provisioned capacity bytes for internal cache engine, while we perform cache eviction outside internal cache engine
            std::ostringstream oss;
            oss << "Cache is full for current size usage " << _currentSize << " bytes under capacity " << _cacheSize << " bytes -> cannot admit new object with size " << object_size << " bytes!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);

            // NOTE: disable internal cache eviction
            // Key tmp_key;
            // Value tmp_value;
            // evict(tmp_key, tmp_value); // Will update _currentL and reduce _currentSize
            // UNUSED(tmp_key);
            // UNUSED(tmp_value);
        }

        // Admit new object with new GF value
        long double ageVal = ageValue_(key, value); // Calculate hval based on latest _currentL

        // Admit hval and key-value pair into valuemap
        ValueMapIteratorType value_map_iter = _valueMap.emplace(ageVal, std::pair(key, value));
        assert(value_map_iter != _valueMap.end());
        _currentSize = Util::uint64Add(_currentSize, Util::uint64Add(sizeof(long double), object_size));

        // Admit key and valuemap iter into lookup table
        GdCacheMapType::const_iterator cache_map_const_iter = _cacheMap.insert(std::pair(key, value_map_iter)).first;
        assert(cache_map_const_iter != _cacheMap.end());
        _currentSize = Util::uint64Add(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(ValueMapIteratorType)));

        return;
    }

    bool GreedyDualBase::getVictimKey(Key& key)
    {
        bool has_victim = false;

        // Choose the first list element (smallest hval)
        if (_valueMap.size() > 0)
        {
            ValueMapIteratorType value_map_iter  = _valueMap.begin();
            assert(value_map_iter != _valueMap.end()); // Bug if this happens

            // Just get victim key yet NOT evict the victim
            key = value_map_iter->second.first;

            has_victim = true;
        }

        return has_victim;
    }

    bool GreedyDualBase::evict(const Key& key, Value& value)
    {
        bool is_evict = false;

        // Evict the object with the given key
        GdCacheMapType::const_iterator cache_map_const_iter = _cacheMap.find(key);
        if (cache_map_const_iter != _cacheMap.end())
        {
            ValueMapIteratorType value_map_iter = cache_map_const_iter->second;
            assert(key == value_map_iter->second.first);
            value = value_map_iter->second.second;
            const uint64_t object_size = key.getKeyLength() + value.getValuesize();

            // Remove hval and key-value pair from valuemap
            _valueMap.erase(value_map_iter);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(sizeof(long double), object_size));

            // Remove key and valuemap iter from lookup table
            _cacheMap.erase(cache_map_const_iter);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(ValueMapIteratorType)));

            is_evict = true;
        }

        return is_evict;
    }

    void GreedyDualBase::evict(Key& key, Value& value)
    {
        // Evict first list element (smallest hval)
        if (_valueMap.size() > 0)
        {
            ValueMapIteratorType value_map_iter  = _valueMap.begin();
            if (value_map_iter == _valueMap.end())
            {
                std::ostringstream oss;
                oss << "No victim to evict for current size usage " << _currentSize << " bytes under capacity " << _cacheSize << " bytes!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
                return;
            }
            assert(value_map_iter != _valueMap.end()); // Bug if this happens

            key = value_map_iter->second.first;
            value = value_map_iter->second.second;
            const uint64_t object_size = key.getKeyLength() + value.getValuesize();
            _currentL = value_map_iter->first; // update L

            // Remove hval and key-value pair from valuemap
            _valueMap.erase(value_map_iter);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(sizeof(long double), object_size));

            // Remove key and valuemap iter from lookup table
            _cacheMap.erase(key);
            _currentSize = Util::uint64Minus(_currentSize, Util::uint64Add(key.getKeyLength(), sizeof(ValueMapIteratorType)));
        }

        return;
    }

    uint64_t GreedyDualBase::getSizeForCapacity()
    {
        return _currentSize;
    }

    long double GreedyDualBase::ageValue_(const Key& key, const Value& value)
    {
        return _currentL + 1.0;
    }

    void GreedyDualBase::hit_(const Key& key)
    {
        // Get valuemap iterator for the old position
        GdCacheMapType::iterator cache_map_iter = _cacheMap.find(key);
        assert(cache_map_iter != _cacheMap.end());
        ValueMapIteratorType value_map_iter = cache_map_iter->second;
        assert(value_map_iter != _valueMap.end());

        // Update current req's value to hval:
        Value value = value_map_iter->second.second;
        long double hval = ageValue_(key, value);
        _valueMap.erase(value_map_iter);
        value_map_iter = _valueMap.emplace(hval, std::pair<Key, Value>(key, value));
        assert(value_map_iter != _valueMap.end());

        // Update lookup table
        cache_map_iter->second = value_map_iter;

        // NOTE: NO need to update _currentSize

        return;
    }
}
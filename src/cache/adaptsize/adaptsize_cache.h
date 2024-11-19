/*
 * AdaptSizeCache: refer to lib/webcachesim/caches/adaptsize_const.h and lib/webcachesim/caches/lru_variants.*.
 *
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2024.11.19).
 */

#ifndef ADAPTSIZE_CACHE_H
#define ADAPTSIZE_CACHE_H

#include <list>
#include <random>
#include <unordered_map>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    /*
      AdaptSize: ExpLRU with automatic adaption of the _cParam
    */
    class AdaptSizeCache
    {
    public:
        AdaptSizeCache(const uint64_t& capacity_bytes);
        ~AdaptSizeCache();

        bool exists(const Key& key); // Check if key exists in cache (NOT udpate cache metadata)

        bool lookup(const Key& key, Value& value);
        bool update(const Key& key, const Value& value);
        bool needAdmit(const Key& key, const Value& value);
        void admit(const Key& key, const Value& value);
        bool getVictimKey(Key& key); // Get the victim key yet NOT evict
        bool evict(const Key& key, Value& value); // Evict the given key if any
        void evict(Key& key, Value& value); // Evict the victim and return the victim key

        uint64_t getSizeForCapacity();

    private:
        typedef std::list<std::pair<Key, Value>>::iterator ListIteratorType;
        typedef std::unordered_map<Key, ListIteratorType, KeyHasher> lruCacheMapType;

        struct ObjInfo {
            double requestCount; // requestRate in adaptsize_stub.h
            uint64_t objSize;

            ObjInfo() : requestCount(0.0), objSize(0) { }
        };

        static const std::string kClassName;

        // AdaptSize utility functions
        void metadataUpdate_(const Key& key, const uint64_t object_size);
        void reconfigure();
        double modelHitRate(double c);

        // AdaptSize const parameters (from lib/webcachesim/caches/adaptsize_const.h)
        const double EWMA_DECAY = 0.3;
        const uint64_t RANGE = 1ull << 32; 
        const double gss_r = 0.61803399;
        const double tol = 3.0e-8;

        // Random generator (from lib/webcachesim/random_helper.*)
        const unsigned int SEED = 1534262824; // const seed for repeatable results
        std::mt19937_64 globalGenerator;
        
        // basic cache properties (from lib/webcachesim/cache.h)
        const uint64_t _cacheSize; // size of cache in bytes
        uint64_t _currentSize; // total size of objects in cache in bytes

        // LRU cache metadata (from lib/webcachesim/caches/lru_variants.h)
        // list for recency order
        // std::list is a container, usually, implemented as a doubly-linked list 
        std::list<std::pair<Key, Value>> _cacheList;
        // map to find objects in list
        lruCacheMapType _cacheMap;

        // AdaptSize cache metadata (from lib/webcachesim/caches/lru_variants.h)
        double _cParam; //
        uint64_t statSize;
        uint64_t _maxIterations;
        uint64_t _reconfiguration_interval;
        uint64_t _nextReconfiguration;
        double _gss_v;  // golden section search book parameters
        // for random number generation 
        std::uniform_real_distribution<double> _uniform_real_distribution = 
            std::uniform_real_distribution<double>(0.0, 1.0); 
        std::unordered_map<Key, ObjInfo, KeyHasher> _longTermMetadata;
        std::unordered_map<Key, ObjInfo, KeyHasher> _intervalMetadata;
        // align data for vectorization
        std::vector<double> _alignedReqCount;
        std::vector<double> _alignedObjSize;
        std::vector<double> _alignedAdmProb;
    };
}

#endif
#include "cache/adaptsize/adaptsize_cache.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    // Utility macros and static inline functions (refer to lib/webcachesim/caches/lru_variants.cpp)

    // golden section search helpers
    #define SHFT2(a,b,c) (a)=(b);(b)=(c);
    #define SHFT3(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);

    // math model below can be directly copiedx
    // static inline double oP1(double T, double l, double p) {
    static inline double oP1(double T, double l, double p) {
        return (l * p * T * (840.0 + 60.0 * l * T + 20.0 * l*l * T*T + l*l*l * T*T*T));
    }

    static inline double oP2(double T, double l, double p) {
        return (840.0 + 120.0 * l * (-3.0 + 7.0 * p) * T + 60.0 * l*l * (1.0 + p) * T*T + 4.0 * l*l*l * (-1.0 + 5.0 * p) * T*T*T + l*l*l*l * p * T*T*T*T);
    }

    const std::string AdaptSizeCache::kClassName("AdaptSizeCache");

    AdaptSizeCache::AdaptSizeCache(const uint64_t& capacity_bytes) : _cacheSize(capacity_bytes)
    {
        // Random generator
        globalGenerator.seed(SEED);

        // basic cache properties
        _currentSize = 2 * sizeof(double) + 4 * sizeof(uint64_t); // AdaptSize cache metadata when initialization

        // LRU cache metadata
        _cacheList.clear();
        _cacheMap.clear();

        // AdaptSize cache metadata
        // NOTE: follow AdaptSizeCache::AdaptSizeCache() in lib/webcachesim/caches/lru_variants.cpp to initialize these metadata
        _cParam = 1 << 15;
        statSize = 0;
        _maxIterations = 15;
        _reconfiguration_interval = 500000;
        _nextReconfiguration = _reconfiguration_interval;
        _gss_v = 1.0 - gss_r; // golden section search book parameters
        _longTermMetadata.clear();
        _intervalMetadata.clear();
        _alignedReqCount.clear();
        _alignedObjSize.clear();
        _alignedAdmProb.clear();
    }

    AdaptSizeCache::~AdaptSizeCache() {}

    bool AdaptSizeCache::exists(const Key& key)
    {
        lruCacheMapType::const_iterator cache_map_const_iter = _cacheMap.find(key);
        bool is_exist = (cache_map_const_iter != _cacheMap.end());
        return is_exist;
    }

    bool AdaptSizeCache::lookup(const Key& key, Value& value)
    {
        // LRU's lookup -> get value
        bool is_hit = false;
        lruCacheMapType::const_iterator cache_map_const_iter = _cacheMap.find(key);
        if (cache_map_const_iter != _cacheMap.end()) {
            ListIteratorType list_iter = cache_map_const_iter->second;
            assert(list_iter != _cacheList.end());

			// Key check
			if (!(list_iter->first == key))
			{
				std::ostringstream oss;
				oss << "lookup() for key " << key.getKeyDebugstr() << " failed due to mismatched key of list_iter->first " << list_iter->first.getKeyDebugstr();
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

            // Update LRU list
            _cacheList.splice(_cacheList.begin(), _cacheList, list_iter); // Move the list entry pointed by list_iter to the head of the list (NOT change the memory address of the list entry)

            value = list_iter->second;
            is_hit = true;
        }

        // AdaptSize's metadata update
        if (is_hit) // Cache hit with object size
        {
            const uint64_t object_size = key.getKeyLength() + value.getValuesize();
            metadataUpdate_(key, object_size);
        }
        else // Cache miss without object size now
        {
            // NOTE: we delay the metadata update until receiving the response of the get request with cache miss, which will update AdaptSize's metadata by AdaptSize::update()
            // (1) If key is locally cached yet invalid, cache wrapper will invoke updateLocalCache(is_getrsp=true) to update both value and metadata
            // (2) Else if key is locally uncached, cache wrapper will invoke updateLocalCache(is_getrsp=true) to update metadata only
        }
        
        return is_hit;
    }

    bool AdaptSizeCache::update(const Key& key, const Value& value)
    {
        // LRU's update -> update value
        bool is_hit = false;
        lruCacheMapType::iterator cache_map_iter = _cacheMap.find(key);
        if (cache_map_iter != _cacheMap.end()) {
            is_hit = true;

			// Update the object with new value
			// Push key-value pair into the head of _cacheList, i.e., the key is the most recently used
			_cacheList.push_front(std::pair<Key, Value>(key, value));
			_currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

			// Remove previous list entry from cache_items_list_
			ListIteratorType prev_list_iter = cache_map_iter->second;
			assert(prev_list_iter != _cacheList.end());
			assert(prev_list_iter->first == key);
			uint32_t prev_keysize = prev_list_iter->first.getKeyLength();
			uint32_t prev_valuesize = prev_list_iter->second.getValuesize();
			_cacheList.erase(prev_list_iter);
			_currentSize = Util::uint64Minus(_currentSize, static_cast<uint64_t>(prev_keysize + prev_valuesize));

			// Update map entry with the latest list iterator
			cache_map_iter->second = _cacheList.begin();
        }

        // AdaptSize's metadata update
        // NOTE: always with object size no matther hit or miss -> NO need to delay
        const uint64_t object_size = key.getKeyLength() + value.getValuesize();
        metadataUpdate_(key, object_size);

        return is_hit;
    }

    bool AdaptSizeCache::needAdmit(const Key& key, const Value& value)
    {
        bool need_admit = false;
        if (_cacheMap.find(key) == _cacheMap.end()) // Key is not cached now
        {
            const uint64_t object_size = key.getKeyLength() + value.getValuesize();

            // AdaptSize's admission control
            double roll = _uniform_real_distribution(globalGenerator);
            double admitProb = std::exp(-1.0 * double(object_size)/_cParam); 
            
            if (roll < admitProb)
            {
                need_admit = true;
            }
        }

        return need_admit;
    }

    void AdaptSizeCache::admit(const Key& key, const Value& value)
    {
        assert(_cacheMap.find(key) == _cacheMap.end()); // Must NOT exist in cache now

        const uint64_t object_size = key.getKeyLength() + value.getValuesize();

        // Perform LRU's admit

        // Check if object feasible to store?
        if (object_size >= _cacheSize)
        {
            std::ostringstream oss;
            oss << "Too large object size (" << object_size << " bytes) for key " << key.getKeyDebugstr() << ", which exceeds capacity " << _cacheSize << " bytes!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
            return;
        }

        // check eviction needed
        while (_currentSize + object_size > _cacheSize) {
            // NOTE: NEVER triggered due to over-provisioned capacity bytes for internal cache engine, while we perform cache eviction outside internal cache engine
            std::ostringstream oss;
            oss << "Cache is full for current size usage " << _currentSize << " bytes under capacity " << _cacheSize << " bytes -> cannot admit new object with size " << object_size << " bytes!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);

            // NOTE: disable internal cache eviction
            // Key tmp_key;
            // Value tmp_value;
            // evict(tmp_key, tmp_value);
            // UNUSED(tmp_key);
            // UNUSED(tmp_value);
        }

        // admit new object
        // Insert the object with new value
        // Push key-value pair into the head of cache_items_list_, i.e., the key is the most recently used
        _cacheList.push_front(std::pair<Key, Value>(key, value));
        _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(key.getKeyLength() + value.getValuesize()));

        _cacheMap.insert(std::pair<Key, ListIteratorType>(key, _cacheList.begin()));
        _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(key.getKeyLength() + sizeof(ListIteratorType)));

        return;
    }

    bool AdaptSizeCache::getVictimKey(Key& key)
    {
        // The same as LRU's getVictimKey
        bool has_victim_key = false;
		if (_cacheList.size() > 0)
		{
			// Select victim by LRU
			ListIteratorType last_list_iter = _cacheList.end();
			last_list_iter--;
			key = last_list_iter->first;

			has_victim_key = true;
		}
		return has_victim_key;
    }

    bool AdaptSizeCache::evict(const Key& key, Value& value)
    {
        // The same as LRU's evict

        bool is_evict = false;

        lruCacheMapType::iterator cache_map_iter = _cacheMap.find(key);
		if (cache_map_iter != _cacheMap.end()) // Key exists
		{
			ListIteratorType victim_list_iter = cache_map_iter->second;
			assert(victim_list_iter != _cacheList.end());
			value = victim_list_iter->second;
			uint32_t victim_valuesize = value.getValuesize();

			// Remove the corresponding map entry
			_cacheMap.erase(key);
			_currentSize = Util::uint64Minus(_currentSize, static_cast<uint64_t>(key.getKeyLength() + sizeof(ListIteratorType)));

			// Remove the corresponding list entry
			// NOTE: we cannot simply pop the least recent object from LRU list, which may NOT be the same as the given victim due to multi-threading access
			//_cacheList.pop_back();
			_cacheList.erase(victim_list_iter);
			_currentSize = Util::uint64Minus(_currentSize, static_cast<uint64_t>(key.getKeyLength() + victim_valuesize));

			is_evict = true;
		}

		return is_evict;
    }

    void AdaptSizeCache::evict(Key& key, Value& value)
    {
        bool has_victim_key = getVictimKey(key);
        assert(has_victim_key);

        bool is_evict = evict((const Key&)key, value);
        assert(is_evict);

        return;
    }

    uint64_t AdaptSizeCache::getSizeForCapacity()
    {
        return _currentSize;
    }

    // AdaptSize utility functions

    void AdaptSizeCache::metadataUpdate_(const Key& key, const uint64_t object_size)
    {
        reconfigure();

        if(_intervalMetadata.count(key)==0 
        && _longTermMetadata.count(key)==0) { 
            // new object 
            statSize += object_size;
        }
        // the else block is not necessary as webcachesim treats an object 
        // with size changed as a new object 
        /** 
        } else {
            // keep track of changing object sizes
            if(_intervalMetadata.count(id)>0 
            && _intervalMetadata[id].size != req.size()) {
            // outdated size info in _intervalMetadata
            statSize -= _intervalMetadata[id].size;
            statSize += req.size();
            }
            if(_longTermMetadata.count(id)>0 && _longTermMetadata[id].size != req.size()) {
            // outdated size info in ewma
            statSize -= _longTermMetadata[id].size;
            statSize += req.size();
            }
        }
        */

        // record stats
        std::unordered_map<Key, ObjInfo, KeyHasher>::iterator interval_metadata_iter = _intervalMetadata.find(key);
        if (interval_metadata_iter != _intervalMetadata.end())
        {
            interval_metadata_iter->second.requestCount += 1.0;
            interval_metadata_iter->second.objSize = object_size;
        }
        else
        {
            ObjInfo info;
            info.requestCount = 1.0;
            info.objSize = object_size;
            _intervalMetadata.insert(std::pair(key, info));

            // Increase size for _intervalMetadata
            _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(key.getKeyLength() + sizeof(struct ObjInfo)));
        }

        return;
    }

    void AdaptSizeCache::reconfigure() {
        --_nextReconfiguration;
        if (_nextReconfiguration > 0) {
            return;
        } else if(statSize <= _cacheSize*3) {
            // not enough data has been gathered
            _nextReconfiguration+=10000;
            return; 
        } else {
            _nextReconfiguration = _reconfiguration_interval;
        }

        // smooth stats for objects 
        for(auto it = _longTermMetadata.begin(); 
            it != _longTermMetadata.end(); 
            it++) {
            it->second.requestCount *= EWMA_DECAY; 
        } 

        // persist intervalinfo in _longTermMetadata 
        for (auto it = _intervalMetadata.begin(); 
            it != _intervalMetadata.end();
            it++) {
            auto ewmaIt = _longTermMetadata.find(it->first); 
            if(ewmaIt != _longTermMetadata.end()) {
                ewmaIt->second.requestCount += (1. - EWMA_DECAY) 
                    * it->second.requestCount;
                ewmaIt->second.objSize = it->second.objSize; 
            } else {
                _longTermMetadata.insert(*it);

                // Increase size for _longTermMetadata
                _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(it->first.getKeyLength() + sizeof(struct ObjInfo)));
            }

            // Decrease size for _intervalMetadata, which will be cleared soon
            _currentSize = Util::uint64Minus(_currentSize, static_cast<uint64_t>(it->first.getKeyLength() + sizeof(struct ObjInfo)));
        }
        _intervalMetadata.clear(); 

        // copy stats into vector for better alignment 
        // and delete small values 
        _currentSize = Util::uint64Minus(_currentSize, _alignedReqCount.size() * sizeof(double)); // Decrease size for _alignedReqCount
        _alignedReqCount.clear(); 
        _currentSize = Util::uint64Minus(_currentSize, _alignedObjSize.size() * sizeof(double)); // Decrease size for _alignedObjSize
        _alignedObjSize.clear();
        double totalReqCount = 0.0; 
        uint64_t totalObjSize = 0.0; 
        for(auto it = _longTermMetadata.begin(); 
            it != _longTermMetadata.end(); 
            /*none*/) {
            if(it->second.requestCount < 0.1) {
                // delete from stats 
                statSize -= it->second.objSize; 
                it = _longTermMetadata.erase(it); 

                _currentSize = Util::uint64Minus(_currentSize, static_cast<uint64_t>(it->first.getKeyLength() + sizeof(struct ObjInfo))); // Decrease size for _longTermMetadata
            } else {
                _alignedReqCount.push_back(it->second.requestCount); 
                totalReqCount += it->second.requestCount; 
                _alignedObjSize.push_back(it->second.objSize); 
                totalObjSize += it->second.objSize; 
                ++it;

                _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(2 * sizeof(double))); // Increase size for _alignedReqCount and _alignedObjSize
            }
        }

        std::cerr << "Reconfiguring over " << _longTermMetadata.size() 
                << " objects - log2 total size " << std::log2(totalObjSize) 
                << " log2 statsize " << std::log2(statSize) << std::endl; 

        // assert(totalObjSize==statSize); 
        //
        // if(totalObjSize > cacheSize*2) {
        //
        // model hit rate and choose best admission parameter, c
        // search for best parameter on log2 scale of c, between min=x0 and max=x3
        // x1 and x2 bracket our current estimate of the optimal parameter range
        // |x0 -- x1 -- x2 -- x3|
        double x0 = 0; 
        double x1 = std::log2(_cacheSize);
        double x2 = x1;
        double x3 = x1; 

        double bestHitRate = 0.0; 
        // course_granular grid search 
        for(int i=2; i<x3; i+=4) {
            const double next_log2c = i; // 1.0 * (i+1) / NUM_PARAMETER_POINTS;
            const double hitRate = modelHitRate(next_log2c); 
            // printf("Model param (%f) : ohr (%f)\n",
            // 	next_log2c,hitRate/totalReqRate);

            if(hitRate > bestHitRate) {
                bestHitRate = hitRate;
                x1 = next_log2c;
            }
        }

        double h1 = bestHitRate; 
        double h2;
        //prepare golden section search into larger segment 
        if(x3-x1 > x1-x0) {
            // above x1 is larger segment 
            x2 = x1+_gss_v*(x3-x1); 
            h2 = modelHitRate(x2);
        } else {
            // below x1 is larger segment 
            x2 = x1; 
            h2 = h1; 
            x1 = x0+_gss_v*(x1-x0); 
            h1 = modelHitRate(x1); 
        }
        assert(x1<x2); 

        uint64_t curIterations=0; 
        // use termination condition from [Numerical recipes in C]
        while(curIterations++<_maxIterations 
            && fabs(x3-x0)>tol*(fabs(x1)+fabs(x2))) {
            //NAN check 
            if((h1!=h1) || (h2!=h2)) 
                break; 
            // printf("Model param low (%f) : ohr low (%f) | param high (%f) 
            // 	: ohr high (    %f)\n",x1,h1/totalReqRate,x2,
            // 	h2/totalReqRate);

            if(h2>h1) {
                SHFT3(x0,x1,x2,gss_r*x1+_gss_v*x3); 
                SHFT2(h1,h2,modelHitRate(x2));
            } else {
                SHFT3(x3,x2,x1,gss_r*x2+_gss_v*x0);
                SHFT2(h2,h1,modelHitRate(x1));
            }
        }

        // check result
        if( (h1!=h1) || (h2!=h2) ) {
            // numerical failure
            std::cerr << "ERROR: numerical bug " << h1 << " " << h2 
                    << std::endl;
            // nop
        } else if (h1 > h2) {
            // x1 should is final parameter
            _cParam = pow(2, x1);
            std::cerr << "Choosing c of " << _cParam << " (log2: " << x1 << ")" 
                    << std::endl;
        } else {
            _cParam = pow(2, x2);
            std::cerr << "Choosing c of " << _cParam << " (log2: " << x2 << ")" 
                    << std::endl;
        }
    }

    double AdaptSizeCache::modelHitRate(double log2c) {
        // this code is adapted from the AdaptSize git repo
        // github.com/dasebe/AdaptSize
        double old_T, the_T, the_C;
        double sum_val = 0.;
        double thparam = log2c;

        for(size_t i=0; i<_alignedReqCount.size(); i++) {
            sum_val += _alignedReqCount[i] * (exp(-_alignedObjSize[i]/ pow(2,thparam))) * _alignedObjSize[i];
        }
        if(sum_val <= 0) {
            return(0);
        }
        the_T = _cacheSize / sum_val;
        // prepare admission probabilities
        _currentSize = Util::uint64Minus(_currentSize, _alignedAdmProb.size() * sizeof(double)); // Decrease size for _alignedAdmProb
        _alignedAdmProb.clear();
        for(size_t i=0; i<_alignedReqCount.size(); i++) {
            _alignedAdmProb.push_back(exp(-_alignedObjSize[i]/ pow(2.0,thparam)));

            _currentSize = Util::uint64Add(_currentSize, static_cast<uint64_t>(sizeof(double))); // Increase size for _alignedAdmProb
        }
        // 20 iterations to calculate TTL
    
        for(int j = 0; j<10; j++) {
            the_C = 0;
            if(the_T > 1e70) {
                break;
            }
            for(size_t i=0; i<_alignedReqCount.size(); i++) {
                const double reqTProd = _alignedReqCount[i]*the_T;
                if(reqTProd>150) {
                    // cache hit probability = 1, but numerically inaccurate to calculate
                    the_C += _alignedObjSize[i];
                } else {
                    const double expTerm = exp(reqTProd) - 1;
                    const double expAdmProd = _alignedAdmProb[i] * expTerm;
                    const double tmp = expAdmProd / (1 + expAdmProd);
                    the_C += _alignedObjSize[i] * tmp;
                }
            }
            old_T = the_T;
            the_T = _cacheSize * old_T/the_C;
        }

        // calculate object hit ratio
        double weighted_hitratio_sum = 0;
        for(size_t i=0; i<_alignedReqCount.size(); i++) {
            const double tmp01= oP1(the_T,_alignedReqCount[i],_alignedAdmProb[i]);
            const double tmp02= oP2(the_T,_alignedReqCount[i],_alignedAdmProb[i]);
            double tmp;
            if(tmp01!=0 && tmp02==0)
                tmp = 0.0;
            else tmp= tmp01/tmp02;
            if(tmp<0.0)
                tmp = 0.0;
            else if (tmp>1.0)
                tmp = 1.0;
            weighted_hitratio_sum += _alignedReqCount[i] * tmp;
        }
        return (weighted_hitratio_sum);
    }
}
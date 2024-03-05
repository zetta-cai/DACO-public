#include "cache/glcache_local_cache.h"

#include <assert.h>
#include <sstream>

#include <libCacheSim/evictionAlgo/GLCache.h> // src/cache/glcache/micro-implementation/build/include/libCacheSim/evictionAlgo/GLCache.h
#include <libCacheSim/cacheObj.h> // Siyuan: src/cache/glcache/micro-implementation/libCacheSim/include/libCacheSim/cacheObj.h
#include <libCacheSim/enum.h> // src/cache/glcache/micro-implementation/libCacheSim/include/libCacheSim/enum.h

#include "common/util.h"

namespace covered
{
    const std::string GLCacheLocalCache::kClassName("GLCacheLocalCache");

    GLCacheLocalCache::GLCacheLocalCache(const EdgeWrapperBase* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Refer to the same settings of cc_params in lib/glcache/micro-implementation/libCacheSim/bin/cachesim/cli.c and lib/glcache/micro-implementation/test/test_glcache.c
        common_cache_params_t cc_params = {
            .cache_size = capacity_bytes,
            .default_ttl = 86400 * 300,
            .hashpower = 24,
            .consider_obj_metadata = true,
        };

        // Allocate and initialized glcache data and metadata
        glcache_ptr_ = GLCache_init(cc_params, NULL);
        assert(glcache_ptr_ != NULL);
    }
    
    GLCacheLocalCache::~GLCacheLocalCache()
    {
        // Release glcache data and metadata
        GLCache_free(glcache_ptr_);
        glcache_ptr_ = NULL;
    }

    const bool GLCacheLocalCache::hasFineGrainedManagement() const
    {
        return false; // CANNOT find and evict a given victim due to segment-level merge-based eviction
    }

    // (1) Check is cached and access validity

    bool GLCacheLocalCache::isLocalCachedInternal_(const Key& key) const
    {
        request_t req = buildRequest_(key);
        cache_ck_res_e result = glcache_ptr_->exists(glcache_ptr_, &req);
        bool is_cached = (result == cache_ck_hit);

        return is_cached;
    }

    // (2) Access local edge cache (KV data and local metadata)

    bool GLCacheLocalCache::getLocalCacheInternal_(const Key& key, const bool& is_redirected, Value& value, bool& affect_victim_tracker) const
    {
        UNUSED(is_redirected); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        request_t req = buildRequest_(key);
        cache_ck_res_e result = glcache_ptr_->get(glcache_ptr_, &req);
        value = Value(req.value.length());
        bool is_local_cached = (result == cache_ck_hit);

        return is_local_cached;
    }

    bool GLCacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY used by COVERED
        UNUSED(is_global_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED
        is_successful = false;

        // Check is local cached
        request_t req = buildRequest_(key);
        cache_ck_res_e result = glcache_ptr_->exists(glcache_ptr_, &req);
        bool is_local_cached = (result == cache_ck_hit);
        
        if (is_local_cached) // Key already exists
        {
            if (!is_valid_objsize)
            {
                is_successful = false; // NOT cache too large object size -> equivalent to NOT caching the latest value (will be invalidated by CacheWrapper later if key is local cached)
                return is_local_cached;
            }

            // Update with the latest value
            request_t tmp_req = buildRequest_(key, value);
            cache_ck_res_e tmp_result = glcache_ptr_->update(glcache_ptr_, &tmp_req);
            bool tmp_is_local_cached = (tmp_result == cache_ck_hit);
            assert(tmp_is_local_cached);
            is_successful = true;
        }

        return is_local_cached;
    }

    // (3) Local edge cache management

    bool GLCacheLocalCache::needIndependentAdmitInternal_(const Key& key, const Value& value) const
    {
        UNUSED(value);
        
        // GL-Cache cache uses default admission policy (i.e., always admit) (i.e., always admit), which always returns true as long as key is not cached
        bool is_local_cached = isLocalCachedInternal_(key);
        return !is_local_cached;
    }

    void GLCacheLocalCache::admitLocalCacheInternal_(const Key& key, const Value& value, const bool& is_neighbor_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        UNUSED(is_neighbor_cached); // ONLY used by COVERED
        UNUSED(affect_victim_tracker); // ONLY used by COVERED

        // Check is local cached
        request_t req = buildRequest_(key);
        cache_ck_res_e result = glcache_ptr_->exists(glcache_ptr_, &req);
        bool is_local_cached = (result == cache_ck_hit);

        if (!is_local_cached) // Admit if NOT exist
        {
            request_t tmp_req = buildRequest_(key, value);
            cache_obj_t* tmp_cache_obj_ptr = glcache_ptr_->insert(glcache_ptr_, &tmp_req);
            bool tmp_is_local_cached = (tmp_cache_obj_ptr != NULL);
            assert(tmp_is_local_cached);

            is_successful = true;
        }
        else
        {
            is_successful = false;
        }

        return;
    }

    bool GLCacheLocalCache::getLocalCacheVictimKeysInternal_(std::unordered_set<Key, KeyHasher>& keys, std::list<VictimCacheinfo>& victim_cacheinfos, const uint64_t& required_size) const
    {
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "getLocalCacheVictimKeysInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    bool GLCacheLocalCache::evictLocalCacheWithGivenKeyInternal_(const Key& key, Value& value)
    {        
        assert(hasFineGrainedManagement());

        Util::dumpErrorMsg(instance_name_, "evictLocalCacheWithGivenKeyInternal_() is not supported due to coarse-grained management");
        exit(1);

        return false;
    }

    void GLCacheLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        UNUSED(required_size);

        cache_obj_t* evicted_obj = static_cast<cache_obj_t*>(malloc(sizeof(cache_obj_t)));
        assert(evicted_obj != NULL);
        memset(evicted_obj, 0, sizeof(cache_obj_t));
        assert(evicted_obj->is_keybased_obj == false);
        glcache_ptr_->evict(glcache_ptr_, NULL, evicted_obj);

        // Convert evicted objs into victims
        std::vector<cache_obj_t*> objptrs; // used for memory free
        if (evicted_obj->is_keybased_obj == true) // Must memcpy some cached object(s) into evicted_obj, i.e., with victim(s)
        {
            cache_obj_t* tmp_objptr = evicted_obj; // Shallow copy
            while (tmp_objptr != NULL)
            {
                assert(tmp_objptr->is_keybased_obj == true);

                objptrs.push_back(tmp_objptr);
                if (victims.find(tmp_objptr->key) == victims.end()) // NOT found
                {
                    victims.insert(std::pair(Key(tmp_objptr->key), Value(tmp_objptr->value.length())));
                }

                tmp_objptr = tmp_objptr->evict_next;
            }
        }
        else // No victim found
        {
            assert(evicted_obj->evict_next == NULL);
            
            objptrs.push_back(evicted_obj);
        }

        // Release allocated cache objects
        for (uint32_t i = 0; i < objptrs.size(); i++)
        {
            assert(objptrs[i] != NULL);
            free(objptrs[i]);
            objptrs[i] = NULL;
        }
        evicted_obj = NULL; // I.e., objptrs[0]

        return;
    }

    // (4) Other functions

    void GLCacheLocalCache::invokeCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr)
    {
        std::ostringstream oss;
        oss << "invokeCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void GLCacheLocalCache::invokeConstCustomFunctionInternal_(const std::string& func_name, CacheCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "invokeConstCustomFunctionInternal_() does NOT support func_name " << func_name;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    uint64_t GLCacheLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = glcache_ptr_->occupied_size;

        return internal_size;
    }

    void GLCacheLocalCache::checkPointersInternal_() const
    {
        assert(glcache_ptr_ != NULL);
        return;
    }

    bool GLCacheLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here (glcache uses # of objects as segment size limitation instead of bytes)
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

    request_t GLCacheLocalCache::buildRequest_(const Key& key, const Value& value)
    {
        // Refer to src/cache/glcache/micro-implementation/libCacheSim/include/libCacheSim/request.h for default settings of some fields

        request_t req;
        req.real_time = Util::getCurrentTimeUs();
        req.hv = 0; // src/cache/glcache/micro-implementation/libCacheSim/cache/eviction/GLCache/GLCache.c will hash key to get a hash value
        req.obj_id = 0; // NOT used by glcache due to key-based lookup
        req.obj_size = key.getKeyLength() + value.getValuesize();
        req.ttl = 0; // NOT used by glcache
        req.op = OP_INVALID; // NOT used by glcache

        req.n_req = 0; // NOT used by glcache
        req.next_access_vtime = -1; // NOTE: we should NOT provide next access time in req, which is invalid assumption in practice
        req.key_size = key.getKeyLength(); // NOT used by glcache
        req.val_size = value.getValuesize(); // NOT used by glcache

        req.ns = 0; // NOT used by glcache
        req.content_type = 0; // NOT used by glcache
        req.tenant_id = 0; // NOT used by glcache

        req.bucket_id = 0; // NOT used by glcache due to disablling TTL
        req.age = 0; // NOT used by glcache
        req.hostname = 0; // NOT used by glcache
        req.extension = 0; // NOT used by glcache
        req.colo = 0; // NOT used by glcache
        req.n_level = 0; // NOT used by glcache
        req.n_param = 0; // NOT used by glcache
        req.method = 0; // NOT used by glcache

        req.valid = true;

        req.is_keybased_req = true;
        req.key = key.getKeystr();
        req.value = value.generateValuestrForStorage();

        return req;
    }

}
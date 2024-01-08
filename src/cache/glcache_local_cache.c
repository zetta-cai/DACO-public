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

    GLCacheLocalCache::GLCacheLocalCache(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& edge_idx, const uint64_t& capacity_bytes) : LocalCacheBase(edge_wrapper_ptr, edge_idx, capacity_bytes)
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
        UNUSED(is_redirected); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED

        request_t req = buildRequest_(key);
        cache_ck_res_e result = glcache_ptr_->get(glcache_ptr_, &req);
        value = req.value;
        bool is_local_cached = (result == cache_ck_hit);

        return is_local_cached;
    }

    std::list<VictimCacheinfo> GLCacheLocalCache::getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() const
    {
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos;

        Util::dumpErrorMsg(instance_name_, "getLocalSyncedVictimCacheinfosFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return local_synced_victim_cacheinfos;
    }

    void GLCacheLocalCache::getCollectedPopularityFromLocalCacheInternal_(const Key& key, CollectedPopularity& collected_popularity) const
    {
        Util::dumpErrorMsg(instance_name_, "getCollectedPopularityFromLocalCacheInternal_() can ONLY be invoked by COVERED local cache!");
        exit(1);

        return;
    }

    bool GLCacheLocalCache::updateLocalCacheInternal_(const Key& key, const Value& value, const bool& is_getrsp, const bool& is_global_cached, bool& affect_victim_tracker, bool& is_successful)
    {
        const bool is_valid_objsize = isValidObjsize_(key, value); // Object size checking

        UNUSED(is_getrsp); // ONLY for COVERED
        UNUSED(is_global_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // Only for COVERED
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
        UNUSED(is_neighbor_cached); // ONLY for COVERED
        UNUSED(affect_victim_tracker); // ONLY for COVERED

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

    // TODO: END HERE
    void S3fifoLocalCache::evictLocalCacheNoGivenKeyInternal_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size)
    {
        assert(!hasFineGrainedManagement());

        UNUSED(required_size);

        s3fifo_cache_ptr_->evictNoGivenKey(victims);

        return;
    }

    // (4) Other functions

    void S3fifoLocalCache::updateLocalCacheMetadataInternal_(const Key& key, const std::string& func_name, const void* func_param_ptr)
    {
        Util::dumpErrorMsg(instance_name_, "updateLocalCacheMetadataInternal_() is ONLY for COVERED!");
        exit(1);
        return;
    }

    uint64_t S3fifoLocalCache::getSizeForCapacityInternal_() const
    {
        uint64_t internal_size = s3fifo_cache_ptr_->getSizeForCapacity();

        return internal_size;
    }

    void S3fifoLocalCache::checkPointersInternal_() const
    {
        assert(s3fifo_cache_ptr_ != NULL);
        return;
    }

    bool S3fifoLocalCache::checkObjsizeInternal_(const ObjectSize& objsize) const
    {
        // NOTE: capacity has been checked by LocalCacheBase, while NO other custom object size checking here
        const bool is_valid_objsize = true;
        return is_valid_objsize;
    }

    request_t GLCacheLocalCache::buildRequest_(const Key& key, const Value& value) const
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
        req.next_access_vtime; // NOTE: we should NOT provide next access time in req, which is invalid assumption in practice
        req.key_size = key.getKeyLength(); // NOT used by glcache
        req.value_size = value.getValuesize(); // NOT used by glcache

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
        req.key = key;
        req.value = value;

        return req;
    }

}
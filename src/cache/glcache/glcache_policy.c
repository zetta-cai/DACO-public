#include "cache/glcache/glcache_policy.h"

#include "src/cache/glcache/GLCache.h"

namespace covered
{
    const std::string GLCachePolicy::kClassName("GLCachePolicy");

    GLCachePolicy::GLCachePolicy(const uint64_t& capacity_bytes)
    {
        // Refer to the same settings in lib/glcache/micro-implementation/libCacheSim/bin/cachesim/cli.c and lib/glcache/micro-implementation/test/test_glcache.c
        common_cache_params_t cc_params = {
            .cache_size = capacity_bytes,
            .default_ttl = 86400 * 300,
            .hashpower = 24,
            .consider_obj_metadata = true,
        };

        // Allocate and initialized cache-level metadata
        cache_metadata_ptr_ = cpp_GLCache_init(cc_params, NULL, this);
        assert(cache_metadata_ptr_ != NULL);
    }

    GLCachePolicy::~GLCachePolicy()
    {
        // Release cache-level metadata
        cpp_GLCache_free(cache_metadata_ptr_);
        cache_metadata_ptr_ = NULL;
    }
};
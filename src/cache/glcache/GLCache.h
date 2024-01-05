//
//  GLCache.h
//  libCacheSim
//

#pragma once

#include <libCacheSim/cache.h> // Siyuan: lib/glcache/micro-implementation/build/include/libCacheSim/cache.h
typedef float feature_t;
typedef float pred_t;
typedef float train_y_t;

#ifdef __cplusplus
extern "C" {
#endif

cache_t *cpp_GLCache_init(const common_cache_params_t ccache_params, const char *cache_specific_params, covered::GLCachePolicy* glcache_policy_ptr);

void GLCache_free(cache_t *cache);

cache_ck_res_e GLCache_check(cache_t *cache, const request_t *req,
                             const bool update_cache);

cache_ck_res_e GLCache_get(cache_t *cache, const request_t *req);

cache_obj_t *GLCache_insert(cache_t *GLCache, const request_t *req);

void GLCache_evict(cache_t *cache, const request_t *req,
                   cache_obj_t *evicted_obj);

void GLCache_remove_obj(cache_t *cache, cache_obj_t *cache_obj);

void GLCache_remove(cache_t *cache, const obj_id_t obj_id);

#ifdef __cplusplus
}
#endif

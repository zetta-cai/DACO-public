#include "cache/local_cache_base.h"

#include <assert.h>
#include <sstream>

#include "common/param/edge_param.h"
#include "common/util.h"
#include "cache/lru_local_cache.h"

namespace covered
{
    const std::string LocalCacheBase::kClassName("LocalCacheBase");

    LocalCacheBase* LocalCacheBase::getLocalCacheByCacheName(const std::string& cache_name, const uint32_t& edge_idx)
    {
        LocalCacheBase* local_cache_ptr = NULL;
        if (cache_name == EdgeParam::LRU_CACHE_NAME)
        {
            local_cache_ptr = new LruLocalCache(edge_idx);
        }
        else
        {
            std::ostringstream oss;
            oss << "local edge cache " << cache_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(local_cache_ptr != NULL);
        return local_cache_ptr;
    }

    LocalCacheBase::LocalCacheBase(const uint32_t& edge_idx)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();
    }

    LocalCacheBase::~LocalCacheBase() {}
}
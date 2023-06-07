/*
 * BasicCooperativeCacheWrapper: basic cooperative caching framework for baselines.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef BASIC_COOPERATIVE_CACHE_WRAPPER_H
#define BASIC_COOPERATIVE_CACHE_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperative_cache_wrapper_base.h"

namespace covered
{
    class BasicCooperativeCacheWrapper : public CooperativeCacheWrapperBase
    {
    public:
        BasicCooperativeCacheWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr);
        ~BasicCooperativeCacheWrapper();

        // Return if edge node is finished
        virtual bool get(const Key& key, Value& value, bool& is_cooperative_cached) override;
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool directoryLookup_(const Key& key, bool& is_cooperative_cached, uint32_t& neighbor_edge_idx);
    };
}

#endif
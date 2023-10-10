/*
 * CacheServerPlacementProcessorParam: parameters to launch a cache server placement processor in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.09.29).
 */

#ifndef CACHE_SERVER_PLACEMENT_PROCESSOR_PARAM_H
#define CACHE_SERVER_PLACEMENT_PROCESSOR_PARAM_H

#include <string>

namespace covered
{
    class CacheServerPlacementProcessorParam;
}

#include "common/key.h"
#include "common/value.h"
#include "concurrency/ring_buffer_impl.h"
#include "edge/cache_server/cache_server_item.h"
#include "edge/cache_server/cache_server.h"

namespace covered
{   
    class LocalCacheAdmissionItem
    {
    public:
        LocalCacheAdmissionItem();
        LocalCacheAdmissionItem(const Key& key, const Value& value, const bool& is_valid, const bool& skip_propagation_latency);
        ~LocalCacheAdmissionItem();

        Key getKey() const;
        Value getValue() const;
        bool isValid() const;
        bool skipPropagationLatency() const;

        const LocalCacheAdmissionItem& operator=(const LocalCacheAdmissionItem& other);    
    private:
        static const std::string kClassName;

        Key key_;
        Value value_;
        bool is_valid_;
        bool skip_propagation_latency_;
    };

    class CacheServerPlacementProcessorParam
    {
    public:
        CacheServerPlacementProcessorParam();
        CacheServerPlacementProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size);
        ~CacheServerPlacementProcessorParam();

        const CacheServerPlacementProcessorParam& operator=(const CacheServerPlacementProcessorParam& other);

        CacheServer* getCacheServerPtr() const;
        RingBuffer<CacheServerItem>* getNotifyPlacementRequestBufferPtr() const;
        RingBuffer<LocalCacheAdmissionItem>* getLocalCacheAdmissionBufferPtr() const;
    private:
        static const std::string kClassName;

        CacheServer* cache_server_ptr_; // thread safe
        RingBuffer<CacheServerItem>* notify_placement_request_buffer_ptr_; // thread safe (directory admission + local cache admission + eviction)
        RingBuffer<LocalCacheAdmissionItem>* local_cache_admission_buffer_ptr_; // thread safe (local cached admission + eviction)
    };
}

#endif
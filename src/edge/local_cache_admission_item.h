/*
 * LocalCacheAdmissionItem: a basic item for local cache admission (provided by cache server worker and beacon server, while consumed by cache server placement processor).
 * 
 * By Siyuan Sheng (2023.10.10).
 */

#ifndef LOCAL_CACHE_ADMISSION_ITEM_H
#define LOCAL_CACHE_ADMISSION_ITEM_H

#include <string>

#include "common/key.h"
#include "common/value.h"

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
}

#endif
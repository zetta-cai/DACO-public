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
#include "message/extra_common_msghdr.h"

namespace covered
{
    class LocalCacheAdmissionItem
    {
    public:
        LocalCacheAdmissionItem();
        LocalCacheAdmissionItem(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid, const ExtraCommonMsghdr& extra_common_msghdr);
        ~LocalCacheAdmissionItem();

        Key getKey() const;
        Value getValue() const;
        bool isNeighborCached() const;
        bool isValid() const;
        ExtraCommonMsghdr getExtraCommonMsghdr() const;

        const LocalCacheAdmissionItem& operator=(const LocalCacheAdmissionItem& other);    
    private:
        static const std::string kClassName;

        Key key_;
        Value value_;
        bool is_neighbor_cached_;
        bool is_valid_;
        ExtraCommonMsghdr extra_common_msghdr_;
    };
}

#endif
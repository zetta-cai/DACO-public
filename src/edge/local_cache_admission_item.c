#include "edge/local_cache_admission_item.h"

namespace covered
{
    const std::string LocalCacheAdmissionItem::kClassName("LocalCacheAdmissionItem");

    LocalCacheAdmissionItem::LocalCacheAdmissionItem() : key_(), value_()
    {
        is_valid_ = false;
        skip_propagation_latency_ = false;
    }

    LocalCacheAdmissionItem::LocalCacheAdmissionItem(const Key& key, const Value& value, const bool& is_valid, const bool& skip_propagation_latency)
    {
        key_ = key;
        value_ = value;
        is_valid_ = is_valid;
        skip_propagation_latency_ = skip_propagation_latency;
    }

    LocalCacheAdmissionItem::~LocalCacheAdmissionItem() {}

    Key LocalCacheAdmissionItem::getKey() const
    {
        return key_;
    }

    Value LocalCacheAdmissionItem::getValue() const
    {
        return value_;
    }

    bool LocalCacheAdmissionItem::isValid() const
    {
        return is_valid_;
    }

    bool LocalCacheAdmissionItem::skipPropagationLatency() const
    {
        return skip_propagation_latency_;
    }

    const LocalCacheAdmissionItem& LocalCacheAdmissionItem::operator=(const LocalCacheAdmissionItem& other)
    {
        key_ = other.key_;
        value_ = other.value_;
        is_valid_ = other.is_valid_;
        skip_propagation_latency_ = other.skip_propagation_latency_;

        return *this;
    }
}
#include "workload/workload_item.h"

namespace covered
{
    const std::string WorkloadItem::kClassName("WorkloadItem");

    WorkloadItem::WorkloadItem(const Key& key, const Value& value, const WorkloadItemType item_type)
    {
        key_ = key;
        value_ = value;
        item_type_ = item_type;
    }

    WorkloadItem::~WorkloadItem() {}

    Key WorkloadItem::getKey()
    {
        return key_;
    }

    Value WorkloadItem::getValue()
    {
        return value_;
    }

    WorkloadItemType WorkloadItem::getItemType()
    {
        return item_type_;
    }
}
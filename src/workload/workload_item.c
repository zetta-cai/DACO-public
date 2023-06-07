#include "workload/workload_item.h"

namespace covered
{
    const std::string WorkloadItem::kClassName("WorkloadItem");

    std::string WorkloadItem::workloadItemTypeToString(const WorkloadItemType& item_type)
    {
        std::string item_type_str = "";
        switch (item_type)
        {
            case WorkloadItemType::kWorkloadItemGet:
            {
                item_type_str = "kWorkloadItemGet";
                break;
            }
            case WorkloadItemType::kWorkloadItemPut:
            {
                item_type_str = "kWorkloadItemPut";
                break;
            }
            case WorkloadItemType::kWorkloadItemDel:
            {
                item_type_str = "kWorkloadItemDel";
                break;
            }
            default:
            {
                item_type_str = std::to_string(static_cast<uint32_t>(item_type));
                break;
            }
        }
        return item_type_str;
    }

    WorkloadItem::WorkloadItem(const Key& key, const Value& value, const WorkloadItemType& item_type)
    {
        key_ = key;
        value_ = value;
        item_type_ = item_type;
    }

    WorkloadItem::~WorkloadItem() {}

    Key WorkloadItem::getKey() const
    {
        return key_;
    }

    Value WorkloadItem::getValue() const
    {
        return value_;
    }

    WorkloadItemType WorkloadItem::getItemType()
    {
        return item_type_;
    }
}
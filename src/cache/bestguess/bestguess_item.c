#include "cache/bestguess/bestguess_item.h"

#include "common/util.h"

namespace covered
{
    BestGuessItem::BestGuessItem(const Key& key, const Value& value, const uint64_t& vtime)
    {
        key_ = key;
        value_ = value;
        vtime_ = vtime;
    }

    BestGuessItem::~BestGuessItem()
    {
    }

    Key BestGuessItem::getKey() const
    {
        return key_;
    }

    Value BestGuessItem::getValue() const
    {
        return value_;
    }

    uint64_t BestGuessItem::getVtime() const
    {
        return vtime_;
    }

    uint64_t BestGuessItem::getSizeForCapacity() const
    {
        return key_.getKeyLength() + value_.getValuesize() + sizeof(uint64_t);
    }

    const BestGuessItem& BestGuessItem::operator=(const BestGuessItem& item)
    {
        key_ = item.key_;
        value_ = item.value_;
        vtime_ = item.vtime_;
        return *this;
    }
}
/*
 * BestGuessItem: item of cached object in BestGuess.
 *
 * By Siyuan Sheng (2024.01.25).
 */

#ifndef BESTGUESS_ITEM_H
#define BESTGUESS_ITEM_H

#include <string>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class BestGuessItem
    {
    public:
        BestGuessItem();
        BestGuessItem(const Key& key, const Value& value, const uint64_t& vtime);
        ~BestGuessItem();

        Key getKey() const;
        Value getValue() const;
        uint64_t getVtime() const;

        uint64_t getSizeForCapacity() const;

        const BestGuessItem& operator=(const BestGuessItem& other);
    private:
        static const std::string kClassName;

        Key key_;
        Value value_;
        uint64_t vtime_; // Virtual time (order of request sequence)
    };
}

#endif
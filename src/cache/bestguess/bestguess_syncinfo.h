/*
 * BestGuessSyncinfo: synced recency information of BestGuess.
 *
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef BESTGUESS_SYNCINFO_H
#define BESTGUESS_SYNCINFO_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class BestGuessSyncinfo
    {
    public:
        BestGuessSyncinfo();
        BestGuessSyncinfo(const uint64_t& vtime);
        ~BestGuessSyncinfo();

        uint64_t getVtime() const;

        uint32_t getSyncinfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const BestGuessSyncinfo& operator=(const BestGuessSyncinfo& other);
    private:
        static const std::string kClassName;

        uint64_t vtime_;
    };
}

#endif
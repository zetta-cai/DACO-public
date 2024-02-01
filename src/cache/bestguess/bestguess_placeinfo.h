/*
 * BestGuessPlaceinfo: placement information of BestGuess.
 *
 * By Siyuan Sheng (2024.02.01).
 */

#ifndef BESTGUESS_PLACEINFO_H
#define BESTGUESS_PLACEINFO_H

#include <string>

#include "common/dynamic_array.h"

namespace covered
{
    class BestGuessPlaceinfo
    {
    public:
        BestGuessPlaceinfo();
        BestGuessPlaceinfo(const uint32_t& placement_edge_idx);
        ~BestGuessPlaceinfo();

        uint32_t getPlacementEdgeIdx() const;

        uint32_t getPlaceinfoPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const BestGuessPlaceinfo& operator=(const BestGuessPlaceinfo& other);
    private:
        static const std::string kClassName;

        uint32_t placement_edge_idx_;
    };
}

#endif
#include "cache/bestguess/bestguess_placeinfo.h"

namespace covered
{
    const std::string BestGuessPlaceinfo::kClassName("BestGuessPlaceinfo");

    BestGuessPlaceinfo::BestGuessPlaceinfo()
    {
        placement_edge_idx_ = 0;
    }

    BestGuessPlaceinfo::BestGuessPlaceinfo(const uint32_t& placement_edge_idx)
    {
        placement_edge_idx_ = placement_edge_idx;
    }

    BestGuessPlaceinfo::~BestGuessPlaceinfo()
    {}

    uint32_t BestGuessPlaceinfo::getPlaceinfoPayloadSize() const
    {
        return sizeof(uint32_t);
    }

    uint32_t BestGuessPlaceinfo::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&placement_edge_idx_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    uint32_t BestGuessPlaceinfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&placement_edge_idx_, sizeof(uint32_t));
        size += sizeof(uint32_t);
        return size - position;
    }

    const BestGuessPlaceinfo& BestGuessPlaceinfo::operator=(const BestGuessPlaceinfo& other)
    {
        placement_edge_idx_ = other.placement_edge_idx_;
        return *this;
    }
}
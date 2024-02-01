#include "cache/bestguess/bestguess_syncinfo.h"

namespace covered
{
    const std::string BestGuessSyncinfo::kClassName("BestGuessSyncinfo");

    BestGuessSyncinfo::BestGuessSyncinfo()
    {
        vtime_ = 0;
    }

    BestGuessSyncinfo::BestGuessSyncinfo(const uint64_t& vtime)
    {
        vtime_ = vtime;
    }

    BestGuessSyncinfo::~BestGuessSyncinfo()
    {}

    uint64_t BestGuessSyncinfo::getVtime() const
    {
        return vtime_;
    }

    uint32_t BestGuessSyncinfo::getSyncinfoPayloadSize() const
    {
        return sizeof(uint64_t);
    }

    uint32_t BestGuessSyncinfo::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;
        msg_payload.deserialize(size, (const char*)&vtime_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    uint32_t BestGuessSyncinfo::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;
        msg_payload.serialize(size, (char*)&vtime_, sizeof(uint64_t));
        size += sizeof(uint64_t);
        return size - position;
    }

    const BestGuessSyncinfo& BestGuessSyncinfo::operator=(const BestGuessSyncinfo& other)
    {
        vtime_ = other.vtime_;
        return *this;
    }
}
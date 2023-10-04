/*
 * CoveredVictimFetchRequest: a request issued by beacon edge node to fetch victims for cooperative cache placement calculation in COVERED (lazy victim fetching).
 * 
 * By Siyuan Sheng (2023.10.03).
 */

#ifndef COVERED_VICTIM_FETCH_REQUEST_H
#define COVERED_VICTIM_FETCH_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/uint_victimset_message.h"

namespace covered
{
    class CoveredVictimFetchRequest : public UintVictimsetMessage
    {
    public:
        CoveredVictimFetchRequest(const uint32_t& object_size, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr);
        CoveredVictimFetchRequest(const DynamicArray& msg_payload);
        virtual ~CoveredVictimFetchRequest();

        uint32_t getObjectSize() const;
    private:
        static const std::string kClassName;
    };
}

#endif
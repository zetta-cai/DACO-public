/*
 * CoveredMetadataUpdateRequest: a request issued by beacon edge node to the first/last cache copy to notify whether the object is neighbor cached after becoming non-single/single cache copy in the edge layer with victim synchroization for COVERED.
 * 
 * By Siyuan Sheng (2023.11.28).
 */

#ifndef COVERED_METADATA_UPDATE_REQUEST_H
#define COVERED_METADATA_UPDATE_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
//#include "message/key_byte_victimset_message.h"
#include "message/key_byte_message.h"

namespace covered
{
    //class CoveredMetadataUpdateRequest : public KeyByteVictimsetMessage
    class CoveredMetadataUpdateRequest : public KeyByteMessage
    {
    public:
        //CoveredMetadataUpdateRequest(const Key& key, const bool& is_neighbor_cached, const VictimSyncset& victim_syncset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredMetadataUpdateRequest(const Key& key, const bool& is_neighbor_cached, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredMetadataUpdateRequest(const DynamicArray& msg_payload);
        virtual ~CoveredMetadataUpdateRequest();

        bool isNeighborCached() const;
    private:
        static const std::string kClassName;
    };
}

#endif
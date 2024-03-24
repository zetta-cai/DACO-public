/*
 * DumpSnapshotResponse: a response issued by client to acknowledge DumpSnapshotRequest from evaluator.
 * 
 * By Siyuan Sheng (2023.07.28).
 */

#ifndef DUMP_SNAPSHOT_RESPONSE_H
#define DUMP_SNAPSHOT_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class DumpSnapshotResponse : public SimpleMessage
    {
    public:
        DumpSnapshotResponse(const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list, const uint64_t& msg_seqnum);
        DumpSnapshotResponse(const DynamicArray& msg_payload);
        virtual ~DumpSnapshotResponse();
    private:
        static const std::string kClassName;
    };
}

#endif
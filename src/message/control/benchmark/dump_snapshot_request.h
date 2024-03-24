/*
 * DumpSnapshotRequest: a request issued by evaluator to notify edge nodes to dump edge snapshots.
 * 
 * By Siyuan Sheng (2024.03.24).
 */

#ifndef DUMP_SNAPSHOT_REQUEST_H
#define DUMP_SNAPSHOT_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "message/simple_message.h"

namespace covered
{
    class DumpSnapshotRequest : public SimpleMessage
    {
    public:
        DumpSnapshotRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum);
        DumpSnapshotRequest(const DynamicArray& msg_payload);
        virtual ~DumpSnapshotRequest();
    private:
        static const std::string kClassName;
    };
}

#endif
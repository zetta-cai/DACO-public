#include "message/control/benchmark/dump_snapshot_request.h"

namespace covered
{
    const std::string DumpSnapshotRequest::kClassName("DumpSnapshotRequest");

    DumpSnapshotRequest::DumpSnapshotRequest(const uint32_t& source_index, const NetworkAddr& source_addr, const uint64_t& msg_seqnum) : SimpleMessage(MessageType::kDumpSnapshotRequest, source_index, source_addr, BandwidthUsage(), EventList(), ExtraCommonMsghdr(true, false, msg_seqnum)) // NOTE: ONLY msg seqnum in extra common msghdr will be used
    {
    }

    DumpSnapshotRequest::DumpSnapshotRequest(const DynamicArray& msg_payload) : SimpleMessage(msg_payload)
    {
    }

    DumpSnapshotRequest::~DumpSnapshotRequest() {}
}
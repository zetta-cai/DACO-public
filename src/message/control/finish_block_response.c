#include "message/control/finish_block_response.h"

namespace covered
{
    const std::string FinishBlockResponse::kClassName("FinishBlockResponse");

    FinishBlockResponse::FinishBlockResponse(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list) : KeyMessage(key, MessageType::kFinishBlockResponse, source_index, source_addr, event_list)
    {
    }

    FinishBlockResponse::FinishBlockResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    FinishBlockResponse::~FinishBlockResponse() {}
}
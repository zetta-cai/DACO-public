/*
 * FinishBlockResponse: a response issued by a closest edge node to acknowledge FinishBlockRequest from the beacon node.
 * 
 * By Siyuan Sheng (2023.06.16).
 */

#ifndef FINISH_BLOCK_RESPONSE_H
#define FINISH_BLOCK_RESPONSE_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class FinishBlockResponse : public KeyMessage
    {
    public:
        FinishBlockResponse(const Key& key, const uint32_t& source_index);
        FinishBlockResponse(const DynamicArray& msg_payload);
        virtual ~FinishBlockResponse();
    private:
        static const std::string kClassName;
    };
}

#endif
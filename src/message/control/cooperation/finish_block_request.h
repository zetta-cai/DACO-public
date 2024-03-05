/*
 * FinishBlockRequest: a request issued by a beacon node to a closest edge node to finish blocking for writes.
 * 
 * By Siyuan Sheng (2023.06.16).
 */

#ifndef FINISH_BLOCK_REQUEST_H
#define FINISH_BLOCK_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_message.h"

namespace covered
{
    class FinishBlockRequest : public KeyMessage
    {
    public:
        FinishBlockRequest(const Key& key, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        FinishBlockRequest(const DynamicArray& msg_payload);
        virtual ~FinishBlockRequest();
    private:
        static const std::string kClassName;
    };
}

#endif
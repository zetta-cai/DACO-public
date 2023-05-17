/*
 * RequestBase: the base class for different messages.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef MESSAGE_BASE_H
#define MESSAGE_BASE_H

#include <string>

#include "common/dynamic_array.h"
#include "workload/workload_item.h"

namespace covered
{
    enum MessageType
    {
        kLocalGetRequest = 1,
        kLocalGetResponse,
        kLocalPutRequest,
        kLocalPutResponse,
        kLocalDelRequest,
        kLocalDelResponse
    };

    class MessageBase
    {
    public:
        static MessageBase* getMessageFromWorkloadItem(WorkloadItem workload_item);

        MessageBase(const MessageType& message_type);
        ~MessageBase();

        virtual uint32_t getMsgPayloadSize() = 0;

        // Offset of request must be 0 in message payload
        virtual uint32_t serialize(DynamicArray& msg_payload) = 0;
        virtual uint32_t deserialize(const DynamicArray& msg_payload) = 0;

        bool isLocalRequest();
    private:
        static const std::string kClassName;

        MessageType message_type_;
    };
}

#endif
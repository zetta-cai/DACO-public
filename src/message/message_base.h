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
        kLocalPutRequest,
        kLocalDelResponse,
        kLocalGetResponse,
        kLocalPutResponse,
        kLocalDelRequest
    };

    class MessageBase
    {
    public:
        static MessageBase* getLocalRequestFromWorkloadItem(WorkloadItem workload_item); // By workers in clients
        static MessageBase* getRequestFromMsgPayload(const DynamicArray& msg_payload); // Data/control requests

        MessageBase(const MessageType& message_type);
        MessageBase(const DynamicArray& msg_payload);
        ~MessageBase();

        virtual uint32_t getMsgPayloadSize() = 0;

        // Offset of message must be 0 in message payload
        uint32_t serialize(DynamicArray& msg_payload);
        uint32_t deserialize(const DynamicArray& msg_payload);

        bool isDataRequest() const;
        bool isLocalRequest() const;
        bool isRedirectedRequest() const;

        bool isControlRequest() const;
    private:
        static const std::string kClassName;

        static uint32_t deserializeMessageTypeFromMsgPayload(const DynamicArray& msg_payload, MessageType& message_type);

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& size) = 0;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& size) = 0;

        MessageType message_type_;
    };
}

#endif
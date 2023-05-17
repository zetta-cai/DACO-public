#include "message/message_base.h"

#include <arpa/inet.h> // htonl ntohl
#include <sstream>

#include "common/util.h"
#include "message/local_get_request.h"
#include "message/local_put_request.h"
#include "message/local_del_request.h"

namespace covered
{
    const std::string MessageBase::kClassName("MessageBase");

    MessageBase* MessageBase::getLocalRequestFromWorkloadItem(WorkloadItem workload_item)
    {
        WorkloadItemType item_type = workload_item.getItemType();

        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (item_type)
        {
            case WorkloadItemType::kWorkloadItemGet:
            {
                message_ptr = new LocalGetRequest(workload_item.getKey());
                break;
            }
            case WorkloadItemType::kWorkloadItemPut:
            {
                message_ptr = new LocalPutRequest(workload_item.getKey(), workload_item.getValue());
                break;
            }
            case WorkloadItemType::kWorkloadItemDel:
            {
                message_ptr = new LocalDelRequest(workload_item.getKey());
                break;
            }
            default
            {
                std::ostringstream oss;
                oss << "WorkloadItemType " << static_cast<uint32_t>(item_type) << " is invalid!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        assert(message_ptr->isLocalRequest()); // Must be local request for clients
        return message_ptr;
    }

    MessageBase* MessageBase::getRequestFromMsgPayload(const DynamicArray& msg_payload)
    {
        // Get message type
        MessageType message_type;
        deserializeMessageTypeFromMsgPayload(msg_payload, message_type);

        // Get message based on message type
        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (message_type)
        {
            case MessageType::kLocalGetRequest:
            {
                message_ptr = new GetRequest(msg_payload);
                break;
            }
            case MessageType::kLocalPutRequest:
            {
                message_ptr = new PutRequest(msg_payload);
                break;
            }
            case MessageType::kLocalDelRequest:
            {
                message_ptr = new DelRequest(msg_payload);
                break;
            }
            default
            {
                std::ostringstream oss;
                oss << "MessageType " << static_cast<uint32_t>(message_type) << " is invalid!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        assert(message_ptr->isDataRequest() || message_ptr->isControlRequest());
        return message_ptr;
    }

    uint32_t deserializeMessageTypeFromMsgPayload(const DynamicArray& msg_payload, MessageType& message_type)
    {
        uint32_t message_type_value = 0;
        msg_payload.read(size, (const char *)&message_type_value, sizeof(uint32_t));
        message_type_value = ntohl(message_type_value);
        message_type = static_cast<MessageType>(message_type_value);
        return sizeof(uint32_t);
    }

    MessageBase::MessageBase(const MessageType& message_type) : message_type_(message_type)
    {
    }

    MessageBase::MessageBase(const DynamicArray& msg_payload)
    {
        deserialize(msg_payload);
    }
    
    MessageBase::~MessageBase() {}

    uint32_t MessageBase::serialize(DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t bigendian_message_type_value = htonl(static_cast<uint32_t>(message_type_));
        msg_payload.write(size, (const char *)&bigendian_message_type_value, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t internal_size = serializeInternal_(msg_payload, size);
        size += internal_size;
        return size;
    }
    
    uint32_t MessageBase::deserialize(const DynamicArray& msg_payload)
    {
        uint32_t size = 0;
        uint32_t message_type_size = deserializeMessageTypeFromMsgPayload(msg_payload, message_type_);
        size += message_type_size;
        uint32_t internal_size = deserializeInternal_(msg_payload, size);
        size += internal_size;
        return size;
    }

    bool MessageBase::isDataMessage() const
    {
        return isLocalRequest() || isRedirectedRequest();
    }

    bool MessageBase::isLocalRequest() const
    {
        if (message_type_ == MessageType::kLocalGetRequest || message_type_ == MessageType::::kLocalPutRequest || message_type_ == MessageType::kLocalDelRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isRedirectedRequest() const
    {
        // TODO: Update isRedirectedRequest() after introducing redirected requests
        return false;
    }
}

#endif
#include "message/message_base.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "message/data_message.h"
#include "message/control_message.h"

namespace covered
{
    const std::string MessageBase::kClassName("MessageBase");

    std::string MessageBase::messageTypeToString(const MessageType& message_type)
    {
        std::string message_type_str = "";
        switch (message_type)
        {
            case MessageType::kLocalGetRequest:
            {
                message_type_str = "kLocalGetRequest";
                break;
            }
            case MessageType::kLocalPutRequest:
            {
                message_type_str = "kLocalPutRequest";
                break;
            }
            case MessageType::kLocalDelRequest:
            {
                message_type_str = "kLocalDelRequest";
                break;
            }
            case MessageType::kLocalGetResponse:
            {
                message_type_str = "kLocalGetResponse";
                break;
            }
            case MessageType::kLocalPutResponse:
            {
                message_type_str = "kLocalPutResponse";
                break;
            }
            case MessageType::kLocalDelResponse:
            {
                message_type_str = "kLocalDelResponse";
                break;
            }
            case MessageType::kRedirectedGetRequest:
            {
                message_type_str = "kRedirectedGetRequest";
                break;
            }
            case MessageType::kRedirectedGetResponse:
            {
                message_type_str = "kRedirectedGetResponse";
                break;
            }
            case MessageType::kDirectoryLookupRequest:
            {
                message_type_str = "kDirectoryLookupRequest";
                break;
            }
            case MessageType::kDirectoryUpdateRequest:
            {
                message_type_str = "kDirectoryUpdateRequest";
                break;
            }
            case MessageType::kDirectoryLookupResponse:
            {
                message_type_str = "kDirectoryLookupResponse";
                break;
            }
            case MessageType::kDirectoryUpdateResponse:
            {
                message_type_str = "kDirectoryUpdateResponse";
                break;
            }
            default:
            {
                message_type_str = std::to_string(static_cast<uint32_t>(message_type));
                break;
            }
        }
        return message_type_str;
    }

    std::string MessageBase::hitflagToString(const Hitflag& hitflag)
    {
        std::string hitflag_str = "";
        switch (hitflag)
        {
            case Hitflag::kLocalHit:
            {
                hitflag_str = "kLocalHit";
                break;
            }
            case Hitflag::kCooperativeHit:
            {
                hitflag_str = "kCooperativeHit";
                break;
            }
            case Hitflag::kGlobalMiss:
            {
                hitflag_str = "kGlobalMiss";
                break;
            }
            default:
            {
                hitflag_str = std::to_string(static_cast<uint8_t>(hitflag));
                break;
            }
        }
        return hitflag_str;
    }

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
            default:
            {
                std::ostringstream oss;
                oss << "invalid workload item type " << WorkloadItem::workloadItemTypeToString(item_type) << " for getLocalRequestFromWorkloadItem()!";
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
                message_ptr = new LocalGetRequest(msg_payload);
                break;
            }
            case MessageType::kLocalPutRequest:
            {
                message_ptr = new LocalPutRequest(msg_payload);
                break;
            }
            case MessageType::kLocalDelRequest:
            {
                message_ptr = new LocalDelRequest(msg_payload);
                break;
            }
            case MessageType::kGlobalGetRequest:
            {
                message_ptr = new GlobalGetRequest(msg_payload);
                break;
            }
            case MessageType::kGlobalPutRequest:
            {
                message_ptr = new GlobalPutRequest(msg_payload);
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                message_ptr = new GlobalDelRequest(msg_payload);
                break;
            }
            case MessageType::kRedirectedGetRequest:
            {
                message_ptr = new RedirectedGetRequest(msg_payload);
                break;
            }
            case MessageType::kDirectoryLookupRequest:
            {
                message_ptr = new DirectoryLookupRequest(msg_payload);
                break;
            }
            case MessageType::kDirectoryUpdateRequest:
            {
                message_ptr = new DirectoryUpdateRequest(msg_payload);
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for getRequestFromMsgPayload()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        assert(message_ptr->isDataRequest() || message_ptr->isControlRequest());
        return message_ptr;
    }

    MessageBase* MessageBase::getResponseFromMsgPayload(const DynamicArray& msg_payload)
    {
        // Get message type
        MessageType message_type;
        deserializeMessageTypeFromMsgPayload(msg_payload, message_type);

        // Get message based on message type
        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (message_type)
        {
            case MessageType::kLocalGetResponse:
            {
                message_ptr = new LocalGetResponse(msg_payload);
                break;
            }
            case MessageType::kLocalPutResponse:
            {
                message_ptr = new LocalPutResponse(msg_payload);
                break;
            }
            case MessageType::kLocalDelResponse:
            {
                message_ptr = new LocalDelResponse(msg_payload);
                break;
            }
            case MessageType::kGlobalGetResponse:
            {
                message_ptr = new GlobalGetResponse(msg_payload);
                break;
            }
            case MessageType::kGlobalPutResponse:
            {
                message_ptr = new GlobalPutResponse(msg_payload);
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                message_ptr = new GlobalDelResponse(msg_payload);
                break;
            }
            case MessageType::kRedirectedGetResponse:
            {
                message_ptr = new RedirectedGetResponse(msg_payload);
                break;
            }
            case MessageType::kDirectoryLookupResponse:
            {
                message_ptr = new DirectoryLookupResponse(msg_payload);
                break;
            }
            case MessageType::kDirectoryUpdateResponse:
            {
                message_ptr = new DirectoryUpdateResponse(msg_payload);
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for getResponseFromMsgPayload()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        assert(message_ptr->isDataResponse() || message_ptr->isControlResponse());
        return message_ptr;
    }

    Key MessageBase::getKeyFromMessage(MessageBase* message_ptr)
    {
        assert(message_ptr != NULL);

        Key tmp_key;
        if (message_ptr->message_type_ == MessageType::kLocalGetRequest)
        {
            const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(message_ptr);
            tmp_key = local_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kLocalPutRequest)
        {
            const LocalPutRequest* const local_put_request_ptr = static_cast<const LocalPutRequest*>(message_ptr);
            tmp_key = local_put_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kLocalDelRequest)
        {
            const LocalDelRequest* const local_del_request_ptr = static_cast<const LocalDelRequest*>(message_ptr);
            tmp_key = local_del_request_ptr->getKey();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << getRequestFromMsgPayload(message_ptr->message_type_) << " for getKeyFromMessage()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return tmp_key;
    }

    uint32_t MessageBase::deserializeMessageTypeFromMsgPayload(const DynamicArray& msg_payload, MessageType& message_type)
    {
        const uint32_t size = 0;
        uint32_t message_type_value = 0;
        msg_payload.serialize(size, (char *)&message_type_value, sizeof(uint32_t));
        message_type_value = ntohl(message_type_value);
        message_type = static_cast<MessageType>(message_type_value);
        return sizeof(uint32_t);
    }

    MessageBase::MessageBase(const MessageType& message_type)
    {
        message_type_ = message_type;
        is_valid_ = true;
    }

    // NOTE: CANNOT call pure method in constructor or destructor, as the derived object has not been contructed or has already been destructed
    //MessageBase::MessageBase(const DynamicArray& msg_payload)
    //{
    //    deserialize(msg_payload);
    //}

    MessageBase::MessageBase()
    {
        is_valid_ = false;
    }
    
    MessageBase::~MessageBase() {}

    MessageType MessageBase::getMessageType() const
    {
        checkIsValid_();
        return message_type_;
    }

    uint32_t MessageBase::getMsgPayloadSize() const
    {
        checkIsValid_();

        // Message type size + internal payload size
        return sizeof(uint32_t) + getMsgPayloadSizeInternal_();
    }

    uint32_t MessageBase::serialize(DynamicArray& msg_payload) const
    {
        checkIsValid_();

        uint32_t size = 0;
        uint32_t bigendian_message_type_value = htonl(static_cast<uint32_t>(message_type_));
        msg_payload.deserialize(size, (const char *)&bigendian_message_type_value, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t internal_size = serializeInternal_(msg_payload, size);
        size += internal_size;
        return size - 0;
    }
    
    uint32_t MessageBase::deserialize(const DynamicArray& msg_payload)
    {
        // deserialize() can only be invoked once after the default constructor
        assert(!is_valid_);
        is_valid_ = true;

        uint32_t size = 0;
        uint32_t message_type_size = deserializeMessageTypeFromMsgPayload(msg_payload, message_type_);
        size += message_type_size;
        uint32_t internal_size = this->deserializeInternal_(msg_payload, size);
        size += internal_size;
        return size - 0;
    }

    bool MessageBase::isDataRequest() const
    {
        checkIsValid_();
        return isLocalRequest() || isRedirectedRequest() || isGlobalRequest();
    }

    bool MessageBase::isLocalRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kLocalGetRequest || message_type_ == MessageType::kLocalPutRequest || message_type_ == MessageType::kLocalDelRequest)
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
        checkIsValid_();
        if (message_type_ == MessageType::kRedirectedGetRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isGlobalRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kGlobalGetRequest || message_type_ == MessageType::kGlobalPutRequest || message_type_ == MessageType::kGlobalDelRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isDataResponse() const
    {
        checkIsValid_();
        return isLocalResponse() || isRedirectedResponse() || isGlobalResponse();
    }

    bool MessageBase::isLocalResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kLocalGetResponse || message_type_ == MessageType::kLocalPutResponse || message_type_ == MessageType::kLocalDelResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isRedirectedResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kRedirectedGetResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isGlobalResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kGlobalGetResponse || message_type_ == MessageType::kGlobalPutResponse || message_type_ == MessageType::kGlobalDelResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isControlRequest() const
    {
        checkIsValid_();
        // TODO: Update isControlRequest() after introducing control requests
        if (message_type_ == MessageType::kDirectoryLookupRequest || message_type_ == MessageType::kDirectoryUpdateRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isControlResponse() const
    {
        checkIsValid_();
        // TODO: Update isControlResponse() after introducing control responses
        if (message_type_ == MessageType::kDirectoryLookupResponse || message_type_ == MessageType::kDirectoryUpdateResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void MessageBase::checkIsValid_() const
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid MessageBase (is_valid_ is still false)!");
            exit(1);
        }
        return;
    }
}
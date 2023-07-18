/*
 * RequestBase: the base class for different messages.
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef MESSAGE_BASE_H
#define MESSAGE_BASE_H

#include <string>

#include "common/dynamic_array.h"
#include "event/event_list.h"
#include "workload/workload_item.h"
#include "network/network_addr.h"

namespace covered
{
    enum MessageType
    {
        kLocalGetRequest = 1,
        kLocalPutRequest,
        kLocalDelRequest,
        kLocalGetResponse,
        kLocalPutResponse,
        kLocalDelResponse,
        kGlobalGetRequest,
        kGlobalPutRequest,
        kGlobalDelRequest,
        kGlobalGetResponse,
        kGlobalPutResponse,
        kGlobalDelResponse,
        kRedirectedGetRequest,
        kRedirectedGetResponse,
        kAcquireWritelockRequest,
        kAcquireWritelockResponse,
        kDirectoryLookupRequest,
        kDirectoryUpdateRequest,
        kDirectoryLookupResponse,
        kDirectoryUpdateResponse,
        kFinishBlockRequest,
        kFinishBlockResponse,
        kInvalidationRequest,
        kInvalidationResponse,
        kReleaseWritelockRequest,
        kReleaseWritelockResponse
    };

    enum Hitflag
    {
        kLocalHit = 1, // Hit local edge cache of closest edge node
        kCooperativeHit, // Hit cooperative edge cache of some target edge node with valid object
        kCooperativeInvalid, // Hit cooperative edge cache of some target edge node yet with invalid object (only used in edge nodes)
        kGlobalMiss // Miss all edge nodes
    };

    enum LockResult
    {
        kSuccess = 1, // Acquire write lock successfully for a cooperatively cached key
        kFailure, // NOT acquire write lock for a cooperatively cached key
        kNoneed // NO need to acquire write lock for a globally uncached key
    };

    class MessageBase
    {
    public:
        static std::string messageTypeToString(const MessageType& message_type);
        static std::string hitflagToString(const Hitflag& hitflag);
        static std::string lockResultToString(const LockResult& lock_result);

        static MessageBase* getLocalRequestFromWorkloadItem(WorkloadItem workload_item, const uint32_t& source_index, const NetworkAddr& source_addr); // By workers in clients
        static MessageBase* getRequestFromMsgPayload(const DynamicArray& msg_payload); // Data/control requests
        static MessageBase* getResponseFromMsgPayload(const DynamicArray& msg_payload); // Data/control responses
        static Key getKeyFromMessage(MessageBase* message_ptr); // Get key from message (e.g., local requests)

        MessageBase(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const EventList& event_list);
        //MessageBase(const DynamicArray& msg_payload);
        MessageBase();
        virtual ~MessageBase();

        MessageType getMessageType() const;
        uint32_t getSourceIndex() const;
        NetworkAddr getSourceAddr() const;
        const EventList& getEventListRef() const;

        uint32_t getMsgPayloadSize() const;

        // Offset of message must be 0 in message payload
        // Message payload format: message_type + source index + internal payload
        uint32_t serialize(DynamicArray& msg_payload) const;
        uint32_t deserialize(const DynamicArray& msg_payload);

        bool isDataRequest() const;
        bool isLocalRequest() const;
        bool isRedirectedRequest() const;
        bool isGlobalRequest() const;

        bool isDataResponse() const;
        bool isLocalResponse() const;
        bool isRedirectedResponse() const;
        bool isGlobalResponse() const;

        bool isControlRequest() const;
        bool isControlResponse() const;
    private:
        static const std::string kClassName;

        static uint32_t deserializeMessageTypeFromMsgPayload(const DynamicArray& msg_payload, MessageType& message_type);

        virtual uint32_t getMsgPayloadSizeInternal_() const = 0;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& size) const = 0;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& size) = 0;

        MessageType message_type_;
        uint32_t source_index_; // global-client-worker/edge/cloud index of source node
        NetworkAddr source_addr_; // Network address of source socket server to hide propagation simulator
        EventList event_list_; // Track events to break down latencies for debugging (NOTE: NOT consume bandwidth if without event tracking)

        bool is_valid_; // NOT serialized/deserialized in msg payload
    protected:
        void checkIsValid_() const;
    };
}

#endif
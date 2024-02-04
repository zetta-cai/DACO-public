/*
 * RequestBase: the base class for different messages.
 *
 * Conditional serialization/deserialization: (i) In CoveredDirectoryUpdateRequest and CoveredPlacementUpdateRequest, is_admit determines whether to embed CollectedPopularity; (ii) In CollectedPopularity, is_tracked determines whether to embed local uncached popularity and object size; (iii) In VictimSyncset, (TODO for delta-based victim synchronization).
 * 
 * By Siyuan Sheng (2023.05.17).
 */

#ifndef MESSAGE_BASE_H
#define MESSAGE_BASE_H

#include <string>

#include "common/bandwidth_usage.h"
#include "common/dynamic_array.h"
#include "event/event_list.h"
#include "workload/workload_item.h"
#include "network/network_addr.h"

namespace covered
{
    enum MessageType
    {
        // Local data messages
        kLocalGetRequest = 1,
        kLocalPutRequest,
        kLocalDelRequest,
        kLocalGetResponse,
        kLocalPutResponse,
        kLocalDelResponse,
        // Global data messages
        kGlobalGetRequest,
        kGlobalPutRequest,
        kGlobalDelRequest,
        kGlobalGetResponse,
        kGlobalPutResponse,
        kGlobalDelResponse,
        // Redirected data messages
        kRedirectedGetRequest,
        kRedirectedGetResponse,
        // Benchmark control message
        kInitializationRequest,
        kInitializationResponse,
        kStartrunRequest,
        kStartrunResponse,
        kSwitchSlotRequest,
        kSwitchSlotResponse,
        kFinishWarmupRequest,
        kFinishWarmupResponse,
        kFinishrunRequest,
        kFinishrunResponse,
        kSimpleFinishrunResponse,
        // Cooperation control messages
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
        kReleaseWritelockResponse,
        // ONLY used by COVERED
        kCoveredDirectoryLookupRequest,
        kCoveredDirectoryLookupResponse,
        kCoveredDirectoryUpdateRequest,
        kCoveredDirectoryUpdateResponse,
        kCoveredAcquireWritelockRequest,
        kCoveredAcquireWritelockResponse,
        kCoveredReleaseWritelockRequest,
        kCoveredReleaseWritelockResponse,
        kCoveredRedirectedGetRequest,
        kCoveredRedirectedGetResponse,
        kCoveredBgfetchRedirectedGetRequest,
        kCoveredBgfetchRedirectedGetResponse,
        kCoveredBgfetchGlobalGetRequest,
        kCoveredBgfetchGlobalGetResponse,
        kCoveredBgplacePlacementNotifyRequest,
        kCoveredBgplaceDirectoryUpdateRequest,
        kCoveredBgplaceDirectoryUpdateResponse,
        kCoveredVictimFetchRequest,
        kCoveredVictimFetchResponse,
        kCoveredFghybridDirectoryLookupResponse,
        kCoveredFghybridDirectoryEvictResponse,
        kCoveredFghybridReleaseWritelockResponse,
        kCoveredFghybridHybridFetchedRequest,
        kCoveredFghybridHybridFetchedResponse,
        kCoveredFghybridDirectoryAdmitRequest,
        kCoveredFghybridDirectoryAdmitResponse,
        kCoveredInvalidationRequest,
        kCoveredInvalidationResponse,
        kCoveredFinishBlockRequest,
        kCoveredFinishBlockResponse,
        kCoveredFastpathDirectoryLookupResponse,
        kCoveredMetadataUpdateRequest,
        // ONLY used by BestGuess
        kBestGuessPlacementTriggerRequest,
        kBestGuessPlacementTriggerResponse,
        kBestGuessDirectoryUpdateRequest,
        kBestGuessDirectoryUpdateResponse,
        kBestGuessBgplaceDirectoryUpdateRequest,
        kBestGuessBgplaceDirectoryUpdateResponse,
        kBestGuessBgplacePlacementNotifyRequest,
        kBestGuessDirectoryLookupRequest,
        kBestGuessDirectoryLookupResponse,
        kBestGuessFinishBlockRequest,
        kBestGuessFinishBlockResponse,
        kBestGuessAcquireWritelockRequest,
        kBestGuessAcquireWritelockResponse,
        kBestGuessInvalidationRequest,
        kBestGuessInvalidationResponse,
        kBestGuessReleaseWritelockRequest,
        kBestGuessReleaseWritelockResponse,
        kBestGuessRedirectedGetRequest,
        kBestGuessRedirectedGetResponse,
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

        static MessageBase* getRequestFromWorkloadItem(WorkloadItem workload_item, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& is_warmup_phase, const bool& is_warmup_speedup); // By workers in clients
        static MessageBase* getRequestFromMsgPayload(const DynamicArray& msg_payload); // Data/control requests
        static MessageBase* getResponseFromMsgPayload(const DynamicArray& msg_payload); // Data/control responses
        static Key getKeyFromMessage(MessageBase* message_ptr); // Get key from message (e.g., local requests)

        MessageBase(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency);
        //MessageBase(const DynamicArray& msg_payload);
        MessageBase();
        virtual ~MessageBase();

        MessageType getMessageType() const;
        uint32_t getSourceIndex() const;
        NetworkAddr getSourceAddr() const;
        const BandwidthUsage& getBandwidthUsageRef() const;
        const EventList& getEventListRef() const;
        bool isSkipPropagationLatency() const;

        uint32_t getMsgPayloadSize() const;

        // Offset of message must be 0 in message payload
        // Message payload format: message_type + source index + internal payload
        uint32_t serialize(DynamicArray& msg_payload) const;
        uint32_t deserialize(const DynamicArray& msg_payload);

        bool isDataRequest() const;
        bool isLocalDataRequest() const;
        bool isRedirectedDataRequest() const;
        bool isGlobalDataRequest() const;

        bool isDataResponse() const;
        bool isLocalDataResponse() const;
        bool isRedirectedDataResponse() const;
        bool isGlobalDataResponse() const;

        bool isControlRequest() const;
        bool isCooperationControlRequest() const;
        bool isBenchmarkControlRequest() const;

        bool isControlResponse() const;
        bool isCooperationControlResponse() const;
        bool isBenchmarkControlResponse() const;

        bool isBackgroundMessage() const;
        bool isBackgroundRequest() const;
        bool isBackgroundResponse() const;
    private:
        static const std::string kClassName;

        static uint32_t deserializeMessageTypeFromMsgPayload(const DynamicArray& msg_payload, MessageType& message_type);

        virtual uint32_t getMsgPayloadSizeInternal_() const = 0;

        virtual uint32_t serializeInternal_(DynamicArray& msg_payload, const uint32_t& size) const = 0;
        virtual uint32_t deserializeInternal_(const DynamicArray& msg_payload, const uint32_t& size) = 0;

        MessageType message_type_;
        uint32_t source_index_; // client/edge/cloud index of source node
        NetworkAddr source_addr_; // Network address of source socket server to hide propagation simulator
        BandwidthUsage bandwidth_usage_; // NOTE: requests MUST have default bandwidth usage
        // Track intermediate events and events of intermediate responses to break down latencies for debugging
        // NOTE: requests MUST have empty event list; NOT consume bandwidth if without event tracking
        EventList event_list_;
        bool skip_propagation_latency_; // NOT simulate propagation latency for warmup speedup

        bool is_valid_; // NOT serialized/deserialized in msg payload
        bool is_response_; // NOT serialized/deserialized in msg payload
    protected:
        void checkIsValid_() const;
    };
}

#endif
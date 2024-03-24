#include "message/message_base.h"

#include <arpa/inet.h> // htonl ntohl
#include <assert.h>
#include <sstream>

#include "common/config.h"
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
            case MessageType::kInitializationRequest:
            {
                message_type_str = "kInitializationRequest";
                break;
            }
            case MessageType::kInitializationResponse:
            {
                message_type_str = "kInitializationResponse";
                break;
            }
            case MessageType::kStartrunRequest:
            {
                message_type_str = "kStartrunRequest";
                break;
            }
            case MessageType::kStartrunResponse:
            {
                message_type_str = "kStartrunResponse";
                break;
            }
            case MessageType::kSwitchSlotRequest:
            {
                message_type_str = "kSwitchSlotRequest";
                break;
            }
            case MessageType::kSwitchSlotResponse:
            {
                message_type_str = "kSwitchSlotResponse";
                break;
            }
            case MessageType::kFinishrunRequest:
            {
                message_type_str = "kFinishrunRequest";
                break;
            }
            case MessageType::kFinishrunResponse:
            {
                message_type_str = "kFinishrunResponse";
                break;
            }
            case MessageType::kDumpSnapshotRequest:
            {
                message_type_str = "kDumpSnapshotRequest";
                break;
            }
            case MessageType::kDumpSnapshotResponse:
            {
                message_type_str = "kDumpSnapshotResponse";
                break;
            }
            case MessageType::kSimpleFinishrunResponse:
            {
                message_type_str = "kSimpleFinishrunResponse";
                break;
            }
            case MessageType::kAcquireWritelockRequest:
            {
                message_type_str = "kAcquireWritelockRequest";
                break;
            }
            case MessageType::kAcquireWritelockResponse:
            {
                message_type_str = "kAcquireWritelockResponse";
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
            case MessageType::kFinishBlockRequest:
            {
                message_type_str = "kFinishBlockRequest";
                break;
            }
            case MessageType::kFinishBlockResponse:
            {
                message_type_str = "kFinishBlockResponse";
                break;
            }
            case MessageType::kInvalidationRequest:
            {
                message_type_str = "kInvalidationRequest";
                break;
            }
            case MessageType::kInvalidationResponse:
            {
                message_type_str = "kInvalidationResponse";
                break;
            }
            case MessageType::kReleaseWritelockRequest:
            {
                message_type_str = "kReleaseWritelockRequest";
                break;
            }
            case MessageType::kReleaseWritelockResponse:
            {
                message_type_str = "kReleaseWritelockResponse";
                break;
            }
            case MessageType::kCoveredDirectoryLookupRequest:
            {
                message_type_str = "kCoveredDirectoryLookupRequest";
                break;
            }
            case MessageType::kCoveredDirectoryLookupResponse:
            {
                message_type_str = "kCoveredDirectoryLookupResponse";
                break;
            }
            case MessageType::kCoveredDirectoryUpdateRequest:
            {
                message_type_str = "kCoveredDirectoryUpdateRequest";
                break;
            }
            case MessageType::kCoveredDirectoryUpdateResponse:
            {
                message_type_str = "kCoveredDirectoryUpdateResponse";
                break;
            }
            case MessageType::kCoveredAcquireWritelockRequest:
            {
                message_type_str = "kCoveredAcquireWritelockRequest";
                break;
            }
            case MessageType::kCoveredAcquireWritelockResponse:
            {
                message_type_str = "kCoveredAcquireWritelockResponse";
                break;
            }
            case MessageType::kCoveredReleaseWritelockRequest:
            {
                message_type_str = "kCoveredReleaseWritelockRequest";
                break;
            }
            case MessageType::kCoveredReleaseWritelockResponse:
            {
                message_type_str = "kCoveredReleaseWritelockResponse";
                break;
            }
            case MessageType::kCoveredRedirectedGetRequest:
            {
                message_type_str = "kCoveredRedirectedGetRequest";
                break;
            }
            case MessageType::kCoveredRedirectedGetResponse:
            {
                message_type_str = "kCoveredRedirectedGetResponse";
                break;
            }
            case MessageType::kCoveredBgfetchRedirectedGetRequest:
            {
                message_type_str = "kCoveredBgfetchRedirectedGetRequest";
                break;
            }
            case MessageType::kCoveredBgfetchRedirectedGetResponse:
            {
                message_type_str = "kCoveredBgfetchRedirectedGetResponse";
                break;
            }
            case MessageType::kCoveredBgfetchGlobalGetRequest:
            {
                message_type_str = "kCoveredBgfetchGlobalGetRequest";
                break;
            }
            case MessageType::kCoveredBgfetchGlobalGetResponse:
            {
                message_type_str = "kCoveredBgfetchGlobalGetResponse";
                break;
            }
            case MessageType::kCoveredBgplacePlacementNotifyRequest:
            {
                message_type_str = "kCoveredBgplacePlacementNotifyRequest";
                break;
            }
            case MessageType::kCoveredBgplaceDirectoryUpdateRequest:
            {
                message_type_str = "kCoveredBgplaceDirectoryUpdateRequest";
                break;
            }
            case MessageType::kCoveredBgplaceDirectoryUpdateResponse:
            {
                message_type_str = "kCoveredBgplaceDirectoryUpdateResponse";
                break;
            }
            case MessageType::kCoveredVictimFetchRequest:
            {
                message_type_str = "kCoveredVictimFetchRequest";
                break;
            }
            case MessageType::kCoveredVictimFetchResponse:
            {
                message_type_str = "kCoveredVictimFetchResponse";
                break;
            }
            case MessageType::kCoveredFghybridDirectoryLookupResponse:
            {
                message_type_str = "kCoveredFghybridDirectoryLookupResponse";
                break;
            }
            case MessageType::kCoveredFghybridDirectoryEvictResponse:
            {
                message_type_str = "kCoveredFghybridDirectoryEvictResponse";
                break;
            }
            case MessageType::kCoveredFghybridReleaseWritelockResponse:
            {
                message_type_str = "kCoveredFghybridReleaseWritelockResponse";
                break;
            }
            case MessageType::kCoveredFghybridHybridFetchedRequest:
            {
                message_type_str = "kCoveredFghybridHybridFetchedRequest";
                break;
            }
            case MessageType::kCoveredFghybridHybridFetchedResponse:
            {
                message_type_str = "kCoveredFghybridHybridFetchedResponse";
                break;
            }
            case MessageType::kCoveredFghybridDirectoryAdmitRequest:
            {
                message_type_str = "kCoveredFghybridDirectoryAdmitRequest";
                break;
            }
            case MessageType::kCoveredFghybridDirectoryAdmitResponse:
            {
                message_type_str = "kCoveredFghybridDirectoryAdmitResponse";
                break;
            }
            case MessageType::kCoveredInvalidationRequest:
            {
                message_type_str = "kCoveredInvalidationRequest";
                break;
            }
            case MessageType::kCoveredInvalidationResponse:
            {
                message_type_str = "kCoveredInvalidationResponse";
                break;
            }
            case MessageType::kCoveredFinishBlockRequest:
            {
                message_type_str = "kCoveredFinishBlockRequest";
                break;
            }
            case MessageType::kCoveredFinishBlockResponse:
            {
                message_type_str = "kCoveredFinishBlockResponse";
                break;
            }
            case MessageType::kCoveredFastpathDirectoryLookupResponse:
            {
                message_type_str = "kCoveredFastpathDirectoryLookupResponse";
                break;
            }
            case MessageType::kCoveredMetadataUpdateRequest:
            {
                message_type_str = "kCoveredMetadataUpdateRequest";
                break;
            }
            case MessageType::kCoveredPlacementTriggerRequest:
            {
                message_type_str = "kCoveredPlacementTriggerRequest";
                break;
            }
            case MessageType::kCoveredPlacementTriggerResponse:
            {
                message_type_str = "kCoveredPlacementTriggerResponse";
                break;
            }
            case MessageType::kCoveredFghybridPlacementTriggerResponse:
            {
                message_type_str = "kCoveredFghybridPlacementTriggerResponse";
                break;
            }
            case MessageType::kBestGuessPlacementTriggerRequest:
            {
                message_type_str = "kBestGuessPlacementTriggerRequest";
                break;
            }
            case MessageType::kBestGuessPlacementTriggerResponse:
            {
                message_type_str = "kBestGuessPlacementTriggerResponse";
                break;
            }
            case MessageType::kBestGuessDirectoryUpdateRequest:
            {
                message_type_str = "kBestGuessDirectoryUpdateRequest";
                break;
            }
            case MessageType::kBestGuessDirectoryUpdateResponse:
            {
                message_type_str = "kBestGuessDirectoryUpdateResponse";
                break;
            }
            case MessageType::kBestGuessBgplaceDirectoryUpdateRequest:
            {
                message_type_str = "kBestGuessBgplaceDirectoryUpdateRequest";
                break;
            }
            case MessageType::kBestGuessBgplaceDirectoryUpdateResponse:
            {
                message_type_str = "kBestGuessBgplaceDirectoryUpdateResponse";
                break;
            }
            case MessageType::kBestGuessBgplacePlacementNotifyRequest:
            {
                message_type_str = "kBestGuessBgplacePlacementNotifyRequest";
                break;
            }
            case MessageType::kBestGuessDirectoryLookupRequest:
            {
                message_type_str = "kBestGuessDirectoryLookupRequest";
                break;
            }
            case MessageType::kBestGuessDirectoryLookupResponse:
            {
                message_type_str = "kBestGuessDirectoryLookupResponse";
                break;
            }
            case MessageType::kBestGuessFinishBlockRequest:
            {
                message_type_str = "kBestGuessFinishBlockRequest";
                break;
            }
            case MessageType::kBestGuessFinishBlockResponse:
            {
                message_type_str = "kBestGuessFinishBlockResponse";
                break;
            }
            case MessageType::kBestGuessAcquireWritelockRequest:
            {
                message_type_str = "kBestGuessAcquireWritelockRequest";
                break;
            }
            case MessageType::kBestGuessAcquireWritelockResponse:
            {
                message_type_str = "kBestGuessAcquireWritelockResponse";
                break;
            }
            case MessageType::kBestGuessInvalidationRequest:
            {
                message_type_str = "kBestGuessInvalidationRequest";
                break;
            }
            case MessageType::kBestGuessInvalidationResponse:
            {
                message_type_str = "kBestGuessInvalidationResponse";
                break;
            }
            case MessageType::kBestGuessReleaseWritelockRequest:
            {
                message_type_str = "kBestGuessReleaseWritelockRequest";
                break;
            }
            case MessageType::kBestGuessReleaseWritelockResponse:
            {
                message_type_str = "kBestGuessReleaseWritelockResponse";
                break;
            }
            case MessageType::kBestGuessRedirectedGetRequest:
            {
                message_type_str = "kBestGuessRedirectedGetRequest";
                break;
            }
            case MessageType::kBestGuessRedirectedGetResponse:
            {
                message_type_str = "kBestGuessRedirectedGetResponse";
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
            case Hitflag::kCooperativeInvalid:
            {
                hitflag_str = "kCooperativeInvalid";
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

    std::string MessageBase::lockResultToString(const LockResult& lock_result)
    {
        std::string lockresult_str = "";
        switch (lock_result)
        {
            case LockResult::kSuccess:
            {
                lockresult_str = "kSuccess";
                break;
            }
            case LockResult::kFailure:
            {
                lockresult_str = "kFailure";
                break;
            }
            case LockResult::kNoneed:
            {
                lockresult_str = "kNoneed";
                break;
            }
            default:
            {
                lockresult_str = std::to_string(static_cast<uint8_t>(lock_result));
                break;
            }
        }
        return lockresult_str;
    }

    MessageBase* MessageBase::getRequestFromWorkloadItem(WorkloadItem workload_item, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& is_warmup_phase, const bool& is_warmup_speedup, const bool& is_monitored, const uint64_t& cur_msg_seqnum)
    {
        assert(source_addr.isValidAddr());
        
        WorkloadItemType item_type = workload_item.getItemType();

        bool skip_propagation_latency = false;
        if (is_warmup_phase && is_warmup_speedup)
        {
            skip_propagation_latency = true;
        }
        
        ExtraCommonMsghdr extra_common_msghdr(skip_propagation_latency, is_monitored, cur_msg_seqnum); // NOTE: the msg seqnum is assiend by client

        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (item_type)
        {
            case WorkloadItemType::kWorkloadItemGet:
            {
                message_ptr = new LocalGetRequest(workload_item.getKey(), source_index, source_addr, extra_common_msghdr);
                break;
            }
            case WorkloadItemType::kWorkloadItemPut:
            {
                message_ptr = new LocalPutRequest(workload_item.getKey(), workload_item.getValue(), source_index, source_addr, extra_common_msghdr);
                break;
            }
            case WorkloadItemType::kWorkloadItemDel:
            {
                message_ptr = new LocalDelRequest(workload_item.getKey(), source_index, source_addr, extra_common_msghdr);
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid workload item type " << WorkloadItem::workloadItemTypeToString(item_type) << " for getRequestFromWorkloadItem()!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        assert(message_ptr->isLocalDataRequest()); // Must be local request for clients
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
        bool special_case = false;
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
            case MessageType::kInitializationRequest:
            {
                message_ptr = new InitializationRequest(msg_payload);
                break;
            }
            case MessageType::kStartrunRequest:
            {
                message_ptr = new StartrunRequest(msg_payload);
                break;
            }
            case MessageType::kSwitchSlotRequest:
            {
                message_ptr = new SwitchSlotRequest(msg_payload);
                break;
            }
            case MessageType::kFinishWarmupRequest:
            {
                message_ptr = new FinishWarmupRequest(msg_payload);
                break;
            }
            case MessageType::kFinishrunRequest:
            {
                message_ptr = new FinishrunRequest(msg_payload);
                break;
            }
            case MessageType::kDumpSnapshotRequest:
            {
                message_ptr = new DumpSnapshotRequest(msg_payload);
                break;
            }
            case MessageType::kAcquireWritelockRequest:
            {
                message_ptr = new AcquireWritelockRequest(msg_payload);
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
            case MessageType::kFinishBlockRequest:
            {
                message_ptr = new FinishBlockRequest(msg_payload);
                break;
            }
            case MessageType::kInvalidationRequest:
            {
                message_ptr = new InvalidationRequest(msg_payload);
                break;
            }
            case MessageType::kReleaseWritelockRequest:
            {
                message_ptr = new ReleaseWritelockRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredDirectoryLookupRequest:
            {
                message_ptr = new CoveredDirectoryLookupRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredDirectoryUpdateRequest:
            {
                message_ptr = new CoveredDirectoryUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredAcquireWritelockRequest:
            {
                message_ptr = new CoveredAcquireWritelockRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredReleaseWritelockRequest:
            {
                message_ptr = new CoveredReleaseWritelockRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredRedirectedGetRequest:
            {
                message_ptr = new CoveredRedirectedGetRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredBgfetchRedirectedGetRequest:
            {
                message_ptr = new CoveredBgfetchRedirectedGetRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredBgfetchRedirectedGetResponse: // NOTE: this is a special case for COVERED to process directed get response by beacon server recvreq port for non-blocking placement deployment
            {
                message_ptr = new CoveredBgfetchRedirectedGetResponse(msg_payload);
                special_case = true;
                break;
            }
            case MessageType::kCoveredBgfetchGlobalGetRequest:
            {
                message_ptr = new CoveredBgfetchGlobalGetRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredBgfetchGlobalGetResponse: // NOTE: this is a special case for COVERED to process global get response by beacon server recvreq port for non-blocking placement deployment
            {
                message_ptr = new CoveredBgfetchGlobalGetResponse(msg_payload);
                special_case = true;
                break;
            }
            case MessageType::kCoveredBgplacePlacementNotifyRequest:
            {
                message_ptr = new CoveredBgplacePlacementNotifyRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredBgplaceDirectoryUpdateRequest:
            {
                message_ptr = new CoveredBgplaceDirectoryUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredVictimFetchRequest:
            {
                message_ptr = new CoveredVictimFetchRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridHybridFetchedRequest:
            {
                message_ptr = new CoveredFghybridHybridFetchedRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridDirectoryAdmitRequest:
            {
                message_ptr = new CoveredFghybridDirectoryAdmitRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredInvalidationRequest:
            {
                message_ptr = new CoveredInvalidationRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredFinishBlockRequest:
            {
                message_ptr = new CoveredFinishBlockRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredMetadataUpdateRequest:
            {
                message_ptr = new CoveredMetadataUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementTriggerRequest:
            {
                message_ptr = new CoveredPlacementTriggerRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessPlacementTriggerRequest:
            {
                message_ptr = new BestGuessPlacementTriggerRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessDirectoryUpdateRequest:
            {
                message_ptr = new BestGuessDirectoryUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessBgplaceDirectoryUpdateRequest:
            {
                message_ptr = new BestGuessBgplaceDirectoryUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessBgplacePlacementNotifyRequest:
            {
                message_ptr = new BestGuessBgplacePlacementNotifyRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessDirectoryLookupRequest:
            {
                message_ptr = new BestGuessDirectoryLookupRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessFinishBlockRequest:
            {
                message_ptr = new BestGuessFinishBlockRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessAcquireWritelockRequest:
            {
                message_ptr = new BestGuessAcquireWritelockRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessInvalidationRequest:
            {
                message_ptr = new BestGuessInvalidationRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessReleaseWritelockRequest:
            {
                message_ptr = new BestGuessReleaseWritelockRequest(msg_payload);
                break;
            }
            case MessageType::kBestGuessRedirectedGetRequest:
            {
                message_ptr = new BestGuessRedirectedGetRequest(msg_payload);
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
        if (!special_case)
        {
            assert(message_ptr->isDataRequest() || message_ptr->isControlRequest());
        }
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
            case MessageType::kGlobalDelResponse:
            {
                message_ptr = new GlobalDelResponse(msg_payload);
                break;
            }
            case MessageType::kRedirectedGetResponse:
            {
                message_ptr = new RedirectedGetResponse(msg_payload);
                break;
            }
            case MessageType::kInitializationResponse:
            {
                message_ptr = new InitializationResponse(msg_payload);
                break;
            }
            case MessageType::kStartrunResponse:
            {
                message_ptr = new StartrunResponse(msg_payload);
                break;
            }
            case MessageType::kSwitchSlotResponse:
            {
                message_ptr = new SwitchSlotResponse(msg_payload);
                break;
            }
            case MessageType::kFinishWarmupResponse:
            {
                message_ptr = new FinishWarmupResponse(msg_payload);
                break;
            }
            case MessageType::kFinishrunResponse:
            {
                message_ptr = new FinishrunResponse(msg_payload);
                break;
            }
            case MessageType::kDumpSnapshotResponse:
            {
                message_ptr = new DumpSnapshotResponse(msg_payload);
                break;
            }
            case MessageType::kSimpleFinishrunResponse:
            {
                message_ptr = new SimpleFinishrunResponse(msg_payload);
                break;
            }
            case MessageType::kAcquireWritelockResponse:
            {
                message_ptr = new AcquireWritelockResponse(msg_payload);
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
            case MessageType::kFinishBlockResponse:
            {
                message_ptr = new FinishBlockResponse(msg_payload);
                break;
            }
            case MessageType::kInvalidationResponse:
            {
                message_ptr = new InvalidationResponse(msg_payload);
                break;
            }
            case MessageType::kReleaseWritelockResponse:
            {
                message_ptr = new ReleaseWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredDirectoryLookupResponse:
            {
                message_ptr = new CoveredDirectoryLookupResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredDirectoryUpdateResponse:
            {
                message_ptr = new CoveredDirectoryUpdateResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredAcquireWritelockResponse:
            {
                message_ptr = new CoveredAcquireWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredReleaseWritelockResponse:
            {
                message_ptr = new CoveredReleaseWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredRedirectedGetResponse:
            {
                message_ptr = new CoveredRedirectedGetResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredBgfetchRedirectedGetResponse:
            {
                message_ptr = new CoveredBgfetchRedirectedGetResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredBgfetchGlobalGetResponse:
            {
                message_ptr = new CoveredBgfetchGlobalGetResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredBgplaceDirectoryUpdateResponse:
            {
                message_ptr = new CoveredBgplaceDirectoryUpdateResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredVictimFetchResponse:
            {
                message_ptr = new CoveredVictimFetchResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridDirectoryLookupResponse:
            {
                message_ptr = new CoveredFghybridDirectoryLookupResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridDirectoryEvictResponse:
            {
                message_ptr = new CoveredFghybridDirectoryEvictResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridReleaseWritelockResponse:
            {
                message_ptr = new CoveredFghybridReleaseWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridHybridFetchedResponse:
            {
                message_ptr = new CoveredFghybridHybridFetchedResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridDirectoryAdmitResponse:
            {
                message_ptr = new CoveredFghybridDirectoryAdmitResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredInvalidationResponse:
            {
                message_ptr = new CoveredInvalidationResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFinishBlockResponse:
            {
                message_ptr = new CoveredFinishBlockResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFastpathDirectoryLookupResponse:
            {
                message_ptr = new CoveredFastpathDirectoryLookupResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementTriggerResponse:
            {
                message_ptr = new CoveredPlacementTriggerResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredFghybridPlacementTriggerResponse:
            {
                message_ptr = new CoveredFghybridPlacementTriggerResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessPlacementTriggerResponse:
            {
                message_ptr = new BestGuessPlacementTriggerResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessDirectoryUpdateResponse:
            {
                message_ptr = new BestGuessDirectoryUpdateResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessBgplaceDirectoryUpdateResponse:
            {
                message_ptr = new BestGuessBgplaceDirectoryUpdateResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessDirectoryLookupResponse:
            {
                message_ptr = new BestGuessDirectoryLookupResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessFinishBlockResponse:
            {
                message_ptr = new BestGuessFinishBlockResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessAcquireWritelockResponse:
            {
                message_ptr = new BestGuessAcquireWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessInvalidationResponse:
            {
                message_ptr = new BestGuessInvalidationResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessReleaseWritelockResponse:
            {
                message_ptr = new BestGuessReleaseWritelockResponse(msg_payload);
                break;
            }
            case MessageType::kBestGuessRedirectedGetResponse:
            {
                message_ptr = new BestGuessRedirectedGetResponse(msg_payload);
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
        else if (message_ptr->message_type_ == MessageType::kLocalGetResponse)
        {
            const LocalGetResponse* const local_get_response_ptr = static_cast<const LocalGetResponse*>(message_ptr);
            tmp_key = local_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kLocalPutResponse)
        {
            const LocalPutResponse* const local_put_response_ptr = static_cast<const LocalPutResponse*>(message_ptr);
            tmp_key = local_put_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kLocalDelResponse)
        {
            const LocalDelResponse* const local_del_response_ptr = static_cast<const LocalDelResponse*>(message_ptr);
            tmp_key = local_del_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalGetRequest)
        {
            const GlobalGetRequest* const global_get_request_ptr = static_cast<const GlobalGetRequest*>(message_ptr);
            tmp_key = global_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalPutRequest)
        {
            const GlobalPutRequest* const global_put_request_ptr = static_cast<const GlobalPutRequest*>(message_ptr);
            tmp_key = global_put_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalDelRequest)
        {
            const GlobalDelRequest* const global_del_request_ptr = static_cast<const GlobalDelRequest*>(message_ptr);
            tmp_key = global_del_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalGetResponse)
        {
            const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(message_ptr);
            tmp_key = global_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalPutResponse)
        {
            const GlobalPutResponse* const global_put_response_ptr = static_cast<const GlobalPutResponse*>(message_ptr);
            tmp_key = global_put_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kGlobalDelResponse)
        {
            const GlobalDelResponse* const global_del_response_ptr = static_cast<const GlobalDelResponse*>(message_ptr);
            tmp_key = global_del_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kRedirectedGetRequest)
        {
            const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(message_ptr);
            tmp_key = redirected_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kRedirectedGetResponse)
        {
            const RedirectedGetResponse* const redirected_get_response_ptr = static_cast<const RedirectedGetResponse*>(message_ptr);
            tmp_key = redirected_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kAcquireWritelockRequest)
        {
            const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(message_ptr);
            tmp_key = acquire_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kAcquireWritelockResponse)
        {
            const AcquireWritelockResponse* const acquire_writelock_response_ptr = static_cast<const AcquireWritelockResponse*>(message_ptr);
            tmp_key = acquire_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kDirectoryLookupRequest)
        {
            const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(message_ptr);
            tmp_key = directory_lookup_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kDirectoryUpdateRequest)
        {
            const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(message_ptr);
            tmp_key = directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kDirectoryLookupResponse)
        {
            const DirectoryLookupResponse* const directory_lookup_response_ptr = static_cast<const DirectoryLookupResponse*>(message_ptr);
            tmp_key = directory_lookup_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kDirectoryUpdateResponse)
        {
            const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(message_ptr);
            tmp_key = directory_update_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kFinishBlockRequest)
        {
            const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(message_ptr);
            tmp_key = finish_block_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kFinishBlockResponse)
        {
            const FinishBlockResponse* const finish_block_response_ptr = static_cast<const FinishBlockResponse*>(message_ptr);
            tmp_key = finish_block_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kInvalidationRequest)
        {
            const InvalidationRequest* const invalidation_request_ptr = static_cast<const InvalidationRequest*>(message_ptr);
            tmp_key = invalidation_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kInvalidationResponse)
        {
            const InvalidationResponse* const invalidation_response_ptr = static_cast<const InvalidationResponse*>(message_ptr);
            tmp_key = invalidation_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kReleaseWritelockRequest)
        {
            const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(message_ptr);
            tmp_key = release_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kReleaseWritelockResponse)
        {
            const ReleaseWritelockResponse* const release_writelock_response_ptr = static_cast<const ReleaseWritelockResponse*>(message_ptr);
            tmp_key = release_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredDirectoryLookupRequest)
        {
            const CoveredDirectoryLookupRequest* const covered_directory_lookup_request_ptr = static_cast<const CoveredDirectoryLookupRequest*>(message_ptr);
            tmp_key = covered_directory_lookup_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredDirectoryLookupResponse)
        {
            const CoveredDirectoryLookupResponse* const covered_directory_lookup_response_ptr = static_cast<const CoveredDirectoryLookupResponse*>(message_ptr);
            tmp_key = covered_directory_lookup_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredDirectoryUpdateRequest)
        {
            const CoveredDirectoryUpdateRequest* const covered_directory_update_request_ptr = static_cast<const CoveredDirectoryUpdateRequest*>(message_ptr);
            tmp_key = covered_directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredDirectoryUpdateResponse)
        {
            const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(message_ptr);
            tmp_key = covered_directory_update_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredAcquireWritelockRequest)
        {
            const CoveredAcquireWritelockRequest* const covered_acquire_writelock_request_ptr = static_cast<const CoveredAcquireWritelockRequest*>(message_ptr);
            tmp_key = covered_acquire_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredAcquireWritelockResponse)
        {
            const CoveredAcquireWritelockResponse* const covered_acquire_writelock_response_ptr = static_cast<const CoveredAcquireWritelockResponse*>(message_ptr);
            tmp_key = covered_acquire_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredReleaseWritelockRequest)
        {
            const CoveredReleaseWritelockRequest* const covered_release_writelock_request_ptr = static_cast<const CoveredReleaseWritelockRequest*>(message_ptr);
            tmp_key = covered_release_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredReleaseWritelockResponse)
        {
            const CoveredReleaseWritelockResponse* const covered_release_writelock_response_ptr = static_cast<const CoveredReleaseWritelockResponse*>(message_ptr);
            tmp_key = covered_release_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredRedirectedGetRequest)
        {
            const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(message_ptr);
            tmp_key = covered_redirected_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredRedirectedGetResponse)
        {
            const CoveredRedirectedGetResponse* const covered_redirected_get_response_ptr = static_cast<const CoveredRedirectedGetResponse*>(message_ptr);
            tmp_key = covered_redirected_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgfetchRedirectedGetRequest)
        {
            const CoveredBgfetchRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredBgfetchRedirectedGetRequest*>(message_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgfetchRedirectedGetResponse)
        {
            const CoveredBgfetchRedirectedGetResponse* const covered_placement_redirected_get_response_ptr = static_cast<const CoveredBgfetchRedirectedGetResponse*>(message_ptr);
            tmp_key = covered_placement_redirected_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgfetchGlobalGetRequest)
        {
            const CoveredBgfetchGlobalGetRequest* const covered_placement_global_get_request_ptr = static_cast<const CoveredBgfetchGlobalGetRequest*>(message_ptr);
            tmp_key = covered_placement_global_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgfetchGlobalGetResponse)
        {
            const CoveredBgfetchGlobalGetResponse* const covered_placement_global_get_response_ptr = static_cast<const CoveredBgfetchGlobalGetResponse*>(message_ptr);
            tmp_key = covered_placement_global_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgplacePlacementNotifyRequest)
        {
            const CoveredBgplacePlacementNotifyRequest* const covered_placement_notify_request_ptr = static_cast<const CoveredBgplacePlacementNotifyRequest*>(message_ptr);
            tmp_key = covered_placement_notify_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateRequest)
        {
            const CoveredBgplaceDirectoryUpdateRequest* const covered_placement_directory_update_request_ptr = static_cast<const CoveredBgplaceDirectoryUpdateRequest*>(message_ptr);
            tmp_key = covered_placement_directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateResponse)
        {
            const CoveredBgplaceDirectoryUpdateResponse* const covered_placement_directory_update_response_ptr = static_cast<const CoveredBgplaceDirectoryUpdateResponse*>(message_ptr);
            tmp_key = covered_placement_directory_update_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridDirectoryLookupResponse)
        {
            const CoveredFghybridDirectoryLookupResponse* const covered_placement_directory_lookup_response_ptr = static_cast<const CoveredFghybridDirectoryLookupResponse*>(message_ptr);
            tmp_key = covered_placement_directory_lookup_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridDirectoryEvictResponse)
        {
            const CoveredFghybridDirectoryEvictResponse* const covered_placement_directory_evict_response_ptr = static_cast<const CoveredFghybridDirectoryEvictResponse*>(message_ptr);
            tmp_key = covered_placement_directory_evict_response_ptr->getKey();
        
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridReleaseWritelockResponse)
        {
            const CoveredFghybridReleaseWritelockResponse* const covered_placement_release_writelock_response_ptr = static_cast<const CoveredFghybridReleaseWritelockResponse*>(message_ptr);
            tmp_key = covered_placement_release_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridHybridFetchedRequest)
        {
            const CoveredFghybridHybridFetchedRequest* const covered_placement_hybrid_fetched_request_ptr = static_cast<const CoveredFghybridHybridFetchedRequest*>(message_ptr);
            tmp_key = covered_placement_hybrid_fetched_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridHybridFetchedResponse)
        {
            const CoveredFghybridHybridFetchedResponse* const covered_placement_hybrid_fetched_response_ptr = static_cast<const CoveredFghybridHybridFetchedResponse*>(message_ptr);
            tmp_key = covered_placement_hybrid_fetched_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridDirectoryAdmitRequest)
        {
            const CoveredFghybridDirectoryAdmitRequest* const covered_placement_directory_admit_request_ptr = static_cast<const CoveredFghybridDirectoryAdmitRequest*>(message_ptr);
            tmp_key = covered_placement_directory_admit_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridDirectoryAdmitResponse)
        {
            const CoveredFghybridDirectoryAdmitResponse* const covered_placement_directory_admit_response_ptr = static_cast<const CoveredFghybridDirectoryAdmitResponse*>(message_ptr);
            tmp_key = covered_placement_directory_admit_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredInvalidationRequest)
        {
            const CoveredInvalidationRequest* const covered_invalidation_request_ptr = static_cast<const CoveredInvalidationRequest*>(message_ptr);
            tmp_key = covered_invalidation_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredInvalidationResponse)
        {
            const CoveredInvalidationResponse* const covered_invalidation_response_ptr = static_cast<const CoveredInvalidationResponse*>(message_ptr);
            tmp_key = covered_invalidation_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFinishBlockRequest)
        {
            const CoveredFinishBlockRequest* const covered_finish_block_request_ptr = static_cast<const CoveredFinishBlockRequest*>(message_ptr);
            tmp_key = covered_finish_block_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFinishBlockResponse)
        {
            const CoveredFinishBlockResponse* const covered_finish_block_response_ptr = static_cast<const CoveredFinishBlockResponse*>(message_ptr);
            tmp_key = covered_finish_block_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFastpathDirectoryLookupResponse)
        {
            const CoveredFastpathDirectoryLookupResponse* const covered_fast_directory_lookup_response_ptr = static_cast<const CoveredFastpathDirectoryLookupResponse*>(message_ptr);
            tmp_key = covered_fast_directory_lookup_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredMetadataUpdateRequest)
        {
            const CoveredMetadataUpdateRequest* const covered_metadata_update_request_ptr = static_cast<const CoveredMetadataUpdateRequest*>(message_ptr);
            tmp_key = covered_metadata_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementTriggerRequest)
        {
            const CoveredPlacementTriggerRequest* const covered_placement_trigger_request_ptr = static_cast<const CoveredPlacementTriggerRequest*>(message_ptr);
            tmp_key = covered_placement_trigger_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementTriggerResponse)
        {
            const CoveredPlacementTriggerResponse* const covered_placement_trigger_response_ptr = static_cast<const CoveredPlacementTriggerResponse*>(message_ptr);
            tmp_key = covered_placement_trigger_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredFghybridPlacementTriggerResponse)
        {
            const CoveredFghybridPlacementTriggerResponse* const covered_placement_trigger_response_ptr = static_cast<const CoveredFghybridPlacementTriggerResponse*>(message_ptr);
            tmp_key = covered_placement_trigger_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessPlacementTriggerRequest)
        {
            const BestGuessPlacementTriggerRequest* const best_guess_placement_trigger_request_ptr = static_cast<const BestGuessPlacementTriggerRequest*>(message_ptr);
            tmp_key = best_guess_placement_trigger_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessPlacementTriggerResponse)
        {
            const BestGuessPlacementTriggerResponse* const best_guess_placement_trigger_response_ptr = static_cast<const BestGuessPlacementTriggerResponse*>(message_ptr);
            tmp_key = best_guess_placement_trigger_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessDirectoryUpdateRequest)
        {
            const BestGuessDirectoryUpdateRequest* const best_guess_directory_update_request_ptr = static_cast<const BestGuessDirectoryUpdateRequest*>(message_ptr);
            tmp_key = best_guess_directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessDirectoryUpdateResponse)
        {
            const BestGuessDirectoryUpdateResponse* const best_guess_directory_update_response_ptr = static_cast<const BestGuessDirectoryUpdateResponse*>(message_ptr);
            tmp_key = best_guess_directory_update_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateRequest)
        {
            const BestGuessBgplaceDirectoryUpdateRequest* const best_guess_bgplace_directory_update_request_ptr = static_cast<const BestGuessBgplaceDirectoryUpdateRequest*>(message_ptr);
            tmp_key = best_guess_bgplace_directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateResponse)
        {
            const BestGuessBgplaceDirectoryUpdateResponse* const best_guess_bgplace_directory_update_response_ptr = static_cast<const BestGuessBgplaceDirectoryUpdateResponse*>(message_ptr);
            tmp_key = best_guess_bgplace_directory_update_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessBgplacePlacementNotifyRequest)
        {
            const BestGuessBgplacePlacementNotifyRequest* const best_guess_bgplace_placement_notify_request_ptr = static_cast<const BestGuessBgplacePlacementNotifyRequest*>(message_ptr);
            tmp_key = best_guess_bgplace_placement_notify_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessDirectoryLookupRequest)
        {
            const BestGuessDirectoryLookupRequest* const best_guess_directory_lookup_request_ptr = static_cast<const BestGuessDirectoryLookupRequest*>(message_ptr);
            tmp_key = best_guess_directory_lookup_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessDirectoryLookupResponse)
        {
            const BestGuessDirectoryLookupResponse* const best_guess_directory_lookup_response_ptr = static_cast<const BestGuessDirectoryLookupResponse*>(message_ptr);
            tmp_key = best_guess_directory_lookup_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessFinishBlockRequest)
        {
            const BestGuessFinishBlockRequest* const best_guess_finish_block_request_ptr = static_cast<const BestGuessFinishBlockRequest*>(message_ptr);
            tmp_key = best_guess_finish_block_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessFinishBlockResponse)
        {
            const BestGuessFinishBlockResponse* const best_guess_finish_block_response_ptr = static_cast<const BestGuessFinishBlockResponse*>(message_ptr);
            tmp_key = best_guess_finish_block_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessAcquireWritelockRequest)
        {
            const BestGuessAcquireWritelockRequest* const best_guess_acquire_writelock_request_ptr = static_cast<const BestGuessAcquireWritelockRequest*>(message_ptr);
            tmp_key = best_guess_acquire_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessAcquireWritelockResponse)
        {
            const BestGuessAcquireWritelockResponse* const best_guess_acquire_writelock_response_ptr = static_cast<const BestGuessAcquireWritelockResponse*>(message_ptr);
            tmp_key = best_guess_acquire_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessInvalidationRequest)
        {
            const BestGuessInvalidationRequest* const best_guess_invalidation_request_ptr = static_cast<const BestGuessInvalidationRequest*>(message_ptr);
            tmp_key = best_guess_invalidation_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessInvalidationResponse)
        {
            const BestGuessInvalidationResponse* const best_guess_invalidation_response_ptr = static_cast<const BestGuessInvalidationResponse*>(message_ptr);
            tmp_key = best_guess_invalidation_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessReleaseWritelockRequest)
        {
            const BestGuessReleaseWritelockRequest* const best_guess_release_writelock_request_ptr = static_cast<const BestGuessReleaseWritelockRequest*>(message_ptr);
            tmp_key = best_guess_release_writelock_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessReleaseWritelockResponse)
        {
            const BestGuessReleaseWritelockResponse* const best_guess_release_writelock_response_ptr = static_cast<const BestGuessReleaseWritelockResponse*>(message_ptr);
            tmp_key = best_guess_release_writelock_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessRedirectedGetRequest)
        {
            const BestGuessRedirectedGetRequest* const best_guess_redirected_get_request_ptr = static_cast<const BestGuessRedirectedGetRequest*>(message_ptr);
            tmp_key = best_guess_redirected_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kBestGuessRedirectedGetResponse)
        {
            const BestGuessRedirectedGetResponse* const best_guess_redirected_get_response_ptr = static_cast<const BestGuessRedirectedGetResponse*>(message_ptr);
            tmp_key = best_guess_redirected_get_response_ptr->getKey();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << messageTypeToString(message_ptr->message_type_) << " for getKeyFromMessage()!";
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

    MessageBase::MessageBase(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr)
    {
        message_type_ = message_type;

        source_index_ = source_index;
        source_addr_ = source_addr;
        bandwidth_usage_ = bandwidth_usage;
        event_list_ = event_list;
        extra_common_msghdr_ = extra_common_msghdr;

        is_valid_ = true;
        is_response_ = isDataResponse() || isControlResponse(); // NOTE: isDataResponse() and isControlResponse() require is_valid_ = true
    }

    // NOTE: CANNOT call pure method in constructor or destructor, as the derived object has not been contructed or has already been destructed
    //MessageBase::MessageBase(const DynamicArray& msg_payload)
    //{
    //    deserialize(msg_payload);
    //}

    MessageBase::MessageBase()
    {
        is_valid_ = false;
        is_response_ = false;
    }
    
    MessageBase::~MessageBase() {}

    MessageType MessageBase::getMessageType() const
    {
        checkIsValid_();
        return message_type_;
    }

    uint32_t MessageBase::getSourceIndex() const
    {
        checkIsValid_();
        return source_index_;
    }

    NetworkAddr MessageBase::getSourceAddr() const
    {
        checkIsValid_();

        return source_addr_;
    }

    const BandwidthUsage& MessageBase::getBandwidthUsageRef() const
    {
        checkIsValid_();
        assert(is_response_ == true); // NOTE: ONLY response message has bandwidth usage
        return bandwidth_usage_;
    }

    const EventList& MessageBase::getEventListRef() const
    {
        checkIsValid_();
        assert(is_response_ == true); // NOTE: ONLY response message has event list
        return event_list_;
    }

    ExtraCommonMsghdr MessageBase::getExtraCommonMsghdr() const
    {
        checkIsValid_();
        return extra_common_msghdr_;
    }

    uint32_t MessageBase::getMsgPayloadSize() const
    {
        checkIsValid_();

        uint32_t msg_payload_size = getCommonMsghdrSize_() + getMsgPayloadSizeInternal_();
        
        return msg_payload_size;
    }

    uint32_t MessageBase::getMsgBandwidthSize() const
    {
        checkIsValid_();

        uint32_t msg_bandwidth_size = getCommonMsghdrSize_() + getMsgBandwidthSizeInternal_();
        
        return msg_bandwidth_size;
    }

    uint32_t MessageBase::getCommonMsghdrSize_() const
    {
        uint32_t common_msghdr_size = 0;
        if (is_response_) // NOTE: ONLY response message has bandwidth usage and event list
        {
            // Message type size + source index + source addr + bandwidth usage + event list (0 if without event tracking) + extra common msghdr
            common_msghdr_size = sizeof(uint32_t) + sizeof(uint32_t) + source_addr_.getAddrPayloadSize() + bandwidth_usage_.getBandwidthUsagePayloadSize() + event_list_.getEventListPayloadSize() + extra_common_msghdr_.getExtraCommonMsghdrPayloadSize();
        }
        else // NOTE: request message does NOT have bandwidth usage and event list
        {
            // Message type size + source index + source addr + extra common msghdr
            common_msghdr_size = sizeof(uint32_t) + sizeof(uint32_t) + source_addr_.getAddrPayloadSize() + extra_common_msghdr_.getExtraCommonMsghdrPayloadSize();
        }

        return common_msghdr_size;
    }

    uint32_t MessageBase::getMsgBandwidthSizeInternal_() const
    {
        // NOTE: the same as getMsgPayloadSizeInternal_() by default as NO value content in most messages -> for the messages with value content (restricted by Value::MAX_VALUE_CONTENT_SIZE), the corresponding classes will override this function to calculate msg bandwidth size based on ideal value content size
        return getMsgPayloadSizeInternal_();
    }

    uint32_t MessageBase::serialize(DynamicArray& msg_payload) const
    {
        checkIsValid_();

        uint32_t size = 0;
        uint32_t bigendian_message_type_value = htonl(static_cast<uint32_t>(message_type_));
        msg_payload.deserialize(size, (const char *)&bigendian_message_type_value, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t bigendian_source_index = htonl(static_cast<uint32_t>(source_index_));
        msg_payload.deserialize(size, (const char *)&bigendian_source_index, sizeof(uint32_t));
        size += sizeof(uint32_t);
        uint32_t addr_payload_size = source_addr_.serialize(msg_payload, size);
        size += addr_payload_size;
        if (is_response_) // NOTE: ONLY response message has bandwidth usage and event list
        {
            uint32_t bandwidth_usage_size = bandwidth_usage_.serialize(msg_payload, size);
            size += bandwidth_usage_size;
            uint32_t eventlist_payload_size = event_list_.serialize(msg_payload, size);
            size += eventlist_payload_size;
        }
        uint32_t extra_common_msghdr_size = extra_common_msghdr_.serialize(msg_payload, size);
        size += extra_common_msghdr_size;
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
        is_response_ = isDataResponse() || isControlResponse();
        size += message_type_size;
        msg_payload.serialize(size, (char *)&source_index_, sizeof(uint32_t));
        source_index_ = ntohl(source_index_);
        size += sizeof(uint32_t);
        uint32_t addr_payload_size = source_addr_.deserialize(msg_payload, size);
        size += addr_payload_size;
        if (is_response_) // NOTE: ONLY response message has bandwidth usage and event list
        {
            uint32_t bandwidth_usage_size = bandwidth_usage_.deserialize(msg_payload, size);
            size += bandwidth_usage_size;
            uint32_t eventlist_payload_size = event_list_.deserialize(msg_payload, size);
            size += eventlist_payload_size;
        }
        uint32_t extra_common_msghdr_size = extra_common_msghdr_.deserialize(msg_payload, size);
        size += extra_common_msghdr_size;
        uint32_t internal_size = this->deserializeInternal_(msg_payload, size);
        size += internal_size;

        return size - 0;
    }

    bool MessageBase::isDataRequest() const
    {
        checkIsValid_();
        return isLocalDataRequest() || isRedirectedDataRequest() || isGlobalDataRequest();
    }

    bool MessageBase::isLocalDataRequest() const
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

    bool MessageBase::isRedirectedDataRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kRedirectedGetRequest || message_type_ == MessageType::kCoveredRedirectedGetRequest || message_type_ == MessageType::kCoveredBgfetchRedirectedGetRequest || message_type_ == MessageType::kBestGuessRedirectedGetRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isGlobalDataRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kGlobalGetRequest || message_type_ == MessageType::kGlobalPutRequest || message_type_ == MessageType::kGlobalDelRequest || message_type_ == MessageType::kCoveredBgfetchGlobalGetRequest)
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
        return isLocalDataResponse() || isRedirectedDataResponse() || isGlobalDataResponse();
    }

    bool MessageBase::isLocalDataResponse() const
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

    bool MessageBase::isRedirectedDataResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kRedirectedGetResponse || message_type_ == MessageType::kCoveredRedirectedGetResponse || message_type_ == MessageType::kCoveredBgfetchRedirectedGetResponse || message_type_ == MessageType::kBestGuessRedirectedGetResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isGlobalDataResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kGlobalGetResponse || message_type_ == MessageType::kGlobalPutResponse || message_type_ == MessageType::kGlobalDelResponse || message_type_ == MessageType::kCoveredBgfetchGlobalGetResponse)
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
        return isCooperationControlRequest() || isBenchmarkControlRequest();
    }

    bool MessageBase::isCooperationControlRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kAcquireWritelockRequest || message_type_ == MessageType::kDirectoryLookupRequest || message_type_ == MessageType::kDirectoryUpdateRequest || message_type_ == MessageType::kFinishBlockRequest || message_type_ == MessageType::kInvalidationRequest || message_type_ == MessageType::kReleaseWritelockRequest || message_type_ == MessageType::kCoveredDirectoryLookupRequest || message_type_ == MessageType::kCoveredDirectoryUpdateRequest || message_type_ == MessageType::kCoveredAcquireWritelockRequest || message_type_ == MessageType::kCoveredReleaseWritelockRequest || message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateRequest || message_type_ == MessageType::kCoveredBgplacePlacementNotifyRequest || message_type_ == MessageType::kCoveredVictimFetchRequest || message_type_ == MessageType::kCoveredFghybridHybridFetchedRequest || message_type_ == MessageType::kCoveredFghybridDirectoryAdmitRequest || message_type_ == MessageType::kCoveredInvalidationRequest || message_type_ == MessageType::kCoveredFinishBlockRequest || message_type_ == MessageType::kCoveredMetadataUpdateRequest || message_type_ == MessageType::kBestGuessPlacementTriggerRequest || message_type_ == MessageType::kBestGuessDirectoryUpdateRequest || message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateRequest || message_type_ == MessageType::kBestGuessBgplacePlacementNotifyRequest || message_type_ == MessageType::kBestGuessDirectoryLookupRequest || message_type_ == MessageType::kBestGuessFinishBlockRequest || message_type_ == MessageType::kBestGuessAcquireWritelockRequest || message_type_ == MessageType::kBestGuessInvalidationRequest || message_type_ == MessageType::kBestGuessReleaseWritelockRequest || message_type_ == MessageType::kCoveredPlacementTriggerRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isBenchmarkControlRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kInitializationRequest || message_type_ == MessageType::kStartrunRequest || message_type_ == MessageType::kSwitchSlotRequest || message_type_ == MessageType::kFinishWarmupRequest || message_type_ == MessageType::kFinishrunRequest || message_type_ == MessageType::kDumpSnapshotRequest)
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
        return isCooperationControlResponse() || isBenchmarkControlResponse();
    }

    bool MessageBase::isCooperationControlResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kAcquireWritelockResponse || message_type_ == MessageType::kDirectoryLookupResponse || message_type_ == MessageType::kDirectoryUpdateResponse || message_type_ == MessageType::kFinishBlockResponse || message_type_ == MessageType::kInvalidationResponse || message_type_ == MessageType::kReleaseWritelockResponse || message_type_ == MessageType::kCoveredDirectoryLookupResponse || message_type_ == MessageType::kCoveredDirectoryUpdateResponse || message_type_ == MessageType::kCoveredAcquireWritelockResponse || message_type_ == MessageType::kCoveredReleaseWritelockResponse || message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateResponse || message_type_ == MessageType::kCoveredVictimFetchResponse || message_type_ == MessageType::kCoveredFghybridDirectoryLookupResponse || message_type_ == MessageType::kCoveredFghybridDirectoryEvictResponse || message_type_ == MessageType::kCoveredFghybridReleaseWritelockResponse || message_type_ == MessageType::kCoveredFghybridHybridFetchedResponse || message_type_ == MessageType::kCoveredFghybridDirectoryAdmitResponse || message_type_ == MessageType::kCoveredInvalidationResponse || message_type_ == MessageType::kCoveredFinishBlockResponse || message_type_ == MessageType::kCoveredFastpathDirectoryLookupResponse || message_type_ == MessageType::kBestGuessPlacementTriggerResponse || message_type_ == MessageType::kBestGuessDirectoryUpdateResponse || message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateResponse || message_type_ == MessageType::kBestGuessDirectoryLookupResponse || message_type_ == MessageType::kBestGuessFinishBlockResponse || message_type_ == MessageType::kBestGuessAcquireWritelockResponse || message_type_ == MessageType::kBestGuessInvalidationResponse || message_type_ == MessageType::kBestGuessReleaseWritelockResponse || message_type_ == MessageType::kCoveredPlacementTriggerResponse || message_type_ == MessageType::kCoveredFghybridPlacementTriggerResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isBenchmarkControlResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kInitializationResponse || message_type_ == MessageType::kStartrunResponse || message_type_ == MessageType::kSwitchSlotResponse || message_type_ == MessageType::kFinishWarmupResponse || message_type_ == MessageType::kFinishrunResponse || message_type_ == MessageType::kDumpSnapshotResponse || message_type_ == MessageType::kSimpleFinishrunResponse)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isBackgroundMessage() const
    {
        checkIsValid_();
        return isBackgroundRequest() || isBackgroundResponse();
    }

    bool MessageBase::isBackgroundRequest() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kCoveredBgfetchRedirectedGetRequest || message_type_ == MessageType::kCoveredBgfetchGlobalGetRequest || message_type_ == MessageType::kCoveredBgplacePlacementNotifyRequest || message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateRequest || message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateRequest || message_type_ == MessageType::kBestGuessBgplacePlacementNotifyRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MessageBase::isBackgroundResponse() const
    {
        checkIsValid_();
        if (message_type_ == MessageType::kCoveredBgfetchRedirectedGetResponse || message_type_ == MessageType::kCoveredBgfetchGlobalGetResponse || message_type_ == MessageType::kCoveredBgplaceDirectoryUpdateResponse || message_type_ == MessageType::kBestGuessBgplaceDirectoryUpdateResponse)
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
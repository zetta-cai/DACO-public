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
            case MessageType::kCoveredPlacementRedirectedGetRequest:
            {
                message_type_str = "kCoveredPlacementRedirectedGetRequest";
                break;
            }
            case MessageType::kCoveredPlacementRedirectedGetResponse:
            {
                message_type_str = "kCoveredPlacementRedirectedGetResponse";
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetRequest:
            {
                message_type_str = "kCoveredPlacementGlobalGetRequest";
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetResponse:
            {
                message_type_str = "kCoveredPlacementGlobalGetResponse";
                break;
            }
            case MessageType::kCoveredPlacementNotifyRequest:
            {
                message_type_str = "kCoveredPlacementNotifyRequest";
                break;
            }
            case MessageType::kCoveredPlacementDirectoryUpdateRequest:
            {
                message_type_str = "kCoveredPlacementDirectoryUpdateRequest";
                break;
            }
            case MessageType::kCoveredPlacementDirectoryUpdateResponse:
            {
                message_type_str = "kCoveredPlacementDirectoryUpdateResponse";
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

    MessageBase* MessageBase::getRequestFromWorkloadItem(WorkloadItem workload_item, const uint32_t& source_index, const NetworkAddr& source_addr, const bool& is_warmup_phase, const bool& is_warmup_speedup)
    {
        assert(source_addr.isValidAddr());
        
        WorkloadItemType item_type = workload_item.getItemType();

        bool skip_propagation_latency = false;
        if (is_warmup_phase && is_warmup_speedup)
        {
            skip_propagation_latency = true;
        }

        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (item_type)
        {
            case WorkloadItemType::kWorkloadItemGet:
            {
                message_ptr = new LocalGetRequest(workload_item.getKey(), source_index, source_addr, skip_propagation_latency);
                break;
            }
            case WorkloadItemType::kWorkloadItemPut:
            {
                message_ptr = new LocalPutRequest(workload_item.getKey(), workload_item.getValue(), source_index, source_addr, skip_propagation_latency);
                break;
            }
            case WorkloadItemType::kWorkloadItemDel:
            {
                message_ptr = new LocalDelRequest(workload_item.getKey(), source_index, source_addr, skip_propagation_latency);
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
            case MessageType::kCoveredPlacementRedirectedGetRequest:
            {
                message_ptr = new CoveredPlacementRedirectedGetRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementRedirectedGetResponse: // NOTE: this is a special case for COVERED to process directed get response by beacon server recvreq port for non-blocking placement deployment
            {
                message_ptr = new CoveredPlacementRedirectedGetResponse(msg_payload);
                special_case = true;
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetRequest:
            {
                message_ptr = new CoveredPlacementGlobalGetRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetResponse: // NOTE: this is a special case for COVERED to process global get response by beacon server recvreq port for non-blocking placement deployment
            {
                message_ptr = new CoveredPlacementGlobalGetResponse(msg_payload);
                special_case = true;
                break;
            }
            case MessageType::kCoveredPlacementNotifyRequest:
            {
                message_ptr = new CoveredPlacementNotifyRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementDirectoryUpdateRequest:
            {
                message_ptr = new CoveredPlacementDirectoryUpdateRequest(msg_payload);
                break;
            }
            case MessageType::kCoveredVictimFetchRequest:
            {
                message_ptr = new CoveredVictimFetchRequest(msg_payload);
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
            case MessageType::kCoveredPlacementRedirectedGetResponse:
            {
                message_ptr = new CoveredPlacementRedirectedGetResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetResponse:
            {
                message_ptr = new CoveredPlacementGlobalGetResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredPlacementDirectoryUpdateResponse:
            {
                message_ptr = new CoveredPlacementDirectoryUpdateResponse(msg_payload);
                break;
            }
            case MessageType::kCoveredVictimFetchResponse:
            {
                message_ptr = new CoveredVictimFetchResponse(msg_payload);
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
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(message_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementRedirectedGetResponse)
        {
            const CoveredPlacementRedirectedGetResponse* const covered_placement_redirected_get_response_ptr = static_cast<const CoveredPlacementRedirectedGetResponse*>(message_ptr);
            tmp_key = covered_placement_redirected_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementGlobalGetRequest)
        {
            const CoveredPlacementGlobalGetRequest* const covered_placement_global_get_request_ptr = static_cast<const CoveredPlacementGlobalGetRequest*>(message_ptr);
            tmp_key = covered_placement_global_get_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementGlobalGetResponse)
        {
            const CoveredPlacementGlobalGetResponse* const covered_placement_global_get_response_ptr = static_cast<const CoveredPlacementGlobalGetResponse*>(message_ptr);
            tmp_key = covered_placement_global_get_response_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementNotifyRequest)
        {
            const CoveredPlacementNotifyRequest* const covered_placement_notify_request_ptr = static_cast<const CoveredPlacementNotifyRequest*>(message_ptr);
            tmp_key = covered_placement_notify_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementDirectoryUpdateRequest)
        {
            const CoveredPlacementDirectoryUpdateRequest* const covered_placement_directory_update_request_ptr = static_cast<const CoveredPlacementDirectoryUpdateRequest*>(message_ptr);
            tmp_key = covered_placement_directory_update_request_ptr->getKey();
        }
        else if (message_ptr->message_type_ == MessageType::kCoveredPlacementDirectoryUpdateResponse)
        {
            const CoveredPlacementDirectoryUpdateResponse* const covered_placement_directory_update_response_ptr = static_cast<const CoveredPlacementDirectoryUpdateResponse*>(message_ptr);
            tmp_key = covered_placement_directory_update_response_ptr->getKey();
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

    MessageBase::MessageBase(const MessageType& message_type, const uint32_t& source_index, const NetworkAddr& source_addr, const BandwidthUsage& bandwidth_usage, const EventList& event_list, const bool& skip_propagation_latency)
    {
        message_type_ = message_type;
        is_response_ = isDataResponse() || isControlResponse();

        source_index_ = source_index;
        source_addr_ = source_addr;
        bandwidth_usage_ = bandwidth_usage;
        event_list_ = event_list;
        skip_propagation_latency_ = skip_propagation_latency;

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
        if (Config::isSingleNode())
        {
            // Fix invalid ipstr of localhost under single-node mode
            return NetworkAddr(Util::LOCALHOST_IPSTR, source_addr_.getPort());
        }
        else
        {
            return source_addr_;
        }
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

    bool MessageBase::isSkipPropagationLatency() const
    {
        checkIsValid_();
        return skip_propagation_latency_;
    }

    uint32_t MessageBase::getMsgPayloadSize() const
    {
        checkIsValid_();

        uint32_t msg_payload_size = 0;
        if (is_response_) // NOTE: ONLY response message has bandwidth usage and event list
        {
            // Message type size + source index + source addr + bandwidth usage + event list (0 if without event tracking) + skip_propagation_latency flag + internal payload size
            msg_payload_size = sizeof(uint32_t) + sizeof(uint32_t) + source_addr_.getAddrPayloadSize() + bandwidth_usage_.getBandwidthUsagePayloadSize() + event_list_.getEventListPayloadSize() + sizeof(bool) + getMsgPayloadSizeInternal_();
        }
        else // NOTE: request message does NOT have bandwidth usage and event list
        {
            // Message type size + source index + source addr + skip_propagation_latency flag + internal payload size
            msg_payload_size = sizeof(uint32_t) + sizeof(uint32_t) + source_addr_.getAddrPayloadSize() + sizeof(bool) + getMsgPayloadSizeInternal_();
        }
        
        return msg_payload_size;
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
        msg_payload.deserialize(size, (const char *)&skip_propagation_latency_, sizeof(bool));
        size += sizeof(bool);
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
        msg_payload.serialize(size, (char *)&skip_propagation_latency_, sizeof(bool));
        size += sizeof(bool);
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
        if (message_type_ == MessageType::kRedirectedGetRequest || message_type_ == MessageType::kCoveredRedirectedGetRequest || message_type_ == MessageType::kCoveredPlacementRedirectedGetRequest)
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
        if (message_type_ == MessageType::kGlobalGetRequest || message_type_ == MessageType::kGlobalPutRequest || message_type_ == MessageType::kGlobalDelRequest || message_type_ == MessageType::kCoveredPlacementGlobalGetRequest)
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
        if (message_type_ == MessageType::kRedirectedGetResponse || message_type_ == MessageType::kCoveredRedirectedGetResponse || message_type_ == MessageType::kCoveredPlacementRedirectedGetResponse)
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
        if (message_type_ == MessageType::kGlobalGetResponse || message_type_ == MessageType::kGlobalPutResponse || message_type_ == MessageType::kGlobalDelResponse || message_type_ == MessageType::kCoveredPlacementGlobalGetResponse)
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
        if (message_type_ == MessageType::kAcquireWritelockRequest || message_type_ == MessageType::kDirectoryLookupRequest || message_type_ == MessageType::kDirectoryUpdateRequest || message_type_ == MessageType::kFinishBlockRequest || message_type_ == MessageType::kInvalidationRequest || message_type_ == MessageType::kReleaseWritelockRequest || message_type_ == MessageType::kCoveredDirectoryLookupRequest || message_type_ == MessageType::kCoveredDirectoryUpdateRequest || message_type_ == MessageType::kCoveredAcquireWritelockRequest || message_type_ == MessageType::kCoveredReleaseWritelockRequest || message_type_ == MessageType::kCoveredPlacementDirectoryUpdateRequest || message_type_ == MessageType::kCoveredPlacementNotifyRequest || message_type_ == MessageType::kCoveredVictimFetchRequest)
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
        if (message_type_ == MessageType::kInitializationRequest || message_type_ == MessageType::kStartrunRequest || message_type_ == MessageType::kSwitchSlotRequest || message_type_ == MessageType::kFinishWarmupRequest || message_type_ == MessageType::kFinishrunRequest)
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
        if (message_type_ == MessageType::kAcquireWritelockResponse || message_type_ == MessageType::kDirectoryLookupResponse || message_type_ == MessageType::kDirectoryUpdateResponse || message_type_ == MessageType::kFinishBlockResponse || message_type_ == MessageType::kInvalidationResponse || message_type_ == MessageType::kReleaseWritelockResponse || message_type_ == MessageType::kCoveredDirectoryLookupResponse || message_type_ == MessageType::kCoveredDirectoryUpdateResponse || message_type_ == MessageType::kCoveredAcquireWritelockResponse || message_type_ == MessageType::kCoveredReleaseWritelockResponse || message_type_ == MessageType::kCoveredPlacementDirectoryUpdateResponse || message_type_ == MessageType::kCoveredVictimFetchResponse)
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
        if (message_type_ == MessageType::kInitializationResponse || message_type_ == MessageType::kStartrunResponse || message_type_ == MessageType::kSwitchSlotResponse || message_type_ == MessageType::kFinishWarmupResponse || message_type_ == MessageType::kFinishrunResponse || message_type_ == MessageType::kSimpleFinishrunResponse)
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
        if (message_type_ == MessageType::kCoveredPlacementRedirectedGetRequest || message_type_ == MessageType::kCoveredPlacementGlobalGetRequest || message_type_ == MessageType::kCoveredPlacementNotifyRequest || message_type_ == MessageType::kCoveredPlacementDirectoryUpdateRequest)
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
        if (message_type_ == MessageType::kCoveredPlacementRedirectedGetResponse || message_type_ == MessageType::kCoveredPlacementGlobalGetResponse || message_type_ == MessageType::kCoveredPlacementDirectoryUpdateResponse)
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
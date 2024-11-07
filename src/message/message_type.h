/*
 * MessageType: the type of messages (break the mutual dependency of declarations between BandwidthUsage and MessageBase).
 * 
 * By Siyuan Sheng (2024.04.03).
 */

#ifndef MESSAGE_TYPE_H
#define MESSAGE_TYPE_H

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
        kDumpSnapshotRequest,
        kDumpSnapshotResponse,
        kSimpleFinishrunResponse,
        kUpdateRulesRequest,
        kUpdateRulesResponse,
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
        kCoveredPlacementTriggerRequest,
        kCoveredPlacementTriggerResponse,
        kCoveredFghybridPlacementTriggerResponse,
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
}

#endif
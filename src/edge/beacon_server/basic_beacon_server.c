#include "edge/beacon_server/basic_beacon_server.h"

#include <assert.h>
#include <sstream>

#include "cache/basic_cache_custom_func_param.h"
#include "common/config.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_custom_func_param.h"
#include "edge/basic_edge_custom_func_param.h"
#include "event/event.h"
#include "event/event_list.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicBeaconServer::kClassName("BasicBeaconServer");

    BasicBeaconServer::BasicBeaconServer(EdgeComponentParam* edge_beacon_server_param_ptr) : BeaconServerBase(edge_beacon_server_param_ptr)
    {
        assert(edge_beacon_server_param_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr != NULL);
        assert(tmp_edge_wrapper_ptr->getCacheName() != Util::COVERED_CACHE_NAME);
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        // Differentiate BasicBeaconServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicBeaconServer::~BasicBeaconServer() {}

    // (1) Access content directory information

    bool BasicBeaconServer::processReqToLookupLocalDirectory_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint& fast_path_hint, BandwidthUsage& total_bandwidth_usage, EventList& event_list) const
    {
        assert(control_request_ptr != NULL);
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Get key from control request if any
        Key tmp_key;
        const MessageType mesasge_type = control_request_ptr->getMessageType();
        if (mesasge_type == MessageType::kDirectoryLookupRequest)
        {
            const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
            tmp_key = directory_lookup_request_ptr->getKey();
        }
        else if (mesasge_type == MessageType::kBestGuessDirectoryLookupRequest)
        {
            const BestGuessDirectoryLookupRequest* const bestguess_directory_lookup_request_ptr = static_cast<const BestGuessDirectoryLookupRequest*>(control_request_ptr);
            tmp_key = bestguess_directory_lookup_request_ptr->getKey();
            BestGuessSyncinfo syncinfo = bestguess_directory_lookup_request_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(mesasge_type) << " for BasicBeaconServer::processReqToLookupLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Lookup local content directory
        is_being_written = false;
        bool is_source_cached = false;
        tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->lookupDirectoryTableByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, is_being_written, is_valid_directory_exist, directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(fast_path_hint);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToLookupLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_valid_directory_exist, const DirectoryInfo& directory_info, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const FastPathHint& fast_path_hint, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        assert(control_request_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();
        
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const MessageType message_type = control_request_ptr->getMessageType();
        MessageBase* directory_lookup_response_ptr = NULL;
        if (message_type == MessageType::kDirectoryLookupRequest)
        {
            // Get key and extra_common_msghdr from control request if any
            const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
            Key tmp_key = directory_lookup_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = directory_lookup_request_ptr->getExtraCommonMsghdr();

            directory_lookup_response_ptr = new DirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else if (message_type == MessageType::kBestGuessDirectoryLookupRequest)
        {
            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            // Get key and extra_common_msghdr from control request if any
            const BestGuessDirectoryLookupRequest* const bestguess_directory_lookup_request_ptr = static_cast<const BestGuessDirectoryLookupRequest*>(control_request_ptr);
            Key tmp_key = bestguess_directory_lookup_request_ptr->getKey();
            const ExtraCommonMsghdr extra_common_msghdr = bestguess_directory_lookup_request_ptr->getExtraCommonMsghdr();

            directory_lookup_response_ptr = new BestGuessDirectoryLookupResponse(tmp_key, is_being_written, is_valid_directory_exist, directory_info, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::getRspToLookupLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(directory_lookup_response_ptr != NULL);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(fast_path_hint);

        return directory_lookup_response_ptr;
    }

    bool BasicBeaconServer::processReqToUpdateLocalDirectory_(MessageBase* control_request_ptr, bool& is_being_written, bool& is_neighbor_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        // Foreground directory updates for baselines
        assert(control_request_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        Key tmp_key;
        bool is_admit = false;
        DirectoryInfo directory_info;
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();

        const MessageType message_type = control_request_ptr->getMessageType();
        if (message_type == MessageType::kDirectoryUpdateRequest)
        {
            const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = directory_update_request_ptr->getKey();
            is_admit = directory_update_request_ptr->isValidDirectoryExist();
            directory_info = directory_update_request_ptr->getDirectoryInfo();

            // Update local directory information in cooperation wrapper
            is_being_written = false;
            MetadataUpdateRequirement unused_metadata_update_requirement;
            tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, source_edge_idx, is_admit, directory_info, is_being_written, is_neighbor_cached, unused_metadata_update_requirement);
            UNUSED(unused_metadata_update_requirement); // ONLY used by COVERED
        }
        else if (message_type == MessageType::kBestGuessDirectoryUpdateRequest || message_type == MessageType::kBestGuessBgplaceDirectoryUpdateRequest)
        {
            const BestGuessDirectoryUpdateRequest* const bestguess_directory_update_request_ptr = static_cast<const BestGuessDirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = bestguess_directory_update_request_ptr->getKey();
            is_admit = bestguess_directory_update_request_ptr->isValidDirectoryExist();
            directory_info = bestguess_directory_update_request_ptr->getDirectoryInfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(bestguess_directory_update_request_ptr->getSourceIndex(), bestguess_directory_update_request_ptr->getSyncinfo().getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);

            if (is_admit) // Foreground/background directory admission (i.e., validation for BestGuess) by local/remote placement notification
            {
                // Validate dirinfo preserved in directory table when triggering best-guess placement
                ValidateDirectoryTableForPreservedDirinfoFuncParam tmp_param_for_validation(tmp_key, source_edge_idx, directory_info, is_being_written, is_neighbor_cached);
                tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->customFunc(ValidateDirectoryTableForPreservedDirinfoFuncParam::FUNCNAME, &tmp_param_for_validation);
                assert(!tmp_param_for_validation.isNeighborCachedRef()); // NOTE: sender MUST be the single placement of the object
                assert(tmp_param_for_validation.isSuccessfulValidationConstRef()); // NOTE: key and dirinfo MUST exist in directory table for validation
            }
            else // Foreground/background directory eviction by value update or remote placement notification
            {
                // Update local directory information in cooperation wrapper
                is_being_written = false;
                MetadataUpdateRequirement unused_metadata_update_requirement;
                tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->updateDirectoryTable(tmp_key, source_edge_idx, is_admit, directory_info, is_being_written, is_neighbor_cached, unused_metadata_update_requirement);
                UNUSED(unused_metadata_update_requirement); // ONLY used by COVERED
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::processReqToUpdateLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        
        // ONLY used by COVERED
        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToUpdateLocalDirectory_(MessageBase* control_request_ptr, const bool& is_being_written, const bool& is_neighbor_cached, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(control_request_ptr != NULL);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();
        
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        Key tmp_key;
        ExtraCommonMsghdr extra_common_msghdr;

        const MessageType message_type = control_request_ptr->getMessageType();
        MessageBase* directory_update_response_ptr = NULL;
        if (message_type == MessageType::kDirectoryUpdateRequest)
        {
            const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = directory_update_request_ptr->getKey();
            extra_common_msghdr = directory_update_request_ptr->getExtraCommonMsghdr();

            directory_update_response_ptr = new DirectoryUpdateResponse(tmp_key, is_being_written, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else if (message_type == MessageType::kBestGuessDirectoryUpdateRequest || message_type == MessageType::kBestGuessBgplaceDirectoryUpdateRequest)
        {
            const BestGuessDirectoryUpdateRequest* const bestguess_directory_update_request_ptr = static_cast<const BestGuessDirectoryUpdateRequest*>(control_request_ptr);
            tmp_key = bestguess_directory_update_request_ptr->getKey();
            extra_common_msghdr = bestguess_directory_update_request_ptr->getExtraCommonMsghdr();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            if (message_type == MessageType::kBestGuessDirectoryUpdateRequest) // Foreground directory admission/eviction by local placement notification or value update
            {
                directory_update_response_ptr = new BestGuessDirectoryUpdateResponse(tmp_key, is_being_written, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
            }
            else // Background directory admission/eviction by remote placement notification
            {
                directory_update_response_ptr = new BestGuessBgplaceDirectoryUpdateResponse(tmp_key, is_being_written, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::getRspToUpdateLocalDirectory_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(directory_update_response_ptr != NULL);

        UNUSED(is_neighbor_cached);
        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return directory_update_response_ptr;
    }

    // (2) Process writes and unblock for MSI protocol

    bool BasicBeaconServer::processReqToAcquireLocalWritelock_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, LockResult& lock_result, DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        const uint32_t source_edge_idx = control_request_ptr->getSourceIndex();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        const MessageType message_type = control_request_ptr->getMessageType();
        Key tmp_key;
        if (message_type == MessageType::kAcquireWritelockRequest)
        {
            const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
            tmp_key = acquire_writelock_request_ptr->getKey();
        }
        else if (message_type == MessageType::kBestGuessAcquireWritelockRequest)
        {
            const BestGuessAcquireWritelockRequest* const bestguess_acquire_writelock_request_ptr = static_cast<const BestGuessAcquireWritelockRequest*>(control_request_ptr);
            tmp_key = bestguess_acquire_writelock_request_ptr->getKey();
            BestGuessSyncinfo syncinfo = bestguess_acquire_writelock_request_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::processReqToAcquireLocalWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Get result of acquiring local write lock
        bool is_source_cached = false;
        lock_result = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->acquireLocalWritelockByBeaconServer(tmp_key, source_edge_idx, edge_cache_server_worker_recvreq_dst_addr, all_dirinfo, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToAcquireLocalWritelock_(MessageBase* control_request_ptr, const LockResult& lock_result, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        assert(control_request_ptr != NULL);

        const MessageType message_type = control_request_ptr->getMessageType();
        MessageBase* acquire_writelock_response_ptr = NULL;
        if (message_type == MessageType::kAcquireWritelockRequest)
        {
            const AcquireWritelockRequest* const acquire_writelock_request_ptr = static_cast<const AcquireWritelockRequest*>(control_request_ptr);
            Key tmp_key = acquire_writelock_request_ptr->getKey();
            ExtraCommonMsghdr extra_common_msghdr = acquire_writelock_request_ptr->getExtraCommonMsghdr();

            acquire_writelock_response_ptr = new AcquireWritelockResponse(tmp_key, lock_result, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else if (message_type == MessageType::kBestGuessAcquireWritelockRequest)
        {
            const BestGuessAcquireWritelockRequest* const bestguess_acquire_writelock_request_ptr = static_cast<const BestGuessAcquireWritelockRequest*>(control_request_ptr);
            Key tmp_key = bestguess_acquire_writelock_request_ptr->getKey();
            ExtraCommonMsghdr extra_common_msghdr = bestguess_acquire_writelock_request_ptr->getExtraCommonMsghdr();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            acquire_writelock_response_ptr = new BestGuessAcquireWritelockResponse(tmp_key, lock_result, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::getRspToAcquireLocalWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(acquire_writelock_response_ptr != NULL);

        return acquire_writelock_response_ptr;
    }

    bool BasicBeaconServer::processReqToReleaseLocalWritelock_(MessageBase* control_request_ptr, std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, BandwidthUsage& total_bandwidth_usage, EventList& event_list)
    {
        assert(control_request_ptr != NULL);
        const MessageType message_type = control_request_ptr->getMessageType();
        uint32_t sender_edge_idx = control_request_ptr->getSourceIndex();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        bool is_finish = false;

        // Get key from request
        Key tmp_key;
        if (message_type == MessageType::kReleaseWritelockRequest)
        {
            const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
            tmp_key = release_writelock_request_ptr->getKey();
        }
        else if (message_type == MessageType::kBestGuessReleaseWritelockRequest)
        {
            const BestGuessReleaseWritelockRequest* const bestguess_release_writelock_request_ptr = static_cast<const BestGuessReleaseWritelockRequest*>(control_request_ptr);
            tmp_key = bestguess_release_writelock_request_ptr->getKey();
            BestGuessSyncinfo syncinfo = bestguess_release_writelock_request_ptr->getSyncinfo();

            // Vtime synchronization
            UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(sender_edge_idx, syncinfo.getVtime());
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::processReqToReleaseLocalWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Release local write lock and validate sender directory info if any
        DirectoryInfo sender_directory_info(sender_edge_idx);
        bool is_source_cached = false;
        blocked_edges = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->releaseLocalWritelock(tmp_key, sender_edge_idx, sender_directory_info, is_source_cached);
        UNUSED(is_source_cached);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        return is_finish;
    }

    MessageBase* BasicBeaconServer::getRspToReleaseLocalWritelock_(MessageBase* control_request_ptr, const Edgeset& best_placement_edgeset, const bool& need_hybrid_fetching, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        checkPointers_();

        assert(control_request_ptr != NULL);
        const MessageType message_type = control_request_ptr->getMessageType();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        MessageBase* release_writelock_response_ptr = NULL;
        if (message_type == MessageType::kReleaseWritelockRequest)
        {
            const ReleaseWritelockRequest* const release_writelock_request_ptr = static_cast<const ReleaseWritelockRequest*>(control_request_ptr);
            Key tmp_key = release_writelock_request_ptr->getKey();
            ExtraCommonMsghdr extra_common_msghdr = release_writelock_request_ptr->getExtraCommonMsghdr();

            uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            release_writelock_response_ptr = new ReleaseWritelockResponse(tmp_key, edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else if (message_type == MessageType::kBestGuessReleaseWritelockRequest)
        {
            const BestGuessReleaseWritelockRequest* const bestguess_release_writelock_request_ptr = static_cast<const BestGuessReleaseWritelockRequest*>(control_request_ptr);
            Key tmp_key = bestguess_release_writelock_request_ptr->getKey();
            ExtraCommonMsghdr extra_common_msghdr = bestguess_release_writelock_request_ptr->getExtraCommonMsghdr();

            // Get local victim vtime for vtime synchronization
            GetLocalVictimVtimeFuncParam tmp_param_for_vtimesync;
            tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_vtimesync);
            const uint64_t& local_victim_vtime = tmp_param_for_vtimesync.getLocalVictimVtimeRef();

            uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
            release_writelock_response_ptr = new BestGuessReleaseWritelockResponse(tmp_key, BestGuessSyncinfo(local_victim_vtime), edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for BasicBeaconServer::getRspToReleaseLocalWritelock_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        assert(release_writelock_response_ptr != NULL);

        UNUSED(best_placement_edgeset);
        UNUSED(need_hybrid_fetching);

        return release_writelock_response_ptr;
    }

    // (3) Process other control requests
    
    bool BasicBeaconServer::processOtherControlRequest_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);

        bool is_finish = false;

        std::ostringstream oss;
        oss << "control request " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " is not supported!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return is_finish;
    }

    // (5) Cache-method-specific custom functions

    void BasicBeaconServer::customFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr)
    {
        assert(func_param_ptr != NULL);

        if (funcname == ProcessPlacementTriggerRequestForBestGuessFuncParam::FUNCNAME)
        {
            ProcessPlacementTriggerRequestForBestGuessFuncParam* tmp_param_ptr = static_cast<ProcessPlacementTriggerRequestForBestGuessFuncParam*>(func_param_ptr);

            bool& is_finish_ref = tmp_param_ptr->isFinishRef();
            is_finish_ref = processPlacementTriggerRequestForBestGuess_(tmp_param_ptr->getControlRequestPtr(), tmp_param_ptr->getEdgeCacheServerWorkerRecvRspDstAddrConstRef());
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid funcname " << funcname << " for BasicBeaconServer::customFunc()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    bool BasicBeaconServer::processPlacementTriggerRequestForBestGuess_(MessageBase* control_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        assert(control_request_ptr != NULL);
        assert(edge_cache_server_worker_recvrsp_dst_addr.isValidAddr());

        checkPointers_();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_beacon_server_param_ptr_->getEdgeWrapperPtr();

        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::BESTGUESS_CACHE_NAME);
        const uint32_t current_beacon_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received placement trigger request
        uint32_t cross_edge_placement_trigger_req_bandwidth_bytes = control_request_ptr->getMsgBandwidthSize();
        total_bandwidth_usage.update(BandwidthUsage(0, cross_edge_placement_trigger_req_bandwidth_bytes, 0, 0, 1, 0));

        // Process received request
        assert(control_request_ptr->getMessageType() == MessageType::kBestGuessPlacementTriggerRequest);
        const BestGuessPlacementTriggerRequest* const best_guess_placement_trigger_request_ptr = static_cast<BestGuessPlacementTriggerRequest*>(control_request_ptr);
        const uint32_t source_edge_idx = best_guess_placement_trigger_request_ptr->getSourceIndex();
        const Key key = best_guess_placement_trigger_request_ptr->getKey();
        const Value value = best_guess_placement_trigger_request_ptr->getValue();
        const BestGuessPlaceinfo placeinfo = best_guess_placement_trigger_request_ptr->getPlaceinfo();
        const uint32_t placement_edge_idx = placeinfo.getPlacementEdgeIdx();
        const BestGuessSyncinfo syncinfo = best_guess_placement_trigger_request_ptr->getSyncinfo();
        const ExtraCommonMsghdr extra_common_msghdr = best_guess_placement_trigger_request_ptr->getExtraCommonMsghdr();

        // Vtime synchronization
        UpdateNeighborVictimVtimeParam tmp_param_for_neighborvtime(source_edge_idx, syncinfo.getVtime());
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->customFunc(UpdateNeighborVictimVtimeParam::FUNCNAME, &tmp_param_for_neighborvtime);

        // Try to preserve invalid dirinfo in directory table for trigger flag
        PreserveDirectoryTableIfGlobalUncachedFuncParam tmp_param_for_preserve(key, DirectoryInfo(placement_edge_idx));
        tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->customFunc(PreserveDirectoryTableIfGlobalUncachedFuncParam::FUNCNAME, &tmp_param_for_preserve);
        const bool is_triggered = tmp_param_for_preserve.isSuccessfulPreservationConstRef();

        // Get local victim vtime for vtime synchronization
        GetLocalVictimVtimeFuncParam tmp_param_for_localvtime;
        tmp_edge_wrapper_ptr->getEdgeCachePtr()->constCustomFunc(GetLocalVictimVtimeFuncParam::FUNCNAME, &tmp_param_for_localvtime);
        const uint64_t& local_victim_vtime = tmp_param_for_localvtime.getLocalVictimVtimeRef();

        if (is_triggered) // The first placement trigger of global uncached object
        {
            if (placement_edge_idx == source_edge_idx) // Sender is placement
            {
                // NOTE: sender will perform local placement after receiving BestGuessTriggerPlacementResponse
                
                // Do nothing
            }
            else if (placement_edge_idx == current_beacon_edge_idx) // Local placement
            {
                assert(tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key) == current_beacon_edge_idx); // Current MUST be the beacon node of the key

                // Admit local beacon directory
                bool is_being_written = false;
                bool unused_is_neighbor_cached = false; // ONLY used by COVERED for local cached reward calculation
                tmp_edge_wrapper_ptr->admitLocalDirectory_(key, DirectoryInfo(placement_edge_idx), is_being_written, unused_is_neighbor_cached, extra_common_msghdr);
                assert(!unused_is_neighbor_cached); // (i) MUST be false due to ONLY preserving dirinfo for the first placement trigger of global miss; (ii) is_neighbor_cached will NOT be used by BestGuess local edge cachce

                // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton in the background
                const bool is_valid = !is_being_written;
                bool is_successful = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, unused_is_neighbor_cached, is_valid, extra_common_msghdr));
                assert(is_successful);
            }
            else // Remote placement notification
            {
                // Prepare BestGuess placement notify request
                const bool is_being_written = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->isBeingWritten(key);
                const bool is_valid = !is_being_written;
                BestGuessBgplacePlacementNotifyRequest* bestguess_placement_notify_request_ptr = new BestGuessBgplacePlacementNotifyRequest(key, value, is_valid, BestGuessSyncinfo(local_victim_vtime), current_beacon_edge_idx, edge_beacon_server_recvreq_source_addr_, extra_common_msghdr);
                assert(bestguess_placement_notify_request_ptr != NULL);

                // Issue remote placement notification to placement node with key and value
                NetworkAddr placement_edge_cache_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getTargetDstaddr(DirectoryInfo(placement_edge_idx)); // Send to cache server of the placement edge node for cache server placement processor
                bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(bestguess_placement_notify_request_ptr, placement_edge_cache_server_recvreq_dst_addr);
                assert(is_successful);

                bestguess_placement_notify_request_ptr = NULL; // NOTE: bestguess_placement_notify_request_ptr will be released by edge-to-edge propagation simulator
            }
        }

        embedBackgroundCounterIfNotEmpty_(total_bandwidth_usage, event_list); // Embed background events/bandwidth if any into control response message

        // Generate response
        BestGuessPlacementTriggerResponse* best_guess_placement_trigger_response_ptr = new BestGuessPlacementTriggerResponse(key, is_triggered, BestGuessSyncinfo(local_victim_vtime), current_beacon_edge_idx, edge_beacon_server_recvreq_source_addr_, total_bandwidth_usage, event_list, extra_common_msghdr);
        assert(best_guess_placement_trigger_response_ptr != NULL);

        // Push the response into edge-to-edge propagation simulator to cache server worker
        bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(best_guess_placement_trigger_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: best_guess_placement_trigger_response_ptr will be released by edge-to-edge propagation simulator
        best_guess_placement_trigger_response_ptr = NULL;

        return is_finish;
    }
}
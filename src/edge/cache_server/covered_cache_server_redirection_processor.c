#include "edge/cache_server/covered_cache_server_redirection_processor.h"

#include "edge/covered_edge_custom_func_param.h"
#include "message/data_message.h"

namespace covered
{
    const std::string CoveredCacheServerRedirectionProcessor::kClassName("CoveredCacheServerRedirectionProcessor");

    CoveredCacheServerRedirectionProcessor::CoveredCacheServerRedirectionProcessor(CacheServerProcessorParam* cache_server_redirection_processor_param_ptr) : CacheServerRedirectionProcessorBase(cache_server_redirection_processor_param_ptr)
    {
        assert(cache_server_redirection_processor_param_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr->getCacheName() == Util::COVERED_CACHE_NAME);
        
        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        oss << kClassName << " edge" << edge_idx << "-redirection-processor";
        instance_name_ = oss.str();
    }

    CoveredCacheServerRedirectionProcessor::~CoveredCacheServerRedirectionProcessor()
    {}

    void CoveredCacheServerRedirectionProcessor::processReqForRedirectedGet_(MessageBase* redirected_request_ptr, Value& value, bool& is_cooperative_cached, bool& is_cooperative_cached_and_valid) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        const bool is_redirected = true;
        assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
        // CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Get key and victim syncset from redirected get request
        Key tmp_key;
        uint32_t source_edge_idx = redirected_request_ptr->getSourceIndex();
        VictimSyncset neighbor_victim_syncset;
        if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
        {
            const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_redirected_get_request_ptr->getKey();
            neighbor_victim_syncset = covered_redirected_get_request_ptr->getVictimSyncsetRef();
        }
        else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
            neighbor_victim_syncset = covered_placement_redirected_get_request_ptr->getVictimSyncsetRef();
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Access local edge cache for redirected get request
        is_cooperative_cached_and_valid = tmp_edge_wrapper_ptr->getLocalEdgeCache_(tmp_key, is_redirected, value);
        is_cooperative_cached = tmp_edge_wrapper_ptr->getEdgeCachePtr()->isLocalCached(tmp_key);

        // Victim synchronization
        UpdateCacheManagerForNeighborVictimSyncsetFuncParam tmp_param(source_edge_idx, neighbor_victim_syncset);
        tmp_edge_wrapper_ptr->constCustomFunc(UpdateCacheManagerForNeighborVictimSyncsetFuncParam::FUNCNAME, &tmp_param);
        
        return;
    }

    MessageBase* CoveredCacheServerRedirectionProcessor::getRspForRedirectedGet_(MessageBase* redirected_request_ptr, const Value& value, const Hitflag& hitflag, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) const
    {
        assert(redirected_request_ptr != NULL);

        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_redirection_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = tmp_cache_server_ptr->getEdgeCacheServerRecvreqPublicSourceAddr(); // NOTE: cross-edge communication for request redirection uses public IP address

        MessageBase* redirected_get_response_ptr = NULL;
        assert(redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest || redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest);
        CoveredCacheManager* tmp_covered_cache_manager_ptr = tmp_edge_wrapper_ptr->getCoveredCacheManagerPtr();

        // Get key (and placement edgeset) from redirected get request
        Key tmp_key;
        Edgeset tmp_placement_edgeset;
        bool skip_propagation_latency = redirected_request_ptr->isSkipPropagationLatency();
        if (redirected_request_ptr->getMessageType() == MessageType::kCoveredRedirectedGetRequest)
        {
            const CoveredRedirectedGetRequest* const covered_redirected_get_request_ptr = static_cast<const CoveredRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_redirected_get_request_ptr->getKey();
        }
        else if (redirected_request_ptr->getMessageType() == MessageType::kCoveredPlacementRedirectedGetRequest)
        {
            const CoveredPlacementRedirectedGetRequest* const covered_placement_redirected_get_request_ptr = static_cast<const CoveredPlacementRedirectedGetRequest*>(redirected_request_ptr);
            tmp_key = covered_placement_redirected_get_request_ptr->getKey();
            tmp_placement_edgeset = covered_placement_redirected_get_request_ptr->getEdgesetRef();
        }
        else
        {
            std::ostringstream oss;
            oss << "Invalid message type " << MessageBase::messageTypeToString(redirected_request_ptr->getMessageType()) << " for processReqForRedirectedGet_()";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Prepare victim syncset for piggybacking-based victim synchronization
        const uint32_t dst_edge_idx_for_compression = redirected_request_ptr->getSourceIndex();
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, tmp_edge_wrapper_ptr->getCacheMarginBytes());

        // Prepare redirected get response
        if (!redirected_request_ptr->isBackgroundRequest()) // Send back normal redirected get response
        {
            // NOTE: CoveredRedirectedGetResponse will be processed by edge cache server worker of the sender edge node
            redirected_get_response_ptr = new CoveredRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        else // Send back redirected get response for non-blocking placement deploymeng
        {
            assert(tmp_placement_edgeset.size() <= tmp_edge_wrapper_ptr->getTopkEdgecntForPlacement()); // At most k placement edge nodes each time

            // NOTE: CoveredPlacementRedirectedGetResponse will be processed by edge beacon server of the sender edge node
            redirected_get_response_ptr = new CoveredPlacementRedirectedGetResponse(tmp_key, value, hitflag, victim_syncset, tmp_placement_edgeset, edge_idx, edge_cache_server_recvreq_source_addr, total_bandwidth_usage, event_list, skip_propagation_latency);
        }
        assert(redirected_get_response_ptr != NULL);

        return redirected_get_response_ptr;
    }
}
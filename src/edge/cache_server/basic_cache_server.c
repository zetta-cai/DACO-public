#include "edge/cache_server/basic_cache_server.h"

#include "common/util.h"
#include "message/control_message.h"

namespace covered
{
    const std::string BasicCacheServer::kClassName("BasicCacheServer");

    BasicCacheServer::BasicCacheServer(EdgeWrapperBase* edge_wrapper_ptr) : CacheServerBase(edge_wrapper_ptr)
    {
        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_wrapper_ptr->getNodeIdx();
        instance_name_ = oss.str();
    }

    BasicCacheServer::~BasicCacheServer()
    {}

    // (1) For local edge cache admission and remote directory admission

    void BasicCacheServer::admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid) const
    {
        checkPointers_();

        bool affect_victim_tracker = false; // If key is a local synced victim now
        const uint32_t beacon_edgeidx = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        edge_wrapper_ptr_->getEdgeCachePtr()->admit(key, value, is_neighbor_cached, is_valid, beacon_edgeidx, affect_victim_tracker);

        return;
    }

    MessageBase* BasicCacheServer::getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        const bool is_admit = true; // Try to admit a new key as local cached object (NOTE: local edge cache has NOT been admitted yet)
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* directory_update_request_ptr = NULL;
        directory_update_request_ptr = new DirectoryUpdateRequest(key, is_admit, directory_info, edge_idx, source_addr, skip_propagation_latency);
        assert(directory_update_request_ptr != NULL);

        return directory_update_request_ptr;
    }

    void BasicCacheServer::processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_neighbor_cached, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        assert(control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

        // Get is_being_written from control response message
        const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
        is_being_written = directory_update_response_ptr->isBeingWritten();

        return;
    }

    // (2) For blocking-based cache eviction and local/remote directory eviction (invoked by edge cache server worker for independent admission or value update; or by placement processor for remote placement notification)
    
    bool BasicCacheServer::processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        bool is_finish = false;

        assert(control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

        // Get is_being_written (UNUSED) from control response message
        const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
        is_being_written = directory_update_response_ptr->isBeingWritten();

        UNUSED(recvrsp_source_addr);
        UNUSED(recvrsp_socket_server_ptr);
        UNUSED(total_bandwidth_usage);
        UNUSED(event_list);
        UNUSED(skip_propagation_latency);

        return is_finish;
    }

    // (3) Method-specific functions

    void BasicCacheServer::constCustomFunc(const std::string& funcname, EdgeCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << funcname << " is not supported in constCustomFunc()";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }
}
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
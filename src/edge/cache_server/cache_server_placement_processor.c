#include "edge/cache_server/cache_server_placement_processor.h"

#include <assert.h>

#include "common/config.h"

namespace covered
{
    const std::string CacheServerPlacementProcessor::kClassName = "CacheServerPlacementProcessor";

    void* CacheServerPlacementProcessor::launchCacheServerPlacementProcessor(void* cache_serer_placement_processor_param_ptr)
    {
        assert(cache_serer_placement_processor_param_ptr != NULL);

        CacheServerPlacementProcessor cache_server_placement_processor((CacheServerPlacementProcessorParam*)cache_serer_placement_processor_param_ptr);
        cache_server_placement_processor.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerPlacementProcessor::CacheServerPlacementProcessor(CacheServerPlacementProcessorParam* cache_serer_placement_processor_param_ptr) : cache_serer_placement_processor_param_ptr_(cache_serer_placement_processor_param_ptr)
    {
        assert(cache_serer_placement_processor_param_ptr != NULL);
        const uint32_t edge_idx = cache_serer_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeIdx();
        const uint32_t edgecnt = cache_serer_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-placement-processor";
        instance_name_ = oss.str();

        // For receiving control responses

        // Get source address of cache server placement processor to receive control responses and redirected data responses
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_placement_processor_recvrsp_port = Util::getEdgeCacheServerPlacementProcessorRecvrspPort(edge_idx, edgecnt);
        edge_cache_server_placement_processor_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_placement_processor_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_placement_processor_recvrsp_port);
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
    }

    CacheServerPlacementProcessor::~CacheServerPlacementProcessor()
    {
        // NOTE: no need to release cache_serer_placement_processor_param_ptr_, which will be released outside CacheServerPlacementProcessor (by CacheServer)

        // Release the socket server to receive control responses,
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_placement_processor_recvrsp_socket_server_ptr_;
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = NULL;
    }

    void CacheServerPlacementProcessor::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_serer_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data management request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful = cache_serer_placement_processor_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getDataRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isManagementDataRequest()) // Management data requests
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processManagementDataRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the cache server placement processor thread
                assert(data_request_ptr != NULL);
                delete data_request_ptr;
                data_request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful == false)
        } // End of while loop

        return;
    }

    bool CacheServerPlacementProcessor::processManagementDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        bool is_finish = false;

        // TODO: (END HERE) Process received CoveredPlacementNotifyRequest

        return is_finish;
    }

    void CacheServerPlacementProcessor::checkPointers_() const
    {
        assert(cache_serer_placement_processor_param_ptr_ != NULL);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
    }
}
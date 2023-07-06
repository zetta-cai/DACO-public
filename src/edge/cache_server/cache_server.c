#include "edge/cache_server/cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/cache_server/cache_server_worker_base.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CacheServer::kClassName("CacheServer");

    CacheServer::CacheServer(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        assert(edge_wrapper_ptr->edge_param_ptr_ != NULL);
        uint32_t edge_idx = edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        uint32_t edgecnt = edge_wrapper_ptr->edgecnt_;

        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Allocate hash wrapper for partition
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapperByHashName(Param::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);

        // Prepare parameters for cache server threads
        cache_server_worker_params_.resize(edge_wrapper_ptr_->percacheserver_workercnt_);
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < edge_wrapper_ptr_->percacheserver_workercnt_; local_cache_server_worker_idx++)
        {
            CacheServerWorkerParam tmp_cache_server_worker_param(this, local_cache_server_worker_idx, Config::getEdgeCacheServerDataRequestBufferSize());
            cache_server_worker_params_[local_cache_server_worker_idx] = tmp_cache_server_worker_param;
        }

        // For receiving local requests

        // Get source address of cache server to receive local requests
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(edge_idx, edgecnt);
        edge_cache_server_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_recvreq_port);

        // Prepare a socket server on recvreq port for cache server
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_cache_server_recvreq_port);
        edge_cache_server_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
    }

    CacheServer::~CacheServer()
    {
        // No need to release edge_wrapper_ptr_, which is performed outside CacheServer

        // Release hash wrapper for partition
        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_recvreq_socket_server_ptr_;
        edge_cache_server_recvreq_socket_server_ptr_ = NULL;
    }

    void CacheServer::start()
    {
        checkPointers_();
        
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getNodeIdx();

        int pthread_returncode;
        pthread_t cache_server_worker_threads[edge_wrapper_ptr_->percacheserver_workercnt_];

        // Launch cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < edge_wrapper_ptr_->percacheserver_workercnt_; local_cache_server_worker_idx++)
        {
            pthread_returncode = pthread_create(&cache_server_worker_threads[local_cache_server_worker_idx], NULL, CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " failed to launch cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Receive data requests and partition to different cache server workers
        receiveRequestsAndPartition_();

        // Wait for cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < edge_wrapper_ptr_->percacheserver_workercnt_; local_cache_server_worker_idx++)
        {
            pthread_returncode = pthread_join(cache_server_worker_threads[local_cache_server_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " failed to join cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        return;
    }

    void CacheServer::receiveRequestsAndPartition_()
    {
        checkPointers_();

        while (edge_wrapper_ptr_->edge_param_ptr_->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Receive the message payload of data (local/redirected) requests
            DynamicArray data_request_msg_payload;
            bool is_timeout = edge_cache_server_recvreq_socket_server_ptr_->recv(data_request_msg_payload);
            if (is_timeout == true) // Timeout-and-retry
            {
                continue; // Retry to receive a message if edge is still running
            } // End of (is_timeout == true)
            else
            {                
                MessageBase* data_request_ptr = MessageBase::getRequestFromMsgPayload(data_request_msg_payload);
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    // Pass data request and network address to corresponding cache server worker by ring buffer
                    // NOTE: data request will be released by the corresponding cache server worker
                    partitionRequest_(data_request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }
            } // End of (is_timeout == false)
        } // End of while loop

        return;
    }

    void CacheServer::partitionRequest_(MessageBase* data_requeset_ptr)
    {
        assert(data_requeset_ptr != NULL && data_requeset_ptr->isDataRequest());

        // Calculate the corresponding cache server worker index by hashing
        assert(cache_server_worker_params_.size() == edge_wrapper_ptr_->percacheserver_workercnt_);
        Key tmp_key = MessageBase::getKeyFromMessage(data_requeset_ptr);
        uint32_t local_cache_server_worker_idx = hash_wrapper_ptr_->hash(tmp_key) % edge_wrapper_ptr_->percacheserver_workercnt_;
        assert(local_cache_server_worker_idx < edge_wrapper_ptr_->percacheserver_workercnt_);

        // Pass cache server worker item into ring buffer
        CacheServerWorkerItem tmp_cache_server_worker_item(data_requeset_ptr);
        bool is_successful = cache_server_worker_params_[local_cache_server_worker_idx].getDataRequestBufferPtr()->push(tmp_cache_server_worker_item);
        assert(is_successful == true); // Ring buffer must NOT be full

        return;
    }

    void CacheServer::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);

        edge_wrapper_ptr_->checkPointers_();
        
        assert(hash_wrapper_ptr_ != NULL);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);

        return;
    }
}
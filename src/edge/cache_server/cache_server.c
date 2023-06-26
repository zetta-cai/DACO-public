#include "edge/cache_server/cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "edge/cache_server/cache_server_worker_param.h"
#include "edge/cache_server/cache_server_worker_base.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CacheServer::kClassName("CacheServer");

    CacheServer::CacheServer(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        assert(edge_wrapper_ptr->edge_param_ptr_ != NULL);
        uint32_t edge_idx = edge_wrapper_ptr->edge_param_ptr_->getEdgeIdx();
        
        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Prepare a socket server on recvreq port for cache server
        uint16_t edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(edge_idx);
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_cache_server_recvreq_port);
        edge_cache_server_recvreq_socket_server_ptr_ = new UdpSocketWrapper(SocketRole::kSocketServer, host_addr);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);

        // Prepare parameters for cache server threads
        uint32_t percacheserver_workercnt = Param::getPercacheserverWorkercnt();
        cache_server_worker_params_.resize(percacheserver_workercnt);
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            CacheServerWorkerParam tmp_cache_server_worker_param(edge_wrapper_ptr_, local_cache_server_worker_idx);
            cache_server_worker_params_[local_cache_server_worker_idx] = tmp_cache_server_worker_param;
        }
    }

    CacheServer::~CacheServer()
    {
        // No need to release edge_wrapper_ptr_, which is performed outside CacheServer

        // Release the socket server on recvreq port
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_recvreq_socket_server_ptr_;
        edge_cache_server_recvreq_socket_server_ptr_ = NULL;
    }

    void CacheServer::start()
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_param_ptr_ != NULL);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr_->getEdgeIdx();

        int pthread_returncode;
        uint32_t percacheserver_workercnt = Param::getPercacheserverWorkercnt();
        pthread_t cache_server_worker_threads[percacheserver_workercnt];

        // Launch cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            pthread_returncode = pthread_create(&cache_server_worker_threads[local_cache_server_worker_idx], NULL, launchCacheServerWorker_, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " failed to launch cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // TODO: Use per-cache-server-worker ring buffer to partition client-issued local requests

        // Wait for cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
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

    void* CacheServer::launchCacheServerWorker_(void* cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);

        CacheServerWorkerBase* cache_server_worker_ptr = CacheServerWorkerBase::getCacheServerWorker((CacheServerWorkerParam*)cache_server_worker_param_ptr);
        assert(cache_server_worker_ptr != NULL);
        cache_server_worker_ptr->start();

        assert(cache_server_worker_ptr != NULL);
        delete cache_server_worker_ptr;
        cache_server_worker_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }
}
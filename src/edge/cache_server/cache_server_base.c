#include "edge/cache_server/cache_server_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "edge/cache_server/cache_server_invalidation_processor_base.h"
#include "edge/cache_server/cache_server_metadata_update_processor.h"
#include "edge/cache_server/cache_server_placement_processor_base.h"
#include "edge/cache_server/cache_server_redirection_processor_base.h"
#include "edge/cache_server/cache_server_victim_fetch_processor.h"
#include "edge/cache_server/cache_server_worker_base.h"
#include "edge/cache_server/basic_cache_server.h"
#include "edge/cache_server/covered_cache_server.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CacheServerBase::kClassName("CacheServerBase");

    // NOTE: do NOT change the following const variables unless you know what you are doing
    // NOTE: you should guarantee the correctness of dedicated corecnt settings in config.json
    const bool CacheServerBase::IS_HIGH_PRIORITY_FOR_CACHE_PLACEMENT = true;
    const bool CacheServerBase::IS_HIGH_PRIORITY_FOR_METADATA_UPDATE = false;
    const bool CacheServerBase::IS_HIGH_PRIORITY_FOR_VICTIM_FETCH = false;

    void* CacheServerBase::launchCacheServer(void* edge_component_ptr)
    {
        assert(edge_component_ptr != NULL);

        CacheServerBase* cache_server_ptr = NULL;
        EdgeComponentParam* tmp_edge_component_ptr = (EdgeComponentParam*)edge_component_ptr;
        const std::string cache_name = tmp_edge_component_ptr->getEdgeWrapperPtr()->getCacheName();
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            cache_server_ptr = new CoveredCacheServer(tmp_edge_component_ptr);
        }
        else
        {
            cache_server_ptr = new BasicCacheServer(tmp_edge_component_ptr);
        }
        assert(cache_server_ptr != NULL);
        cache_server_ptr->start();

        delete cache_server_ptr;
        cache_server_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerBase::CacheServerBase(EdgeComponentParam* edge_component_ptr) : edge_component_ptr_(edge_component_ptr)
    {
        assert(edge_component_ptr != NULL);
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr != NULL);
        uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        uint32_t edgecnt = tmp_edge_wrapper_ptr->getNodeCnt();

        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();

        // Allocate hash wrapper for partition
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapperByHashName(Util::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);

        // Prepare parameters for cache server threads
        const uint32_t percacheserver_workercnt = tmp_edge_wrapper_ptr->getPercacheserverWorkercnt();
        cache_server_worker_params_.resize(percacheserver_workercnt);
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            CacheServerWorkerParam tmp_cache_server_worker_param(this, local_cache_server_worker_idx, Config::getEdgeCacheServerDataRequestBufferSize());
            cache_server_worker_params_[local_cache_server_worker_idx] = tmp_cache_server_worker_param;
        }
        
        // Prepare parameters for cache server metadata update processor thread
        cache_server_metadata_update_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize(), IS_HIGH_PRIORITY_FOR_METADATA_UPDATE);

        // Prepare parameters for cache server victim fetch processor thread
        cache_server_victim_fetch_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize(), IS_HIGH_PRIORITY_FOR_VICTIM_FETCH);
        assert(cache_server_victim_fetch_processor_param_ptr_ != NULL);

        // Prepare parameters for cache server redirection processor thread
        cache_server_redirection_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize(), true);
        assert(cache_server_redirection_processor_param_ptr_ != NULL);

        // Prepare parameters for cache server placement processor thread
        // TODO: If you want to process cache placement decisions in a low priority, you have to split placement processor into local/remote placement processors, each with an individual interruption-based ring buffer
        cache_server_placement_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize(), true); // NOTE: NOT use IS_HIGH_PRIORITY_FOR_CACHE_PLACEMENT here, as false will incur overflow issue of cache placement ring buffer
        assert(cache_server_placement_processor_param_ptr_ != NULL);

        // Prepare parameters for cache server invalidation processor thread
        cache_server_invalidation_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize(), false);
        assert(cache_server_invalidation_processor_param_ptr_ != NULL);

        // For receiving local requests

        // Get private source address of cache server to receive local requests
        const bool is_private_edge_ipstr_for_clients = true; // NOTE: the closest edge communicates with client via private IP address
        const bool is_launch_edge = true; // The edge cache server belongs to the logical edge node launched in the current physical machine
        std::string edge_private_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_private_edge_ipstr_for_clients, is_launch_edge);
        uint16_t edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(edge_idx, edgecnt);
        edge_cache_server_recvreq_private_source_addr_ = NetworkAddr(edge_private_ipstr, edge_cache_server_recvreq_port);

        // Get public source address of cache server to receive local requests
        const bool is_private_edge_ipstr_for_neighbors = false; // NOTE: edge communicates with neighbors via public IP address
        std::string edge_public_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_private_edge_ipstr_for_neighbors, is_launch_edge);
        edge_cache_server_recvreq_public_source_addr_ = NetworkAddr(edge_public_ipstr, edge_cache_server_recvreq_port);

        // Prepare a socket server on recvreq port for cache server
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_cache_server_recvreq_port);
        edge_cache_server_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);

        oss.clear();
        oss.str("");
        oss << base_instance_name_ << " " << "rwlock_for_eviction_ptr_";
        rwlock_for_eviction_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_eviction_ptr_ != NULL);
    }

    CacheServerBase::~CacheServerBase()
    {
        // No need to release edge_component_ptr_, which is performed outside CacheServerBase

        // Release hash wrapper for partition
        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;

        // Release cache server metadata update processor param
        assert(cache_server_metadata_update_processor_param_ptr_ != NULL);
        delete cache_server_metadata_update_processor_param_ptr_;
        cache_server_metadata_update_processor_param_ptr_ = NULL;

        // Release cache server victim fetch processor param
        assert(cache_server_victim_fetch_processor_param_ptr_ != NULL);
        delete cache_server_victim_fetch_processor_param_ptr_;
        cache_server_victim_fetch_processor_param_ptr_ = NULL;

        // Release cache server redirection processor param
        assert(cache_server_redirection_processor_param_ptr_ != NULL);
        delete cache_server_redirection_processor_param_ptr_;
        cache_server_redirection_processor_param_ptr_ = NULL;

        // Release cache server placement processor param
        assert(cache_server_placement_processor_param_ptr_ != NULL);
        delete cache_server_placement_processor_param_ptr_;
        cache_server_placement_processor_param_ptr_ = NULL;

        // Release cache server invalidation processor param
        assert(cache_server_invalidation_processor_param_ptr_ != NULL);
        delete cache_server_invalidation_processor_param_ptr_;
        cache_server_invalidation_processor_param_ptr_ = NULL;

        // Release the socket server on recvreq port
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_recvreq_socket_server_ptr_;
        edge_cache_server_recvreq_socket_server_ptr_ = NULL;

        assert(rwlock_for_eviction_ptr_ != NULL);
        delete rwlock_for_eviction_ptr_;
        rwlock_for_eviction_ptr_ = NULL;
    }

    void CacheServerBase::start()
    {
        checkPointers_();
        
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const uint32_t percacheserver_workercnt = tmp_edge_wrapper_ptr->getPercacheserverWorkercnt();

        int pthread_returncode;
        pthread_t cache_server_worker_threads[percacheserver_workercnt];
        pthread_t cache_server_victim_fetch_processor_thread;
        pthread_t cache_server_redirection_processor_thread;
        pthread_t cache_server_placement_processor_thread;
        pthread_t cache_server_invalidation_processor_thread;
        pthread_t cache_server_metadata_update_processor_thread;

        // Launch cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            //pthread_returncode = pthread_create(&cache_server_worker_threads[local_cache_server_worker_idx], NULL, CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            // if (pthread_returncode != 0)
            // {
            //     std::ostringstream oss;
            //     oss << "edge " << edge_idx << " failed to launch cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            //     exit(1);
            // }
            std::string tmp_thread_name = "edge-cache-server-worker-" + std::to_string(edge_idx) + "-" + std::to_string(local_cache_server_worker_idx);
            ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_worker_threads[local_cache_server_worker_idx], CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
        }

        // Launch cache server victim fetch processor
        //pthread_returncode = pthread_create(&cache_server_victim_fetch_processor_thread, NULL, CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor, (void*)(cache_server_victim_fetch_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server victim fetch processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "edge-cache-server-victim-fetch-processor-" + std::to_string(edge_idx);
        if (IS_HIGH_PRIORITY_FOR_VICTIM_FETCH)
        {
            ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_victim_fetch_processor_thread, CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor, (void*)(cache_server_victim_fetch_processor_param_ptr_));
        }
        else
        {
            ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_victim_fetch_processor_thread, CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor, (void*)(cache_server_victim_fetch_processor_param_ptr_));
        }

        // Launch cache server redirection processor
        //pthread_returncode = pthread_create(&cache_server_redirection_processor_thread, NULL, CacheServerRedirectionProcessorBase::launchCacheServerRedirectionProcessor, (void*)(cache_server_redirection_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server redirection processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-redirection-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_redirection_processor_thread, CacheServerRedirectionProcessorBase::launchCacheServerRedirectionProcessor, (void*)(cache_server_redirection_processor_param_ptr_));

        // Launch cache server placement processor
        //pthread_returncode = pthread_create(&cache_server_placement_processor_thread, NULL, CacheServerPlacementProcessorBase::launchCacheServerPlacementProcessor, (void*)(cache_server_placement_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-placement-processor-" + std::to_string(edge_idx);
        if (IS_HIGH_PRIORITY_FOR_CACHE_PLACEMENT)
        {
            ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_placement_processor_thread, CacheServerPlacementProcessorBase::launchCacheServerPlacementProcessor, (void*)(cache_server_placement_processor_param_ptr_));
        }
        else
        {
            ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_placement_processor_thread, CacheServerPlacementProcessorBase::launchCacheServerPlacementProcessor, (void*)(cache_server_placement_processor_param_ptr_));
        }

        // Launch cache server invalidation processor
        //pthread_returncode = pthread_create(&cache_server_invalidation_processor_thread, NULL, CacheServerInvalidationProcessorBase::launchCacheServerInvalidationProcessor, (void*)(cache_server_invalidation_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server invalidation processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-invalidation-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_invalidation_processor_thread, CacheServerInvalidationProcessorBase::launchCacheServerInvalidationProcessor, (void*)(cache_server_invalidation_processor_param_ptr_));

        // Launch cache server metadata update processor
        //pthread_returncode = pthread_create(&cache_server_metadata_update_processor_thread, NULL, CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor, (void*)(cache_server_metadata_update_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server metadata update processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-metadata-update-processor-" + std::to_string(edge_idx);
        if (IS_HIGH_PRIORITY_FOR_METADATA_UPDATE)
        {
            ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_metadata_update_processor_thread, CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor, (void*)(cache_server_metadata_update_processor_param_ptr_));
        }
        else
        {
            ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_metadata_update_processor_thread, CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor, (void*)(cache_server_metadata_update_processor_param_ptr_));
        }

        // Wait cache server workers to finish initialization
        std::ostringstream tmposs;
        tmposs << "wait " << percacheserver_workercnt << " cache server workers to finish initialization...";
        Util::dumpNormalMsg(base_instance_name_, tmposs.str());
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            while (!cache_server_worker_params_[local_cache_server_worker_idx].isFinishInitialization())
            {
                usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
            }
        }

        // Wait cache server victim fetch processor to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait victim fetch processor to finish initialization...");
        while (!cache_server_victim_fetch_processor_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait cache server redirection processor to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait redirection processor to finish initialization...");
        while (!cache_server_redirection_processor_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait cache server placement processor to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait placement processor to finish initialization...");
        while (!cache_server_placement_processor_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait cache server invalidation processor to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait invalidation processor to finish initialization...");
        while (!cache_server_invalidation_processor_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait cache server metadata update processor to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait metadata update processor to finish initialization...");
        while (!cache_server_metadata_update_processor_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Notify edge wrapper that edge cache server has finished initialization
        edge_component_ptr_->markFinishInitialization();

        // Receive data requests and partition to different cache server workers
        receiveRequestsAndPartition_();

        // Wait for cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            pthread_returncode = pthread_join(cache_server_worker_threads[local_cache_server_worker_idx], NULL); // void* retval = NULL
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " failed to join cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }
        }

        // Wait for cache server victim fetch processor
        pthread_returncode = pthread_join(cache_server_victim_fetch_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server victim fetch processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server redirection processor
        pthread_returncode = pthread_join(cache_server_redirection_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server redirection processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server placement processor
        pthread_returncode = pthread_join(cache_server_placement_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server invalidation processor
        pthread_returncode = pthread_join(cache_server_invalidation_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server invalidation processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server metadata update processor
        pthread_returncode = pthread_join(cache_server_metadata_update_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server metadata update processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    EdgeWrapperBase* CacheServerBase::getEdgeWrapperPtr() const
    {
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();
        assert(tmp_edge_wrapper_ptr != NULL);
        return tmp_edge_wrapper_ptr;
    }

    NetworkAddr CacheServerBase::getEdgeCacheServerRecvreqPrivateSourceAddr() const
    {
        return edge_cache_server_recvreq_private_source_addr_;
    }

    NetworkAddr CacheServerBase::getEdgeCacheServerRecvreqPublicSourceAddr() const
    {
        return edge_cache_server_recvreq_public_source_addr_;
    }

    void CacheServerBase::receiveRequestsAndPartition_()
    {
        checkPointers_();

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
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

                const MessageType message_type = data_request_ptr->getMessageType();
                if (data_request_ptr->isDataRequest() || message_type == MessageType::kCoveredVictimFetchRequest || message_type == MessageType::kCoveredBgplacePlacementNotifyRequest || message_type == MessageType::kCoveredMetadataUpdateRequest || message_type == MessageType::kInvalidationRequest || message_type == MessageType::kCoveredInvalidationRequest || message_type == MessageType::kBestGuessBgplacePlacementNotifyRequest || message_type == MessageType::kBestGuessInvalidationRequest) // Local data requests for cache server workers; redirected data requests for cache server redirection processor; lazy victim fetching for cache server victim fetch processor; placement notification for cache server placement processor; metadata update requests for cache server metadata update processor; invalidation requests for cache server invalidation processor
                {
                    // Pass received request and network address to corresponding cache server worker/processor by ring buffer
                    // NOTE: received request will be released by the corresponding cache server worker/processor
                    partitionRequest_(data_request_ptr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }
            } // End of (is_timeout == false)
        } // End of while loop

        // Notify all processors to finish if necessary (i.e., with interruption-based ring buffer)
        cache_server_metadata_update_processor_param_ptr_->notifyFinishIfNecessary(tmp_edge_wrapper_ptr);
        cache_server_victim_fetch_processor_param_ptr_->notifyFinishIfNecessary(tmp_edge_wrapper_ptr);
        cache_server_redirection_processor_param_ptr_->notifyFinishIfNecessary(tmp_edge_wrapper_ptr);
        cache_server_placement_processor_param_ptr_->notifyFinishIfNecessary(tmp_edge_wrapper_ptr);
        cache_server_invalidation_processor_param_ptr_->notifyFinishIfNecessary(tmp_edge_wrapper_ptr);

        return;
    }

    void CacheServerBase::partitionRequest_(MessageBase* data_requeset_ptr)
    {
        assert(data_requeset_ptr != NULL);

        // TMPDEBUG24
        int64_t tmp_debug_int = -69916166;
        if (MessageBase::getKeyFromMessage(data_requeset_ptr) == Key(std::string((const char*)&tmp_debug_int, sizeof(int64_t))))
        {
            Util::dumpNormalMsg(base_instance_name_, "receive a local request for key -69916166");
        }

        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();
        const uint32_t percacheserver_workercnt = tmp_edge_wrapper_ptr->getPercacheserverWorkercnt();

        if (data_requeset_ptr->isLocalDataRequest()) // Local data requests
        {
            // Calculate the corresponding cache server worker index by hashing
            assert(cache_server_worker_params_.size() == percacheserver_workercnt);
            Key tmp_key = MessageBase::getKeyFromMessage(data_requeset_ptr);
            uint32_t local_cache_server_worker_idx = hash_wrapper_ptr_->hash(tmp_key) % percacheserver_workercnt;
            assert(local_cache_server_worker_idx < percacheserver_workercnt);

            // Pass cache server item into ring buffer of the corresponding cache server worker
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_worker_params_[local_cache_server_worker_idx].getDataRequestBufferPtr()->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->isRedirectedDataRequest()) // Redirected data requests
        {
            // Pass cache server item into ring buffer of the cache server redirection processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_redirection_processor_param_ptr_->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest) // Lazy victim fetching
        {
            // Pass cache server item into ring buffer of the cache server victim fetch processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_victim_fetch_processor_param_ptr_->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredBgplacePlacementNotifyRequest || data_requeset_ptr->getMessageType() == MessageType::kBestGuessBgplacePlacementNotifyRequest) // Placement notification
        {
            // Pass cache server item into ring buffer of the cache server placement processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_placement_processor_param_ptr_->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredMetadataUpdateRequest) // Beacon-based cached metadata update
        {
            // Pass cache server item into ring buffer of the cache server metadata update processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_metadata_update_processor_param_ptr_->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kInvalidationRequest || data_requeset_ptr->getMessageType() == MessageType::kCoveredInvalidationRequest || data_requeset_ptr->getMessageType() == MessageType::kBestGuessInvalidationRequest) // Cache invalidation for MSI protocol
        {
            // Pass cache server item into ring buffer of the cache server invalidation processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_invalidation_processor_param_ptr_->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(data_requeset_ptr->getMessageType()) << " for partitionRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // (1) For local edge cache admission and remote directory admission

    bool CacheServerBase::admitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list,const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec issue_directory_update_req_start_timestamp = Util::getCurrentTimespec();

        // Get destination address of beacon node
        const uint32_t beacon_edge_idx = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(beacon_edge_idx);

        while (true) // Timeout-and-retry mechanism
        {
            // Prepare directory update request to check directory information in beacon node
            MessageBase* directory_update_request_ptr = getReqToAdmitBeaconDirectory_(key, directory_info, source_addr, skip_propagation_latency, is_background);
            assert(directory_update_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: directory_update_request_ptr will be released by edge-to-edge propagation simulator
            directory_update_request_ptr = NULL;

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    std::ostringstream oss;
                    oss << "edge timeout to wait for DirectoryUpdateResponse for key " << key.getKeyDebugstr() << " from beacon " << beacon_edge_idx;
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                // Update is_neighbor_cached based on remote directory admission response
                processRspToAdmitBeaconDirectory_(control_response_ptr, is_being_written, is_neighbor_cached, is_background); // NOTE: is_being_written is updated here

                // Update total bandwidth usage for received directory update (admission) response
                BandwidthUsage directory_update_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_directory_update_rsp_bandwidth_bytes = control_response_ptr->getMsgBandwidthSize();
                directory_update_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_update_rsp_bandwidth_bytes, 0, 0, 1, 0));
                total_bandwidth_usage.update(directory_update_response_bandwidth_usage);

                // Add events of intermediate response if with evet tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;
                break;
            } // End of (is_timeout == false)
        } // End of while(true)

        // Add intermediate event if with evet tracking
        struct timespec issue_directory_update_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_directory_update_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_directory_update_req_end_timestamp, issue_directory_update_req_start_timestamp));
        if (is_background)
        {
            event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_UPDATE_DIRECTORY_TO_ADMIT_EVENT_NAME, issue_directory_update_req_latency_us);
        }
        else
        {
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_UPDATE_REQ_EVENT_NAME, issue_directory_update_req_latency_us);
        }

        return is_finish;
    }

    // (2) For blocking-based cache eviction and local/remote directory eviction

    bool CacheServerBase::evictForCapacity_(const Key& key, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        // Acquire a write lock for atomicity of eviction among different edge cache server workers
        std::string context_name = "CacheServerBase::evictForCapacity_()";
        rwlock_for_eviction_ptr_->acquire_lock(context_name);

        bool is_finish = false;
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();

        // Evict victims from local edge cache and also update cache size usage
        std::unordered_map<Key, Value, KeyHasher> total_victims;
        total_victims.clear();
        while (true) // Evict until used bytes <= capacity bytes
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint64_t used_bytes = tmp_edge_wrapper_ptr->getSizeForCapacity();
            uint64_t capacity_bytes = tmp_edge_wrapper_ptr->getCapacityBytes();
            if (used_bytes <= capacity_bytes) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                std::unordered_map<Key, Value, KeyHasher> tmp_victims;
                tmp_victims.clear();

                // NOTE: we calculate required size after locking to avoid duplicate evictions for the same part of required size
                uint64_t required_size = used_bytes - capacity_bytes;

                // Evict unpopular objects from local edge cache for cache capacity
                evictLocalEdgeCache_(tmp_victims, required_size);

                total_victims.insert(tmp_victims.begin(), tmp_victims.end());

                continue;
            }
        }

        // NOTE: we can release writelock here as cache size usage has already been updated after evicting local edge cache
        rwlock_for_eviction_ptr_->unlock(context_name);

        #ifdef DEBUG_CACHE_SERVER_BASE
        if (total_victims.size() > 0)
        {
            std::ostringstream oss;
            oss << "evict " << total_victims.size() << " victims for key " << key.getKeyDebugstr() << "(beacon node: " << edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key) << ") in local edge " << edge_wrapper_ptr_->getNodeIdx();
            uint32_t i = 0;
            for (std::unordered_map<Key, Value, KeyHasher>::const_iterator total_victims_const_iter = total_victims.begin(); total_victims_const_iter != total_victims.end(); total_victims_const_iter++)
            {
                oss << "[" << i << "] victim_key " << total_victims_const_iter->first.getKeystr() << " valuesize " << total_victims_const_iter->second.getValuesize();
            }
            Util::dumpDebugMsg(base_instance_name_, oss.str());
        }
        #endif

        // Perform directory updates for all evicted victims in parallel
        struct timespec update_directory_to_evict_start_timestamp = Util::getCurrentTimespec();
        is_finish = parallelEvictDirectory_(total_victims, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
        struct timespec update_directory_to_evict_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_directory_to_evict_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_directory_to_evict_end_timestamp, update_directory_to_evict_start_timestamp));
        if (!is_background)
        {
            event_list.addEvent(Event::EDGE_CACHE_SERVER_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME, update_directory_to_evict_latency_us); // Add intermediate event if with event tracking
        }
        else
        {
            event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_UPDATE_DIRECTORY_TO_EVICT_EVENT_NAME, update_directory_to_evict_latency_us); // Add intermediate event if with event tracking
        }

        return is_finish;
    }

    bool CacheServerBase::parallelEvictDirectory_(const std::unordered_map<Key, Value, KeyHasher>& total_victims, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        assert(recvrsp_socket_server_ptr != NULL);

        bool is_finish = false;
        EdgeWrapperBase* tmp_edge_wrapper_ptr = edge_component_ptr_->getEdgeWrapperPtr();

        // Track whether all keys have received directory update responses
        uint32_t acked_cnt = 0;
        std::unordered_map<Key, std::pair<bool, uint32_t>, KeyHasher> acked_flags; // bool refers to whether ACK has been received, while uint32_t refers to beacon edge index for debugging if with timeout
        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator victim_iter = total_victims.begin(); victim_iter != total_victims.end(); victim_iter++)
        {
            const uint32_t tmp_victim_beacon_edge_idx = tmp_edge_wrapper_ptr->getCooperationWrapperPtr()->getBeaconEdgeIdx(victim_iter->first);
            acked_flags.insert(std::pair(victim_iter->first, std::pair(false, tmp_victim_beacon_edge_idx)));
        }

        // Issue multiple directory update requests with is_admit = false simultaneously
        const uint32_t total_victim_cnt = total_victims.size();
        const DirectoryInfo directory_info(tmp_edge_wrapper_ptr->getNodeIdx());
        bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
        while (acked_cnt != total_victim_cnt)
        {
            // Send (total_victim_cnt - acked_cnt) directory update requests to the beacon nodes that have not acknowledged
            for (std::unordered_map<Key, std::pair<bool, uint32_t>, KeyHasher>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second.first) // Skip the key that has received directory update response
                {
                    continue;
                }

                const Key& tmp_victim_key = iter_for_request->first; // key that has NOT received any directory update response
                const uint32_t& tmp_victim_beacon_edge_idx = iter_for_request->second.second;
                const Value& tmp_victim_value = total_victims.find(tmp_victim_key)->second; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
                bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon(tmp_victim_key);
                if (current_is_beacon) // Evict local directory info for the victim key
                {
                    is_finish = evictLocalDirectory_(tmp_victim_key, tmp_victim_value, directory_info, _unused_is_being_written, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
                    if (is_finish)
                    {
                        return is_finish;
                    }

                    // Update ack information
                    assert(!acked_flags[tmp_victim_key].first);
                    acked_flags[tmp_victim_key].first = true;
                    acked_cnt += 1;
                }
                else // Send directory update req with is_admit = false to evict remote directory info for the victim key
                {
                    MessageBase* directory_update_request_ptr = getReqToEvictBeaconDirectory_(tmp_victim_key, directory_info, source_addr, skip_propagation_latency, is_background);
                    assert(directory_update_request_ptr != NULL);

                    // Push the control request into edge-to-edge propagation simulator to the beacon node
                    NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->getBeaconDstaddr_(tmp_victim_beacon_edge_idx);
                    bool is_successful = tmp_edge_wrapper_ptr->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
                    assert(is_successful);

                    // NOTE: directory_update_request_ptr will be released by edge-to-edge propagation simulator
                    directory_update_request_ptr = NULL;
                }

                #ifdef DEBUG_CACHE_SERVER_BASE
                Util::dumpVariablesForDebug(base_instance_name_, 7, "eviction;", "keystr:", tmp_victim_key.getKeystr().c_str(), "is value deleted:", Util::toString(tmp_victim_value.isDeleted()).c_str(), "value size:", Util::toString(tmp_victim_value.getValuesize()).c_str());
                #endif
            } // End of iter_for_request

            // Receive (total_victim_cnt - acked_cnt) directory update repsonses from the beacon edge nodes
            // NOTE: acked_cnt has already been increased for local diretory eviction
            const uint32_t expected_rspcnt = total_victim_cnt - acked_cnt;
            for (uint32_t keyidx_for_response = 0; keyidx_for_response < expected_rspcnt; keyidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    if (!tmp_edge_wrapper_ptr->isNodeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for DirectoryUpdateResponse from " + Util::getAckedStatusStr(acked_flags, "beacon"));
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the diretory update response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);

                    // Mark the key has been acknowledged with DirectoryUpdateResponse
                    const Key tmp_received_key = MessageBase::getKeyFromMessage(control_response_ptr);
                    bool is_match = false;
                    for (std::unordered_map<Key, std::pair<bool, uint32_t>, KeyHasher>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_received_key) // Match an un-acked key
                        {
                            assert(iter_for_response->second.first == false); // Original ack flag should be false

                            // Update total bandwidth usage for received directory update (eviction) repsonse
                            BandwidthUsage directory_update_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_directory_update_rsp_bandwidth_bytes = control_response_ptr->getMsgBandwidthSize();
                            directory_update_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_update_rsp_bandwidth_bytes, 0, 0, 1, 0));
                            total_bandwidth_usage.update(directory_update_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second.first = true;
                            acked_cnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive DirectoryUpdateResponse from edge node " << control_response_ptr->getSourceIndex() << " for key " << tmp_received_key.getKeystr() << ", which is NOT in the victim list!";
                        Util::dumpWarnMsg(base_instance_name_, oss.str());
                    } 
                    else // Process received correct directory eviction response if key matches
                    {
                        std::unordered_map<Key, Value, KeyHasher>::const_iterator total_victims_const_iter = total_victims.find(tmp_received_key);
                        assert(total_victims_const_iter != total_victims.end());
                        const Value tmp_victim_value = total_victims_const_iter->second;
                        is_finish = processRspToEvictBeaconDirectory_(control_response_ptr, tmp_victim_value, _unused_is_being_written, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
                        if (is_finish)
                        {
                            return is_finish; // Edge is NOT running
                        }
                    } // End of !is_match

                    // Release the control response message
                    delete control_response_ptr;
                    control_response_ptr = NULL;
                } // End of (is_timeout == false)
            } // End of edgeidx_for_response

            if (is_finish) // Edge node is NOT running now
            {
                break;
            }
        } // End of (acked_cnt != total_victim_cnt)

        return is_finish;
    }

    void CacheServerBase::checkPointers_() const
    {
        assert(edge_component_ptr_ != NULL);
        assert(edge_component_ptr_->getEdgeWrapperPtr() != NULL);
        assert(hash_wrapper_ptr_ != NULL);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        assert(rwlock_for_eviction_ptr_ != NULL);

        return;
    }
}
#include "edge/cache_server/cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "edge/cache_server/cache_server_metadata_update_processor.h"
#include "edge/cache_server/cache_server_placement_processor.h"
#include "edge/cache_server/cache_server_redirection_processor.h"
#include "edge/cache_server/cache_server_victim_fetch_processor.h"
#include "edge/cache_server/cache_server_worker_base.h"
#include "message/control_message.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CacheServer::kClassName("CacheServer");

    void* CacheServer::launchCacheServer(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        CacheServer cache_server = CacheServer((EdgeWrapper*)edge_wrapper_ptr);
        cache_server.start();

        pthread_exit(NULL);
        return NULL;
    }

    CacheServer::CacheServer(EdgeWrapper* edge_wrapper_ptr) : edge_wrapper_ptr_(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);
        uint32_t edge_idx = edge_wrapper_ptr->getNodeIdx();
        uint32_t edgecnt = edge_wrapper_ptr->getNodeCnt();

        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        // Allocate hash wrapper for partition
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapperByHashName(Util::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);

        // Prepare parameters for cache server threads
        const uint32_t percacheserver_workercnt = edge_wrapper_ptr->getPercacheserverWorkercnt();
        cache_server_worker_params_.resize(percacheserver_workercnt);
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            CacheServerWorkerParam tmp_cache_server_worker_param(this, local_cache_server_worker_idx, Config::getEdgeCacheServerDataRequestBufferSize());
            cache_server_worker_params_[local_cache_server_worker_idx] = tmp_cache_server_worker_param;
        }
        
        // Prepare parameters for cache server metadata update processor thread
        cache_server_metadata_update_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize());

        // Prepare parameters for cache server victim fetch processor thread
        cache_server_victim_fetch_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize());
        assert(cache_server_victim_fetch_processor_param_ptr_ != NULL);

        // Prepare parameters for cache server redirection processor thread
        cache_server_redirection_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize());
        assert(cache_server_redirection_processor_param_ptr_ != NULL);

        // Prepare parameters for cache server placement processor thread
        cache_server_placement_processor_param_ptr_ = new CacheServerProcessorParam(this, Config::getEdgeCacheServerDataRequestBufferSize());
        assert(cache_server_placement_processor_param_ptr_ != NULL);

        // For receiving local requests

        // Get source address of cache server to receive local requests
        const bool is_launch_edge = true; // The edge cache server belongs to the logical edge node launched in the current physical machine
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_launch_edge);
        uint16_t edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(edge_idx, edgecnt);
        edge_cache_server_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_recvreq_port);

        // Prepare a socket server on recvreq port for cache server
        NetworkAddr host_addr(Util::ANY_IPSTR, edge_cache_server_recvreq_port);
        edge_cache_server_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_eviction_ptr_";
        rwlock_for_eviction_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_eviction_ptr_ != NULL);
    }

    CacheServer::~CacheServer()
    {
        // No need to release edge_wrapper_ptr_, which is performed outside CacheServer

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

        // Release the socket server on recvreq port
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_recvreq_socket_server_ptr_;
        edge_cache_server_recvreq_socket_server_ptr_ = NULL;

        assert(rwlock_for_eviction_ptr_ != NULL);
        delete rwlock_for_eviction_ptr_;
        rwlock_for_eviction_ptr_ = NULL;
    }

    void CacheServer::start()
    {
        checkPointers_();
        
        const uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const uint32_t percacheserver_workercnt = edge_wrapper_ptr_->getPercacheserverWorkercnt();

        int pthread_returncode;
        pthread_t cache_server_worker_threads[percacheserver_workercnt];
        pthread_t cache_server_victim_fetch_processor_thread;
        pthread_t cache_server_redirection_processor_thread;
        pthread_t cache_server_placement_processor_thread;
        pthread_t cache_server_metadata_update_processor_thread;

        // Launch cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            //pthread_returncode = pthread_create(&cache_server_worker_threads[local_cache_server_worker_idx], NULL, CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            // if (pthread_returncode != 0)
            // {
            //     std::ostringstream oss;
            //     oss << "edge " << edge_idx << " failed to launch cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
            //     covered::Util::dumpErrorMsg(instance_name_, oss.str());
            //     exit(1);
            // }
            std::string tmp_thread_name = "edge-cache-server-worker-" + std::to_string(edge_idx) + "-" + std::to_string(local_cache_server_worker_idx);
            ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &cache_server_worker_threads[local_cache_server_worker_idx], CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
        }

        // Launch cache server victim fetch processor
        //pthread_returncode = pthread_create(&cache_server_victim_fetch_processor_thread, NULL, CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor, (void*)(cache_server_victim_fetch_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server victim fetch processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "edge-cache-server-victim-fetch-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_victim_fetch_processor_thread, CacheServerVictimFetchProcessor::launchCacheServerVictimFetchProcessor, (void*)(cache_server_victim_fetch_processor_param_ptr_));

        // Launch cache server redirection processor
        //pthread_returncode = pthread_create(&cache_server_redirection_processor_thread, NULL, CacheServerRedirectionProcessor::launchCacheServerRedirectionProcessor, (void*)(cache_server_redirection_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server redirection processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-redirection-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &cache_server_redirection_processor_thread, CacheServerRedirectionProcessor::launchCacheServerRedirectionProcessor, (void*)(cache_server_redirection_processor_param_ptr_));

        // Launch cache server placement processor
        //pthread_returncode = pthread_create(&cache_server_placement_processor_thread, NULL, CacheServerPlacementProcessor::launchCacheServerPlacementProcessor, (void*)(cache_server_placement_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-placement-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &cache_server_placement_processor_thread, CacheServerPlacementProcessor::launchCacheServerPlacementProcessor, (void*)(cache_server_placement_processor_param_ptr_));

        // Launch cache server metadata update processor
        //pthread_returncode = pthread_create(&cache_server_metadata_update_processor_thread, NULL, CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor, (void*)(cache_server_metadata_update_processor_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << edge_idx << " failed to launch cache server metadata update processor (error code: " << pthread_returncode << ")" << std::endl;
        //     covered::Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-metadata-update-processor-" + std::to_string(edge_idx);
        ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &cache_server_metadata_update_processor_thread, CacheServerMetadataUpdateProcessor::launchCacheServerMetadataUpdateProcessor, (void*)(cache_server_metadata_update_processor_param_ptr_));

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
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Wait for cache server victim fetch processor
        pthread_returncode = pthread_join(cache_server_victim_fetch_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server victim fetch processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server redirection processor
        pthread_returncode = pthread_join(cache_server_redirection_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server redirection processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server placement processor
        pthread_returncode = pthread_join(cache_server_placement_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server metadata update processor
        pthread_returncode = pthread_join(cache_server_metadata_update_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server metadata update processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    EdgeWrapper* CacheServer::getEdgeWrapperPtr() const
    {
        assert(edge_wrapper_ptr_ != NULL);
        return edge_wrapper_ptr_;
    }

    NetworkAddr CacheServer::getEdgeCacheServerRecvreqSourceAddr() const
    {
        return edge_cache_server_recvreq_source_addr_;
    }

    void CacheServer::receiveRequestsAndPartition_()
    {
        checkPointers_();

        while (edge_wrapper_ptr_->isNodeRunning()) // edge_running_ is set as true by default
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
                if (data_request_ptr->isDataRequest() || message_type == MessageType::kCoveredVictimFetchRequest || message_type == MessageType::kCoveredPlacementNotifyRequest || message_type == MessageType::kCoveredMetadataUpdateRequest) // Local data requests for cache server workers; redirected data requests for cache server redirection processor; lazy victim fetching for cache server victim fetch processor; placement notification for cache server placement processor; metadata update requests for cache server metadata update processor
                {
                    // Pass received request and network address to corresponding cache server worker/processor by ring buffer
                    // NOTE: received request will be released by the corresponding cache server worker/processor
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
        assert(data_requeset_ptr != NULL);

        const uint32_t percacheserver_workercnt = edge_wrapper_ptr_->getPercacheserverWorkercnt();

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
            bool is_successful = cache_server_redirection_processor_param_ptr_->getCacheServerItemBufferPtr()->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredVictimFetchRequest) // Lazy victim fetching
        {
            // Pass cache server item into ring buffer of the cache server victim fetch processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_victim_fetch_processor_param_ptr_->getCacheServerItemBufferPtr()->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredPlacementNotifyRequest) // Placement notification
        {
            // Pass cache server item into ring buffer of the cache server placement processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_placement_processor_param_ptr_->getCacheServerItemBufferPtr()->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else if (data_requeset_ptr->getMessageType() == MessageType::kCoveredMetadataUpdateRequest) // Beacon-based cached metadata update
        {
            // Pass cache server item into ring buffer of the cache server metadata update processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_metadata_update_processor_param_ptr_->getCacheServerItemBufferPtr()->push(tmp_cache_server_item);
            assert(is_successful == true); // Ring buffer must NOT be full
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(data_requeset_ptr->getMessageType()) << " for partitionRequest_()!";
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // (1) For local edge cache admission and remote directory admission

    void CacheServer::admitLocalEdgeCache_(const Key& key, const Value& value, const bool& is_neighbor_cached, const bool& is_valid) const
    {
        checkPointers_();

        bool affect_victim_tracker = false; // If key is a local synced victim now
        const uint32_t beacon_edgeidx = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        edge_wrapper_ptr_->getEdgeCachePtr()->admit(key, value, is_neighbor_cached, is_valid, beacon_edgeidx, affect_victim_tracker);

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
            edge_wrapper_ptr_->updateCacheManagerForLocalSyncedVictims(affect_victim_tracker);

            // Remove existing cached directory if any as key is local cached, while we ONLY cache valid remote dirinfo for local uncached objects
            edge_wrapper_ptr_->getCoveredCacheManagerPtr()->updateDirectoryCacherToRemoveCachedDirectory(key);
        }

        return;
    }

    bool CacheServer::admitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list,const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = edge_wrapper_ptr_->currentIsBeacon(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec issue_directory_update_req_start_timestamp = Util::getCurrentTimespec();

        // Get destination address of beacon node
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = edge_wrapper_ptr_->getBeaconDstaddr_(key);

        while (true) // Timeout-and-retry mechanism
        {
            // Prepare directory update request to check directory information in beacon node
            MessageBase* directory_update_request_ptr = getReqToAdmitBeaconDirectory_(key, directory_info, source_addr, skip_propagation_latency, is_background);
            assert(directory_update_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: directory_update_request_ptr will be released by edge-to-edge propagation simulator
            directory_update_request_ptr = NULL;

            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_wrapper_ptr_->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    std::ostringstream oss;
                    oss << "edge timeout to wait for DirectoryUpdateResponse for key " << key.getKeystr();
                    Util::dumpWarnMsg(instance_name_, oss.str());
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

                // Update total bandwidth usage for received directory update response
                BandwidthUsage directory_update_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_directory_update_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                directory_update_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_update_rsp_bandwidth_bytes, 0));
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
        {}
        else
        {
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_UPDATE_REQ_EVENT_NAME, issue_directory_update_req_latency_us);
        }

        return is_finish;
    }

    MessageBase* CacheServer::getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        const bool is_admit = true; // Try to admit a new key as local cached object (NOTE: local edge cache has NOT been admitted yet)
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);

        MessageBase* directory_update_request_ptr = NULL;
        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

            // Prepare victim syncset for piggybacking-based victim synchronization
            VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

            // ONLY need victim synchronization yet without popularity collection/aggregation
            if (!is_background) // Foreground remote directory admission triggered by hybrid data fetching at the sender/closest edge node (different from beacon)
            {
                // (OBSOLETE) NOTE: For COVERED, although there still exist foreground directory update requests for eviction (triggered by local gets to update invalid value and local puts to update cached value), all directory update requests for admission MUST be background due to non-blocking placement deployment

                // NOTE: For COVERED, both directory eviction (triggered by value update and local/remote placement notification) and directory admission (triggered by only-sender hybrid data fetching, fast-path single placement, and local/remote placement notification) can be foreground and background
                directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, source_addr, skip_propagation_latency);
            }
            else // Background remote directory admission triggered by remote placement notification
            {
                // NOTE: use background event names by sending CoveredPlacementDirectoryUpdateRequest (NOT DISABLE recursive cache placement due to is_admit = true)
                directory_update_request_ptr = new CoveredPlacementDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, source_addr, skip_propagation_latency);
            }
        }
        else // Baselines
        {
            directory_update_request_ptr = new DirectoryUpdateRequest(key, is_admit, directory_info, edge_idx, source_addr, skip_propagation_latency);
        }
        assert(directory_update_request_ptr != NULL);

        return directory_update_request_ptr;
    }

    void CacheServer::processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written, bool& is_neighbor_cached, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

            uint32_t source_edge_idx = control_response_ptr->getSourceIndex();
            VictimSyncset neighbor_victim_syncset;

            // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching, while foreground/background directory admission will NEVER perform placement calculation and hence NO hybrid data fetching
            if (!is_background)
            {
                assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);

                // Get is_being_written and victim syncset from control response message
                const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = covered_directory_update_response_ptr->isBeingWritten();
                is_neighbor_cached = covered_directory_update_response_ptr->isNeighborCached();
                neighbor_victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();
            }
            else
            {
                assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementDirectoryUpdateResponse);

                // Get is_being_written and victim syncset from control response message
                const CoveredPlacementDirectoryUpdateResponse* const covered_placement_directory_update_response_ptr = static_cast<const CoveredPlacementDirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = covered_placement_directory_update_response_ptr->isBeingWritten();
                is_neighbor_cached = covered_placement_directory_update_response_ptr->isNeighborCached();
                neighbor_victim_syncset = covered_placement_directory_update_response_ptr->getVictimSyncsetRef();
            }

            // Victim synchronization
            edge_wrapper_ptr_->updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
        }
        else // Baselines
        {
            assert(control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

            // Get is_being_written from control response message
            const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = directory_update_response_ptr->isBeingWritten();
        }

        return;
    }

    // (2) For blocking-based cache eviction and local/remote directory eviction

    bool CacheServer::evictForCapacity_(const Key& key, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        // Acquire a write lock for atomicity of eviction among different edge cache server workers
        std::string context_name = "CacheServer::evictForCapacity_()";
        rwlock_for_eviction_ptr_->acquire_lock(context_name);

        bool is_finish = false;

        // Evict victims from local edge cache and also update cache size usage
        std::unordered_map<Key, Value, KeyHasher> total_victims;
        total_victims.clear();
        while (true) // Evict until used bytes <= capacity bytes
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint64_t used_bytes = edge_wrapper_ptr_->getSizeForCapacity();
            uint64_t capacity_bytes = edge_wrapper_ptr_->getCapacityBytes();
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

        #ifdef DEBUG_CACHE_SERVER
        if (total_victims.size() > 0)
        {
            std::ostringstream oss;
            oss << "evict " << total_victims.size() << " victims for key " << key.getKeystr() << "(beacon node: " << edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key) << ") in local edge " << edge_wrapper_ptr_->getNodeIdx();
            uint32_t i = 0;
            for (std::unordered_map<Key, Value, KeyHasher>::const_iterator total_victims_const_iter = total_victims.begin(); total_victims_const_iter != total_victims.end(); total_victims_const_iter++)
            {
                oss << "[" << i << "] victim_key " << total_victims_const_iter->first.getKeystr() << " valuesize " << total_victims_const_iter->second.getValuesize();
            }
            Util::dumpDebugMsg(instance_name_, oss.str());
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

    void CacheServer::evictLocalEdgeCache_(std::unordered_map<Key, Value, KeyHasher>& victims, const uint64_t& required_size) const
    {
        checkPointers_();

        edge_wrapper_ptr_->getEdgeCachePtr()->evict(victims, required_size);

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // NOTE: eviction MUST affect victim tracker due to evicting objects with least local rewards (i.e., local synced victims)
            const bool affect_victim_tracker = true;
            edge_wrapper_ptr_->updateCacheManagerForLocalSyncedVictims(affect_victim_tracker);
        }

        return;
    }

    bool CacheServer::parallelEvictDirectory_(const std::unordered_map<Key, Value, KeyHasher>& total_victims, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        assert(recvrsp_socket_server_ptr != NULL);

        bool is_finish = false;

        // Track whether all keys have received directory update responses
        uint32_t acked_cnt = 0;
        std::unordered_map<Key, bool, KeyHasher> acked_flags;
        for (std::unordered_map<Key, Value, KeyHasher>::const_iterator victim_iter = total_victims.begin(); victim_iter != total_victims.end(); victim_iter++)
        {
            acked_flags.insert(std::pair(victim_iter->first, false));
        }

        // Issue multiple directory update requests with is_admit = false simultaneously
        const uint32_t total_victim_cnt = total_victims.size();
        const DirectoryInfo directory_info(edge_wrapper_ptr_->getNodeIdx());
        bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
        while (acked_cnt != total_victim_cnt)
        {
            // Send (total_victim_cnt - acked_cnt) directory update requests to the beacon nodes that have not acknowledged
            for (std::unordered_map<Key, bool, KeyHasher>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second) // Skip the key that has received directory update response
                {
                    continue;
                }

                const Key& tmp_victim_key = iter_for_request->first; // key that has NOT received any directory update response
                const Value& tmp_victim_value = total_victims.find(tmp_victim_key)->second; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
                bool current_is_beacon = edge_wrapper_ptr_->currentIsBeacon(tmp_victim_key);
                if (current_is_beacon) // Evict local directory info for the victim key
                {
                    is_finish = evictLocalDirectory_(tmp_victim_key, tmp_victim_value, directory_info, _unused_is_being_written, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
                    if (is_finish)
                    {
                        return is_finish;
                    }

                    // Update ack information
                    assert(!acked_flags[tmp_victim_key]);
                    acked_flags[tmp_victim_key] = true;
                    acked_cnt += 1;
                }
                else // Send directory update req with is_admit = false to evict remote directory info for the victim key
                {
                    MessageBase* directory_update_request_ptr = getReqToEvictBeaconDirectory_(tmp_victim_key, directory_info, source_addr, skip_propagation_latency, is_background);
                    assert(directory_update_request_ptr != NULL);

                    // Push the control request into edge-to-edge propagation simulator to the beacon node
                    NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = edge_wrapper_ptr_->getBeaconDstaddr_(tmp_victim_key);
                    bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(directory_update_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
                    assert(is_successful);

                    // NOTE: directory_update_request_ptr will be released by edge-to-edge propagation simulator
                    directory_update_request_ptr = NULL;
                }

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(instance_name_, 7, "eviction;", "keystr:", tmp_victim_key.getKeystr().c_str(), "is value deleted:", Util::toString(tmp_victim_value.isDeleted()).c_str(), "value size:", Util::toString(tmp_victim_value.getValuesize()).c_str());
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
                    if (!edge_wrapper_ptr_->isNodeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryUpdateResponse");
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
                    for (std::unordered_map<Key, bool, KeyHasher>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_received_key) // Match an un-acked key
                        {
                            assert(iter_for_response->second == false); // Original ack flag should be false

                            // Update total bandwidth usage for received directory update repsonse
                            BandwidthUsage directory_update_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_directory_update_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                            directory_update_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_directory_update_rsp_bandwidth_bytes, 0));
                            total_bandwidth_usage.update(directory_update_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second = true;
                            acked_cnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive DirectoryUpdateResponse from edge node " << control_response_ptr->getSourceIndex() << " for key " << tmp_received_key.getKeystr() << ", which is NOT in the victim list!";
                        Util::dumpWarnMsg(instance_name_, oss.str());
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

    bool CacheServer::evictLocalDirectory_(const Key& key, const Value& value, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        CooperationWrapperBase* tmp_cooperation_wrapper_ptr = edge_wrapper_ptr_->getCooperationWrapperPtr();

        bool is_finish = false;

        uint32_t current_edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const bool is_admit = false; // Evict a victim as local uncached object
        bool unused_is_neighbor_cached = false; // NOTE: ONLY need is_neighbor_cached for directory admission to initizalize cached metadata, yet NO need for directory eviction
        MetadataUpdateRequirement metadata_update_requirement;
        bool is_global_cached = tmp_cooperation_wrapper_ptr->updateDirectoryTable(key, current_edge_idx, is_admit, directory_info, is_being_written, unused_is_neighbor_cached, metadata_update_requirement);
        UNUSED(unused_is_neighbor_cached);

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation -> while here NOT need piggyacking-based popularity collection and victim synchronization for local directory update

            // Prepare local uncached popularity of key for popularity aggregation
            CollectedPopularity collected_popularity;
            edge_wrapper_ptr_->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata

            // Issue metadata update request if necessary, update victim dirinfo, assert NO local uncached popularity, and perform selective popularity aggregation after local directory eviction
            Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
            bool need_hybrid_fetching = false;
            is_finish = edge_wrapper_ptr_->afterDirectoryEvictionHelper_(key, current_edge_idx, metadata_update_requirement, directory_info, collected_popularity, is_global_cached, best_placement_edgeset, need_hybrid_fetching, recvrsp_socket_server_ptr, source_addr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }

            // Trigger non-blocking placement notification if need hybrid fetching for non-blocking data fetching (ONLY for COVERED)
            if (need_hybrid_fetching)
            {
                assert(!is_background); // Must be foreground local directory eviction (triggered by invalid/valid value update by local get/put and independent admission)

                assert(edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME);
                edge_wrapper_ptr_->nonblockNotifyForPlacement(key, value, best_placement_edgeset, skip_propagation_latency);
            }
        }

        return is_finish;
    }

    MessageBase* CacheServer::getReqToEvictBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();

        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const bool is_admit = false; // Evict a victim as local uncached object (NOTE: local edge cache has already been evicted)
        MessageBase* directory_update_request_ptr = NULL;

        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t dst_beacon_edge_idx_for_compression = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != edge_idx);
    
        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

            // Prepare local uncached popularity of key for piggybacking-based popularity collection
            CollectedPopularity collected_popularity;
            edge_wrapper_ptr_->getEdgeCachePtr()->getCollectedPopularity(key, collected_popularity); // collected_popularity.is_tracked_ indicates if the local uncached key is tracked in local uncached metadata (due to selective metadata preservation)

            // Prepare victim syncset for piggybacking-based victim synchronization
            VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

            // Need BOTH popularity collection and victim synchronization
            if (!is_background) // Foreground remote directory eviction (triggered by invalid/valid value update by local get/put and independent admission)
            {
                directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, collected_popularity, victim_syncset, edge_idx, source_addr, skip_propagation_latency);
            }
            else // Background remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
            {
                // NOTE: use background event names and DISABLE recursive cache placement by sending CoveredPlacementDirectoryUpdateRequest
                directory_update_request_ptr = new CoveredPlacementDirectoryUpdateRequest(key, is_admit, directory_info, collected_popularity, victim_syncset, edge_idx, source_addr, skip_propagation_latency);
            }

            // NOTE: key MUST NOT have any cached directory, as key is local cached before eviction (even if key may be local uncached and tracked by local uncached metadata due to metadata preservation after eviction, we have NOT lookuped remote directory yet from beacon node)
            CachedDirectory cached_directory;
            bool has_cached_directory = tmp_covered_cache_manager_ptr->accessDirectoryCacherForCachedDirectory(key, cached_directory);
            assert(!has_cached_directory);
            UNUSED(cached_directory);
        }
        else // Baselines
        {
            directory_update_request_ptr = new DirectoryUpdateRequest(key, is_admit, directory_info, edge_idx, source_addr, skip_propagation_latency);
        }

        assert(directory_update_request_ptr != NULL);
        return directory_update_request_ptr;
    }

    bool CacheServer::processRspToEvictBeaconDirectory_(MessageBase* control_response_ptr, const Value& value, bool& is_being_written, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        bool is_finish = false;

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();
            
            // Victim synchronization
            const KeyByteVictimsetMessage* directory_update_response_ptr = static_cast<const KeyByteVictimsetMessage*>(control_response_ptr);
            const uint32_t source_edge_idx = directory_update_response_ptr->getSourceIndex();
            const VictimSyncset& neighbor_victim_syncset = 
            directory_update_response_ptr->getVictimSyncsetRef();
            edge_wrapper_ptr_->updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
            
            if (!is_background) // Foreground remote directory eviction (triggered by invalid/valid value update by local get/put and independent admission)
            {
                // NOTE: ONLY foreground directory eviction could trigger hybrid data fetching
                const MessageType message_type = control_response_ptr->getMessageType();
                //assert(message_type == MessageType::kCoveredDirectoryUpdateResponse || message_type == MessageType::kCoveredPlacementDirectoryEvictResponse);
                if (message_type == MessageType::kCoveredDirectoryUpdateResponse) // Normal directory eviction response
                {
                    // Get is_being_written (UNUSED) from control response message
                    const CoveredDirectoryUpdateResponse* covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                    is_being_written = covered_directory_update_response_ptr->isBeingWritten();
                }
                else if (message_type == MessageType::kCoveredPlacementDirectoryEvictResponse) // Directory eviction response with hybrid data fetching
                {
                    const CoveredPlacementDirectoryEvictResponse* const covered_placement_directory_evict_response_ptr = static_cast<const CoveredPlacementDirectoryEvictResponse*>(control_response_ptr);

                    // Get is_being_written (UNUSED) and best placement edgeset from control response message
                    is_being_written = covered_placement_directory_evict_response_ptr->isBeingWritten();
                    Edgeset best_placement_edgeset = covered_placement_directory_evict_response_ptr->getEdgesetRef();

                    // Trigger placement notification remotely at the beacon edge node
                    const Key tmp_key = covered_placement_directory_evict_response_ptr->getKey();
                    is_finish = notifyBeaconForPlacementAfterHybridFetch_(tmp_key, value, best_placement_edgeset, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency);
                    if (is_finish)
                    {
                        return is_finish;
                    }
                }
                else
                {
                    std::ostringstream oss;
                    oss << "Invalid message type " << MessageBase::messageTypeToString(message_type) << " for processRspToEvictBeaconDirectory_()";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }
            }
            else // Background remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
            {
                assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementDirectoryUpdateResponse);

                // Get is_being_written (UNUSED) from control response message
                const CoveredPlacementDirectoryUpdateResponse* covered_placement_directory_update_response_ptr = static_cast<const CoveredPlacementDirectoryUpdateResponse*>(control_response_ptr);
                is_being_written = covered_placement_directory_update_response_ptr->isBeingWritten();
            }
        }
        else // Baselines
        {
            assert(control_response_ptr->getMessageType() == MessageType::kDirectoryUpdateResponse);

            // Get is_being_written (UNUSED) from control response message
            const DirectoryUpdateResponse* const directory_update_response_ptr = static_cast<const DirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = directory_update_response_ptr->isBeingWritten();

            UNUSED(recvrsp_source_addr);
            UNUSED(recvrsp_socket_server_ptr);
            UNUSED(total_bandwidth_usage);
            UNUSED(event_list);
            UNUSED(skip_propagation_latency);
        }

        return is_finish;
    }

    // (3) Trigger non-blocking placement notification (ONLY for COVERED)

    bool CacheServer::notifyBeaconForPlacementAfterHybridFetch_(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& recvrsp_source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        Edgeset tmp_best_placement_edgest = best_placement_edgeset; // Deep copy for excluding sender if with local placement

        checkPointers_();
        CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();
        
        // NOTE: current edge node MUST NOT be the beacon edge node for the given key
        const uint32_t current_edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const uint32_t dst_beacon_edge_idx_for_compression = edge_wrapper_ptr_->getCooperationWrapperPtr()->getBeaconEdgeIdx(key);
        assert(dst_beacon_edge_idx_for_compression != current_edge_idx); 

        bool is_finish = false;

        // Check if current edge node needs placement
        std::unordered_set<uint32_t>::const_iterator tmp_best_placement_edgeset_const_iter = tmp_best_placement_edgest.find(current_edge_idx);
        bool current_need_placement = false;
        bool current_is_only_placement = false;
        if (tmp_best_placement_edgeset_const_iter != tmp_best_placement_edgest.end())
        {
            current_need_placement = true;
            tmp_best_placement_edgest.erase(tmp_best_placement_edgeset_const_iter); // NOTE: we exclude current edge idx from best placement edgeset
            if (tmp_best_placement_edgest.size() == 0) // Current edge node is the only placement
            {
                current_is_only_placement = true;
            }
        }

        // Prepare destination address of beacon server
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = edge_wrapper_ptr_->getBeaconDstaddr_(dst_beacon_edge_idx_for_compression);

        // Prepare victim syncset for piggybacking-based victim synchronization
        VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForLocalVictimSyncset(dst_beacon_edge_idx_for_compression, edge_wrapper_ptr_->getCacheMarginBytes());

        // Notify result of hybrid data fetching towards the beacon edge node to trigger non-blocking placement notification
        bool is_being_written = false;
        VictimSyncset received_beacon_victim_syncset; // For victim synchronization
        // NOTE: for only-/including-sender hybrid fetching, ONLY consider remote directory lookup/eviction and release writelock instead of local ones
        // -> (i) Remote ones NEVER use coooperation wrapper to get is_neighbor_cached, as sender is NOT beacon here
        // -> (ii) Local ones always need "hybrid fetching" to get value and trigger normal placement (sender MUST be beacon and can get is_neighbor_cached by cooperation wrapper when admiting local directory)
        bool is_neighbor_cached = false; // ONLY used if current_need_placement
        while (true) // Timeout-and-retry mechanism
        {
            // Prepare control request message to notify the beacon edge node
            MessageBase* control_request_ptr = NULL;
            if (!current_need_placement) // Current edge node does NOT need placement
            {
                // Prepare CoveredPlacementHybridFetchedRequest
                control_request_ptr = new CoveredPlacementHybridFetchedRequest(key, value, victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
            }
            else
            {
                // NOTE: we cannot optimistically admit valid object into local edge cache first before issuing dirinfo admission request, as clients may get incorrect value if key is being written
                if (!current_is_only_placement) // Current edge node is NOT the only placement
                {
                    // Prepare CoveredPlacementDirectoryAdmitRequest (also equivalent to directory admission request)
                    control_request_ptr = new CoveredPlacementDirectoryAdmitRequest(key, value, DirectoryInfo(current_edge_idx), victim_syncset, tmp_best_placement_edgest, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
                }
                else // Current edge node is the only placement
                {
                    // Prepare CoveredDirectoryUpdateRequest (NOT trigger placement notification; also equivalent to directory admission request)
                    // NOTE: unlike CoveredPlacementDirectoryUpdateRequest, CoveredDirectoryUpdateRequest is a foreground message with foreground events and bandwidth usage
                    const bool is_admit = true;
                    control_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, DirectoryInfo(current_edge_idx), victim_syncset, current_edge_idx, recvrsp_source_addr, skip_propagation_latency);
                }
            }
            assert(control_request_ptr != NULL);

            // Push the control request into edge-to-edge propagation simulator to the beacon node
            bool is_successful = edge_wrapper_ptr_->getEdgeToedgePropagationSimulatorParamPtr()->push(control_request_ptr, beacon_edge_beacon_server_recvreq_dst_addr);
            assert(is_successful);

            // NOTE: control_request_ptr will be released by edge-to-edge propagation simulator
            control_request_ptr = NULL;

            // Wait for the corresponding control response from the beacon edge node
            DynamicArray control_response_msg_payload;
            bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_wrapper_ptr_->isNodeRunning())
                {
                    is_finish = true; // Edge is NOT running
                    break; // Break from while (true)
                }
                else
                {
                    std::ostringstream oss;
                    oss << "edge timeout to wait for foreground directory update response after hybrid data fetching for key " << key.getKeystr();
                    Util::dumpWarnMsg(instance_name_, oss.str());
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                if (!current_need_placement)
                {
                    assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementHybridFetchedResponse);
                    UNUSED(is_being_written); // NOTE: is_being_written will NOT be used as the current edge node (sender) is NOT a placement
                    const CoveredPlacementHybridFetchedResponse* const covered_placement_hybrid_fetched_response_ptr = static_cast<const CoveredPlacementHybridFetchedResponse*>(control_response_ptr);
                    received_beacon_victim_syncset = covered_placement_hybrid_fetched_response_ptr->getVictimSyncsetRef();
                }
                else
                {
                    if (!current_is_only_placement) // Current edge node is NOT the only placement
                    {
                        assert(control_response_ptr->getMessageType() == MessageType::kCoveredPlacementDirectoryAdmitResponse);
                        const CoveredPlacementDirectoryAdmitResponse* const covered_placement_directory_admit_response_ptr = static_cast<const CoveredPlacementDirectoryAdmitResponse*>(control_response_ptr);
                        is_being_written = covered_placement_directory_admit_response_ptr->isBeingWritten(); // Used by local edge cache admission later
                        received_beacon_victim_syncset = covered_placement_directory_admit_response_ptr->getVictimSyncsetRef();

                        is_neighbor_cached = covered_placement_directory_admit_response_ptr->isNeighborCached(); // Get is_neighbor_cached for including-sender hybrid fetching
                    }
                    else // Current edge node is the only placement
                    {
                        assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);
                        const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
                        is_being_written = covered_directory_update_response_ptr->isBeingWritten(); // Used by local edge cache admission later
                        received_beacon_victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();

                        is_neighbor_cached = covered_directory_update_response_ptr->isNeighborCached(); // Get is_neighbor_cached for only-sender hybrid fetching
                    }
                }

                // Victim synchronization
                const uint32_t beacon_edge_idx = control_response_ptr->getSourceIndex();
                edge_wrapper_ptr_->updateCacheManagerForNeighborVictimSyncset(beacon_edge_idx, received_beacon_victim_syncset);

                // Update total bandwidth usage for received control response
                BandwidthUsage control_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                uint32_t cross_edge_control_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                control_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_control_rsp_bandwidth_bytes, 0));
                total_bandwidth_usage.update(control_response_bandwidth_usage);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(control_response_ptr->getEventListRef());

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;

                break; // Break from while (true)
            } // End of (is_timeout == false)
        } // End of while (true)

        if (is_finish)
        {
            return is_finish; // Edge is NOT running now
        }

        // Perform local placement (equivalent an in-advance remote placement notification) if necessary
        if (current_need_placement)
        {
            // (OBSOLETE) NOTE: we do NOT need to notify placement processor of the current sender/closest edge node for local placement, because sender is NOT beacon and waiting for response will NOT block subsequent local/remote placement calculation

            // NOTE: we need to notify placement processor of the current sender/closest edge node for local placement, because we need to use the background directory update requests to DISABLE recursive cache placement and also avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon (similar as EdgeWrapper::nonblockNotifyForPlacement() invoked by local/remote beacon edge node)

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker which may serve subsequent placement calculation if sender is beacon
            const bool is_valid = !is_being_written;
            bool is_successful = edge_wrapper_ptr_->getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, is_neighbor_cached, is_valid, skip_propagation_latency));
            assert(is_successful);
        }

        return is_finish;
    }

    void CacheServer::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);        
        assert(hash_wrapper_ptr_ != NULL);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);
        assert(rwlock_for_eviction_ptr_ != NULL);

        return;
    }
}
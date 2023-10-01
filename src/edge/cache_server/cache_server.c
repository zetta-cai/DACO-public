#include "edge/cache_server/cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "edge/cache_server/cache_server_placement_processor.h"
#include "edge/cache_server/cache_server_worker_base.h"
#include "message/control_message.h"
#include "network/network_addr.h"

namespace covered
{
    const std::string CacheServer::kClassName("CacheServer");

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

        // Prepare parameters for cache server placement processor thread
        CacheServerPlacementProcessorParam tmp_cache_server_placement_processor_param(this, Config::getEdgeCacheServerDataRequestBufferSize());
        cache_server_placement_processor_param_ = tmp_cache_server_placement_processor_param;

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
        
        const uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();
        const uint32_t percacheserver_workercnt = edge_wrapper_ptr_->getPercacheserverWorkercnt();

        int pthread_returncode;
        pthread_t cache_server_worker_threads[percacheserver_workercnt];
        pthread_t cache_server_placement_processor_thread;

        // Launch cache server workers
        for (uint32_t local_cache_server_worker_idx = 0; local_cache_server_worker_idx < percacheserver_workercnt; local_cache_server_worker_idx++)
        {
            //pthread_returncode = pthread_create(&cache_server_worker_threads[local_cache_server_worker_idx], NULL, CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            pthread_returncode = Util::pthreadCreateHighPriority(&cache_server_worker_threads[local_cache_server_worker_idx], CacheServerWorkerBase::launchCacheServerWorker, (void*)(&cache_server_worker_params_[local_cache_server_worker_idx]));
            if (pthread_returncode != 0)
            {
                std::ostringstream oss;
                oss << "edge " << edge_idx << " failed to launch cache server worker " << local_cache_server_worker_idx << " (error code: " << pthread_returncode << ")" << std::endl;
                covered::Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        // Launch cache server placement processor
        //pthread_returncode = pthread_create(&cache_server_placement_processor_thread, NULL, CacheServerPlacementProcessor::launchCacheServerPlacementProcessor, (void*)(&cache_server_placement_processor_param_));
        pthread_returncode = Util::pthreadCreateHighPriority(&cache_server_placement_processor_thread, CacheServerPlacementProcessor::launchCacheServerPlacementProcessor, (void*)(&cache_server_placement_processor_param_));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

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

        // Wait for cache server placement processor
        pthread_returncode = pthread_join(cache_server_placement_processor_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server placement processor (error code: " << pthread_returncode << ")" << std::endl;
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

                if (data_request_ptr->isDataRequest()) // Data requests (e.g., local/redirected/management requests)
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

        const uint32_t percacheserver_workercnt = edge_wrapper_ptr_->getPercacheserverWorkercnt();

        if (data_requeset_ptr->isLocalDataRequest() || data_requeset_ptr->isRedirectedDataRequest()) // Local/redirected data requests
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
        else if (data_requeset_ptr->isManagementDataRequest()) // Management data requests
        {
            // Pass cache server item into ring buffer of the cache server placement processor
            CacheServerItem tmp_cache_server_item(data_requeset_ptr);
            bool is_successful = cache_server_placement_processor_param_.getDataRequestBufferPtr()->push(tmp_cache_server_item);
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

    // Admit content directory information (invoked by edge cache server worker or placement processor)

    bool CacheServer::admitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, BandwidthUsage& total_bandwidth_usage, EventList& event_list,const bool& skip_propagation_latency) const
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
            MessageBase* directory_update_request_ptr = getReqToAdmitBeaconDirectory_(key, directory_info, source_addr, skip_propagation_latency);
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
                    Util::dumpWarnMsg(instance_name_, "edge timeout to wait for DirectoryUpdateResponse");
                    continue; // Resend the control request message
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL);

                processRspToAdmitBeaconDirectory_(control_response_ptr, is_being_written); // NOTE: is_being_written is updated here

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
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_DIRECTORY_UPDATE_REQ_EVENT_NAME, issue_directory_update_req_latency_us);

        return is_finish;
    }

    MessageBase* CacheServer::getReqToAdmitBeaconDirectory_(const Key& key, const DirectoryInfo& directory_info, const NetworkAddr& source_addr, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        const bool is_admit = true; // Try to admit a new key as local cached object (NOTE: local edge cache has NOT been admitted yet)
        uint32_t edge_idx = edge_wrapper_ptr_->getNodeIdx();

        MessageBase* directory_update_request_ptr = NULL;
        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

            // Prepare victim syncset for piggybacking-based victim synchronization
            VictimSyncset victim_syncset = tmp_covered_cache_manager_ptr->accessVictimTrackerForVictimSyncset();

            // ONLY need victim synchronization yet without popularity collection/aggregation
            MessageBase* covered_directory_update_request_ptr = new CoveredDirectoryUpdateRequest(key, is_admit, directory_info, victim_syncset, edge_idx, source_addr, skip_propagation_latency);

            // Remove existing cached directory if any as key will be local cached
            tmp_covered_cache_manager_ptr->updateDirectoryCacherToRemoveCachedDirectory(key);
        }
        else // Baselines
        {
            directory_update_request_ptr = new DirectoryUpdateRequest(key, is_admit, directory_info, edge_idx, source_addr, skip_propagation_latency);
        }
        assert(directory_update_request_ptr != NULL);

        return directory_update_request_ptr;
    }

    void CacheServer::processRspToAdmitBeaconDirectory_(MessageBase* control_response_ptr, bool& is_being_written) const
    {
        checkPointers_();
        assert(control_response_ptr != NULL);

        if (edge_wrapper_ptr_->getCacheName() == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            assert(control_response_ptr->getMessageType() == MessageType::kCoveredDirectoryUpdateResponse);

            CoveredCacheManager* tmp_covered_cache_manager_ptr = edge_wrapper_ptr_->getCoveredCacheManagerPtr();

            // Get is_being_written from control response message
            const CoveredDirectoryUpdateResponse* const covered_directory_update_response_ptr = static_cast<const CoveredDirectoryUpdateResponse*>(control_response_ptr);
            is_being_written = covered_directory_update_response_ptr->isBeingWritten();

            // Victim synchronization
            const uint32_t source_edge_idx = covered_directory_update_response_ptr->getSourceIndex();
            const VictimSyncset& victim_syncset = covered_directory_update_response_ptr->getVictimSyncsetRef();
            std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets = edge_wrapper_ptr_->getLocalBeaconedVictimsFromVictimSyncset(victim_syncset);
            tmp_covered_cache_manager_ptr->updateVictimTrackerForVictimSyncset(source_edge_idx, victim_syncset, local_beaconed_neighbor_synced_victim_dirinfosets);
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

    void CacheServer::checkPointers_() const
    {
        assert(edge_wrapper_ptr_ != NULL);        
        assert(hash_wrapper_ptr_ != NULL);
        assert(edge_cache_server_recvreq_socket_server_ptr_ != NULL);

        return;
    }
}
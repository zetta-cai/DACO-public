#include "edge/edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "edge/beacon_server/beacon_server_base.h"
#include "edge/cache_server/cache_server.h"
#include "edge/invalidation_server/invalidation_server_base.h"
#include "event/event.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string EdgeWrapper::kClassName("EdgeWrapper");

    EdgeWrapperParam::EdgeWrapperParam()
    {
        edge_idx_ = 0;
        edge_cli_ptr_ = NULL;
    }

    EdgeWrapperParam::EdgeWrapperParam(const uint32_t& edge_idx, EdgeCLI* edge_cli_ptr)
    {
        assert(edge_cli_ptr != NULL);

        edge_idx_ = edge_idx;
        edge_cli_ptr_ = edge_cli_ptr;
    }

    EdgeWrapperParam::~EdgeWrapperParam()
    {
        // NOTE: NO need to release edge_cli_ptr_, which is maintained outside EdgeWrapperParam
    }

    uint32_t EdgeWrapperParam::getEdgeIdx() const
    {
        return edge_idx_;
    }
    
    EdgeCLI* EdgeWrapperParam::getEdgeCLIPtr() const
    {
        assert(edge_cli_ptr_ != NULL);
        return edge_cli_ptr_;
    }

    EdgeWrapperParam& EdgeWrapperParam::operator=(const EdgeWrapperParam& other)
    {
        edge_idx_ = other.edge_idx_;
        edge_cli_ptr_ = other.edge_cli_ptr_;
        return *this;
    }

    void* EdgeWrapper::launchEdge(void* edge_wrapper_param_ptr)
    {
        assert(edge_wrapper_param_ptr != NULL);
        EdgeWrapperParam& edge_wrapper_param = *((EdgeWrapperParam*)edge_wrapper_param_ptr);

        uint32_t edge_idx = edge_wrapper_param.getEdgeIdx();
        EdgeCLI* edge_cli_ptr = edge_wrapper_param.getEdgeCLIPtr();

        EdgeWrapper local_edge(edge_cli_ptr->getCacheName(), edge_cli_ptr->getCapacityBytes(), edge_idx, edge_cli_ptr->getEdgecnt(), edge_cli_ptr->getHashName(), edge_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes(), edge_cli_ptr->getPercacheserverWorkercnt(), edge_cli_ptr->getPeredgeSyncedVictimcnt(), edge_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes(), edge_cli_ptr->getCoveredPopularityCollectionChangeRatio(), edge_cli_ptr->getPropagationLatencyClientedgeUs(), edge_cli_ptr->getPropagationLatencyCrossedgeUs(), edge_cli_ptr->getPropagationLatencyEdgecloudUs(), edge_cli_ptr->getCoveredTopkEdgecnt());
        local_edge.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EdgeWrapper::EdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt) : NodeWrapperBase(NodeWrapperBase::EDGE_NODE_ROLE, edge_idx,edgecnt, true), cache_name_(cache_name), capacity_bytes_(capacity_bytes), percacheserver_workercnt_(percacheserver_workercnt)
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = new CacheWrapper(cache_name_, edge_idx, capacity_bytes, local_uncached_capacity_bytes, peredge_synced_victimcnt);
        assert(edge_cache_ptr_ != NULL);

        // Allocate cooperation wrapper for cooperative edge caching
        cooperation_wrapper_ptr_ = CooperationWrapperBase::getCooperationWrapperByCacheName(cache_name, edgecnt, edge_idx, hash_name);
        assert(cooperation_wrapper_ptr_ != NULL);

        // Allocate covered cache manager for COVERED only
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            covered_cache_manager_ptr_ = new CoveredCacheManager(edge_idx, edgecnt, peredge_synced_victimcnt, popularity_aggregation_capacity_bytes, popularity_collection_change_ratio, topk_edgecnt);
            assert(covered_cache_manager_ptr_ != NULL);
        }
        else
        {
            covered_cache_manager_ptr_ = NULL;
        }

        // Allocate edge-to-client propagation simulator param
        edge_toclient_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_clientedge_us, Config::getPropagationItemBufferSizeEdgeToclient());
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);

        // Allocate edge-to-edge propagation simulator param
        edge_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_crossedge_us, Config::getPropagationItemBufferSizeEdgeToedge());
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);

        // Allocate edge-to-cloud propagation simulator param
        edge_tocloud_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_edgecloud_us, Config::getPropagationItemBufferSizeEdgeTocloud());
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);

        // Sub-threads
        edge_toclient_propagation_simulator_thread_ = 0;
        edge_toedge_propagation_simulator_thread_ = 0;
        edge_tocloud_propagation_simulator_thread_ = 0;
        beacon_server_thread_ = 0;
        cache_server_thread_ = 0;
        invalidation_server_thread_ = 0;
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // Release local edge cache
        assert(edge_cache_ptr_ != NULL);
        delete edge_cache_ptr_;
        edge_cache_ptr_ = NULL;

        // Release cooperative cache wrapper
        assert(cooperation_wrapper_ptr_ != NULL);
        delete cooperation_wrapper_ptr_;
        cooperation_wrapper_ptr_ = NULL;

        // Release covered cache manager
        if (cache_name_ == Util::COVERED_CACHE_NAME)
        {
            assert(covered_cache_manager_ptr_ != NULL);
            delete covered_cache_manager_ptr_;
            covered_cache_manager_ptr_ = NULL;
        }
        else
        {
            assert(covered_cache_manager_ptr_ == NULL);
        }

        // Release edge-to-client propagation simulator param
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);
        delete edge_toclient_propagation_simulator_param_ptr_;
        edge_toclient_propagation_simulator_param_ptr_ = NULL;

        // Release edge-to-edge propagation simulator param
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);
        delete edge_toedge_propagation_simulator_param_ptr_;
        edge_toedge_propagation_simulator_param_ptr_ = NULL;

        // Release edge-to-cloud propagation simulator param
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);
        delete edge_tocloud_propagation_simulator_param_ptr_;
        edge_tocloud_propagation_simulator_param_ptr_ = NULL;
    }

    // (1) Const getters

    std::string EdgeWrapper::getCacheName() const
    {
        return cache_name_;
    }

    uint64_t EdgeWrapper::getCapacityBytes() const
    {
        return capacity_bytes_;
    }

    uint32_t EdgeWrapper::getPercacheserverWorkercnt() const
    {
        return percacheserver_workercnt_;
    }

    CacheWrapper* EdgeWrapper::getEdgeCachePtr() const
    {
        assert(edge_cache_ptr_ != NULL);
        return edge_cache_ptr_;
    }

    CooperationWrapperBase* EdgeWrapper::getCooperationWrapperPtr() const
    {
        assert(cooperation_wrapper_ptr_ != NULL);
        return cooperation_wrapper_ptr_;
    }

    CoveredCacheManager* EdgeWrapper::getCoveredCacheManagerPtr() const
    {
        // NOTE: non-COVERED caches should NOT call this function
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        assert(covered_cache_manager_ptr_ != NULL);
        return covered_cache_manager_ptr_;
    }

    PropagationSimulatorParam* EdgeWrapper::getEdgeToclientPropagationSimulatorParamPtr() const
    {
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);
        return edge_toclient_propagation_simulator_param_ptr_;
    }

    PropagationSimulatorParam* EdgeWrapper::getEdgeToedgePropagationSimulatorParamPtr() const
    {
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);
        return edge_toedge_propagation_simulator_param_ptr_;
    }
    
    PropagationSimulatorParam* EdgeWrapper::getEdgeTocloudPropagationSimulatorParamPtr() const
    {
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);
        return edge_tocloud_propagation_simulator_param_ptr_;
    }

    // (2) Utility functions

    uint64_t EdgeWrapper::getSizeForCapacity() const
    {
        checkPointers_();

        uint64_t edge_cache_size = edge_cache_ptr_->getSizeForCapacity();
        uint64_t cooperation_size = cooperation_wrapper_ptr_->getSizeForCapacity();
        uint64_t cache_manager_size = 0;
        if (cache_name_ == Util::COVERED_CACHE_NAME)
        {
            cache_manager_size = covered_cache_manager_ptr_->getSizeForCapacity();
        }

        uint64_t size = edge_cache_size + cooperation_size + cache_manager_size;

        return size;
    }

    bool EdgeWrapper::currentIsBeacon(const Key& key) const
    {
        checkPointers_();

        bool current_is_beacon = false;
        uint32_t beacon_edge_idx = cooperation_wrapper_ptr_->getBeaconEdgeIdx(key);
        uint32_t current_edge_idx = node_idx_;
        if (beacon_edge_idx == current_edge_idx)
        {
            current_is_beacon = true;
        }

        return current_is_beacon;
    }

    bool EdgeWrapper::currentIsTarget(const DirectoryInfo& directory_info) const
    {
        checkPointers_();

        bool current_is_target = false;
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        uint32_t current_edge_idx = node_idx_;
        if (target_edge_idx == current_edge_idx)
        {
            current_is_target = true;
        }

        return current_is_target;
    }

    // (3) Invalidate and unblock for MSI protocol

    bool EdgeWrapper::invalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        assert(recvrsp_socket_server_ptr != NULL);
        assert(recvrsp_source_addr.isValidAddr());
        checkPointers_();

        bool is_finish = false;
        struct timespec invalidate_cache_copies_start_timestamp = Util::getCurrentTimespec();

        uint32_t invalidate_edgecnt = all_dirinfo.size();
        if (invalidate_edgecnt == 0)
        {
            return is_finish;
        }
        assert(invalidate_edgecnt > 0);

        // Convert directory informations into destination network addresses
        std::unordered_map<uint32_t, NetworkAddr> percachecopy_dstaddr;
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator iter = all_dirinfo.begin(); iter != all_dirinfo.end(); iter++)
        {
            uint32_t tmp_edgeidx = iter->getTargetEdgeIdx();
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(tmp_edgeidx, node_cnt_);
            uint16_t tmp_edge_invaliation_server_port = Util::getEdgeInvalidationServerRecvreqPort(tmp_edgeidx, node_cnt_);
            NetworkAddr tmp_edge_invalidation_server_recvreq_dst_addr(tmp_edge_ipstr, tmp_edge_invaliation_server_port);
            percachecopy_dstaddr.insert(std::pair<uint32_t, NetworkAddr>(tmp_edgeidx, tmp_edge_invalidation_server_recvreq_dst_addr));
        }

        // Track whether invalidation requests to all involved edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::unordered_map<uint32_t, bool> acked_flags;
        for (std::unordered_map<uint32_t, NetworkAddr>::const_iterator iter_for_ackflag = percachecopy_dstaddr.begin(); iter_for_ackflag != percachecopy_dstaddr.end(); iter_for_ackflag++)
        {
            acked_flags.insert(std::pair<uint32_t, bool>(iter_for_ackflag->first, false));
        }

        // Issue all invalidation requests simultaneously
        while (acked_edgecnt != invalidate_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (invalidate_edgecnt - acked_edgecnt) control requests to the involved edge nodes that have not acknowledged invalidation requests
            for (std::unordered_map<uint32_t, bool>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second) // Skip the edge node that has acknowledged the invalidation request
                {
                    continue;
                }

                const NetworkAddr& tmp_edge_invalidation_server_recvreq_dst_addr = percachecopy_dstaddr[iter_for_request->first]; // cache server address of a blocked closest edge node          
                sendInvalidationRequest_(key, recvrsp_source_addr, tmp_edge_invalidation_server_recvreq_dst_addr, skip_propagation_latency);
            } // End of edgeidx_for_request

            // Receive (invalidate_edgecnt - acked_edgecnt) control repsonses from involved edge nodes
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < invalidate_edgecnt - acked_edgecnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    if (!isNodeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for InvalidationResponse");
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kInvalidationResponse);
                    uint32_t tmp_edgeidx = control_response_ptr->getSourceIndex();
                    NetworkAddr tmp_edge_invalidation_server_recvreq_source_addr = control_response_ptr->getSourceAddr();

                    // Mark the edge node has acknowledged the InvalidationRequest
                    bool is_match = false;
                    for (std::unordered_map<uint32_t, bool>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_edgeidx) // Match a neighbor edge node
                        {
                            assert(iter_for_response->second == false); // Original ack flag should be false
                            assert(percachecopy_dstaddr[iter_for_response->first] == tmp_edge_invalidation_server_recvreq_source_addr);

                            // Update total bandwidth usage for received invalidation response
                            BandwidthUsage invalidation_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_invalidation_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                            invalidation_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_invalidation_rsp_bandwidth_bytes, 0));
                            total_bandwidth_usage.update(invalidation_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive InvalidationResponse from edge node " << tmp_edgeidx << ", which is NOT in the content directory list!";
                        Util::dumpWarnMsg(instance_name_, oss.str());
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
        } // End of while(acked_edgecnt != blocked_edgecnt)

        // Add intermediate event if with event tracking
        struct timespec invalidate_cache_copies_end_timestamp = Util::getCurrentTimespec();
        uint32_t invalidate_cache_copies_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(invalidate_cache_copies_end_timestamp, invalidate_cache_copies_start_timestamp));
        event_list.addEvent(Event::EDGE_INVALIDATE_CACHE_COPIES_EVENT_NAME, invalidate_cache_copies_latency_us);

        return is_finish;
    }

    void EdgeWrapper::sendInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr edge_invalidation_server_recvreq_dst_addr, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        // Prepare invalidation request to invalidate the cache copy
        uint32_t edge_idx = node_idx_;
        MessageBase* invalidation_request_ptr = new InvalidationRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        assert(invalidation_request_ptr != NULL);

        // Push InvalidationRequest into edge-to-edge propagation simulator to send to neighbor edge node
        bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(invalidation_request_ptr, edge_invalidation_server_recvreq_dst_addr);
        assert(is_successful);

        // NOTE: invalidation_request_ptr will be released by edge-to-edge propagation simulator
        invalidation_request_ptr = NULL;

        return;
    }

    bool EdgeWrapper::notifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        assert(recvrsp_socket_server_ptr != NULL);
        assert(recvrsp_source_addr.isValidAddr());
        checkPointers_();

        bool is_finish = false;
        struct timespec finish_block_start_timestamp = Util::getCurrentTimespec();

        uint32_t blocked_edgecnt = blocked_edges.size();
        if (blocked_edgecnt == 0)
        {
            return is_finish;
        }
        assert(blocked_edgecnt > 0);

        // Track whether notifictions to all closest edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> acked_flags;
        for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_ackflag = blocked_edges.begin(); iter_for_ackflag != blocked_edges.end(); iter_for_ackflag++)
        {
            acked_flags.insert(std::pair<NetworkAddr, bool>(*iter_for_ackflag, false));
        }

        // Issue all finish block requests simultaneously
        while (acked_edgecnt != blocked_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (blocked_edgecnt - acked_edgecnt) control requests to the closest edge nodes that have not acknowledged notifications
            for (std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second) // Skip the closest edge node that has acknowledged the notification
                {
                    continue;
                }

                const NetworkAddr& tmp_edge_cache_server_worker_recvreq_dst_addr = iter_for_request->first; // cache server address of a blocked closest edge node          
                sendFinishBlockRequest_(key, recvrsp_source_addr, tmp_edge_cache_server_worker_recvreq_dst_addr, skip_propagation_latency);     
            } // End of edgeidx_for_request

            // Receive (blocked_edgecnt - acked_edgecnt) control repsonses from the closest edge nodes
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < blocked_edgecnt - acked_edgecnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                bool is_timeout = recvrsp_socket_server_ptr->recv(control_response_msg_payload);
                if (is_timeout)
                {
                    if (!isNodeRunning())
                    {
                        is_finish = true;
                        break; // Break as edge is NOT running
                    }
                    else
                    {
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for FinishBlockResponse");
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kFinishBlockResponse);
                    uint32_t tmp_edgeidx = control_response_ptr->getSourceIndex();
                    NetworkAddr tmp_edge_cache_server_worker_recvreq_source_addr = control_response_ptr->getSourceAddr();

                    // Mark the closest edge node has acknowledged the FinishBlockRequest
                    bool is_match = false;
                    for (std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_edge_cache_server_worker_recvreq_source_addr) // Match a blocked edge node
                        {
                            assert(iter_for_response->second == false); // Original ack flag should be false

                            // Update total bandwidth usage for received finish block response
                            BandwidthUsage finish_block_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_finish_block_rsp_bandwidth_bytes = control_response_ptr->getMsgPayloadSize();
                            finish_block_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_finish_block_rsp_bandwidth_bytes, 0));
                            total_bandwidth_usage.update(finish_block_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive FinishBlockResponse from edge node " << tmp_edgeidx << ", which is NOT in the block list!";
                        Util::dumpWarnMsg(instance_name_, oss.str());
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
        } // End of while(acked_edgecnt != blocked_edgecnt)

        // Add intermediate event if with event tracking
        struct timespec finish_block_end_timestamp = Util::getCurrentTimespec();
        uint32_t finish_block_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(finish_block_end_timestamp, finish_block_start_timestamp));
        event_list.addEvent(Event::EDGE_FINISH_BLOCK_EVENT_NAME, finish_block_latency_us);

        return is_finish;
    }

    void EdgeWrapper::sendFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        // Prepare finish block request to finish blocking for writes in all closest edge nodes
        uint32_t edge_idx = node_idx_;
        MessageBase* finish_block_request_ptr = new FinishBlockRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        assert(finish_block_request_ptr != NULL);

        // Push FinishBlockRequest into edge-to-edge propagation simulator to send to blocked edge node
        bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(finish_block_request_ptr, edge_cache_server_worker_recvreq_dst_addr);
        assert(is_successful);

        // NOTE: finish_block_request_ptr will be released by edge-to-edge propagation simulator
        finish_block_request_ptr = NULL;

        return;
    }

    // (4) Benchmark process

    void EdgeWrapper::initialize_()
    {
        checkPointers_();

        int pthread_returncode = 0;

        // Launch edge-to-client propagation simulator
        //pthread_returncode = pthread_create(&edge_toclient_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);
        pthread_returncode = Util::pthreadCreateHighPriority(&edge_toclient_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch edge-to-client propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch edge-to-edge propagation simulator
        //pthread_returncode = pthread_create(&edge_toedge_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);
        pthread_returncode = Util::pthreadCreateHighPriority(&edge_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch edge-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch edge-to-cloud propagation simulator
        //pthread_returncode = pthread_create(&edge_tocloud_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);
        pthread_returncode = Util::pthreadCreateHighPriority(&edge_tocloud_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to launch edge-to-cloud propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch beacon server
        //pthread_returncode = pthread_create(&beacon_server_thread_, NULL, launchBeaconServer_, (void*)(this));
        pthread_returncode = Util::pthreadCreateHighPriority(&beacon_server_thread_, launchBeaconServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to launch beacon server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch cache server
        //pthread_returncode = pthread_create(&cache_server_thread_, NULL, launchCacheServer_, (void*)(this));
        pthread_returncode = Util::pthreadCreateLowPriority(&cache_server_thread_, launchCacheServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to launch cache server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch invalidation server
        //pthread_returncode = pthread_create(&invalidation_server_thread_, NULL, launchInvalidationServer_, (void*)(this));
        pthread_returncode = Util::pthreadCreateHighPriority(&invalidation_server_thread_, launchInvalidationServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to launch invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void EdgeWrapper::processFinishrunRequest_()
    {
        checkPointers_();

        // Mark the current node as NOT running to finish benchmark
        resetNodeRunning_();

        // Send back SimpleFinishrunResponse to evaluator
        SimpleFinishrunResponse simple_finishrun_response(node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&simple_finishrun_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    void EdgeWrapper::processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        assert(control_request_ptr != NULL);
        
        std::ostringstream oss;
        oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for startInternal_()";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);
        return;
    }

    void EdgeWrapper::cleanup_()
    {
        checkPointers_();

        int pthread_returncode = 0;

        // Wait edge-to-client propagation simulator
        pthread_returncode = pthread_join(edge_toclient_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-client propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait edge-to-edge propagation simulator
        pthread_returncode = pthread_join(edge_toedge_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait edge-to-cloud propagation simulator
        pthread_returncode = pthread_join(edge_tocloud_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-cloud propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for beacon server
        pthread_returncode = pthread_join(beacon_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join beacon server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server
        pthread_returncode = pthread_join(cache_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join cache server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for invalidation server
        pthread_returncode = pthread_join(invalidation_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // (5) Other utilities

    void* EdgeWrapper::launchBeaconServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        BeaconServerBase* beacon_server_ptr = BeaconServerBase::getBeaconServerByCacheName((EdgeWrapper*)edge_wrapper_ptr);
        assert(beacon_server_ptr != NULL);
        beacon_server_ptr->start();

        assert(beacon_server_ptr != NULL);
        delete beacon_server_ptr;
        beacon_server_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchCacheServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        CacheServer cache_server = CacheServer((EdgeWrapper*)edge_wrapper_ptr);
        cache_server.start();

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchInvalidationServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        InvalidationServerBase* invalidation_server_ptr = InvalidationServerBase::getInvalidationServerByCacheName((EdgeWrapper*)edge_wrapper_ptr);
        assert(invalidation_server_ptr != NULL);
        invalidation_server_ptr->start();

        assert(invalidation_server_ptr != NULL);
        delete invalidation_server_ptr;
        invalidation_server_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    void EdgeWrapper::checkPointers_() const
    {
        NodeWrapperBase::checkPointers_();

        assert(edge_cache_ptr_ != NULL);
        assert(cooperation_wrapper_ptr_ != NULL);
        if (cache_name_ == Util::COVERED_CACHE_NAME)
        {
            assert(covered_cache_manager_ptr_ != NULL);
        }
        else
        {
            assert(covered_cache_manager_ptr_ == NULL);
        }
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);

        return;
    }

    // (6) covered-specific utility functions

    void EdgeWrapper::updateCacheManagerForLocalSyncedVictims() const
    {
        checkPointers_();

        // Get local edge margin bytes
        uint64_t used_bytes = getSizeForCapacity();
        uint64_t capacity_bytes = getCapacityBytes();
        uint64_t local_cache_margin_bytes = (capacity_bytes >= used_bytes) ? (capacity_bytes - used_bytes) : 0;

        // Get victim cacheinfos of local synced victims for the current edge node
        std::list<VictimCacheinfo> local_synced_victim_cacheinfos = edge_cache_ptr_->getLocalSyncedVictimCacheinfos();

        // Get directory info sets for local synced victimed beaconed by the current edge node
        const uint32_t current_edge_idx = getNodeIdx();
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_local_synced_victim_dirinfosets;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victim_cacheinfos.begin(); cacheinfo_list_iter != local_synced_victim_cacheinfos.end(); cacheinfo_list_iter++)
        {
            const Key& tmp_key = cacheinfo_list_iter->getKey();
            bool current_is_beacon = currentIsBeacon(tmp_key);
            if (current_is_beacon) // Key is beaconed by current edge node
            {
                dirinfo_set_t tmp_dirinfo_set = cooperation_wrapper_ptr_->getLocalDirectoryInfos(tmp_key);
                local_beaconed_local_synced_victim_dirinfosets.insert(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        // Update local synced victims for the current edge node
        covered_cache_manager_ptr_->updateVictimTrackerForLocalSyncedVictims(local_cache_margin_bytes, local_synced_victim_cacheinfos, local_beaconed_local_synced_victim_dirinfosets); 

        return;
    }

    std::unordered_map<Key, dirinfo_set_t, KeyHasher> EdgeWrapper::getLocalBeaconedVictimsFromVictimSyncset(const VictimSyncset& victim_syncset) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);

        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_neighbor_synced_victim_dirinfosets;

        const std::list<VictimCacheinfo>& neighbor_synced_victims = victim_syncset.getLocalSyncedVictimsRef();
        for (std::list<VictimCacheinfo>::const_iterator iter = neighbor_synced_victims.begin(); iter != neighbor_synced_victims.end(); iter++)
        {
            const Key& tmp_key = iter->getKey();
            bool current_is_beacon = currentIsBeacon(tmp_key);
            if (current_is_beacon) // Key is beaconed by current edge node
            {
                dirinfo_set_t tmp_dirinfo_set = cooperation_wrapper_ptr_->getLocalDirectoryInfos(tmp_key);
                local_beaconed_neighbor_synced_victim_dirinfosets.insert(std::pair(tmp_key, tmp_dirinfo_set));
            }
        }

        return local_beaconed_neighbor_synced_victim_dirinfosets;
    }

    bool EdgeWrapper::nonblockDataFetchForPlacement(const Key& key, const std::unordered_set<uint32_t>& best_placement_edgeset) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);

        bool need_hybrid_fetching = false;

        // Check local edge cache in local/remote beacon node first
        Value value;
        bool affect_victim_tracker = false;
        bool is_local_cached_and_valid = edge_cache_ptr_->get(key, value, affect_victim_tracker);

        // Avoid unnecessary VictimTracker update
        if (affect_victim_tracker) // If key was a local synced victim before or is a local synced victim now
        {
            updateCacheManagerForLocalSyncedVictims();
        }

        if (is_local_cached_and_valid) // Directly get valid value from local edge cache in local/remote beacon node
        {
            // TODO: Perform non-blocking placement notification
            //nonblockNotifyForPlacement(key, value, best_placement_edgeset);
        }
        else // No valid value in local edge cache in local/remote beacon node
        {
            // Check if exist valid dirinfo to fetch data from neighbor
            uint32_t current_edge_idx = getNodeIdx();
            bool is_being_written = false;
            bool is_valid_directory_exist = false;
            DirectoryInfo directory_info;
            bool is_source_cached = false;
            bool is_global_cached = cooperation_wrapper_ptr_->lookupDirectoryTableByCacheServer(key, current_edge_idx, is_being_written, is_valid_directory_exist, directory_info, is_source_cached); // NOTE: local/remote beacon node lookup its own local directory table (= current is beacon for cache server)
            UNUSED(is_source_cached);
            UNUSED(is_global_cached);

            if (is_valid_directory_exist) // A neighbor edge node has cached the object
            {
                assert(!is_being_written); // Being-written object CANNOT have any valid dirinfo

                // END HERE
                // TODO: Send CoveredPlacementRedirectedGetRequest to the neighbor node

                // NOTE: we use edge_beacon_server_recvreq_source_addr_ as the soure address even if invoker is cache server to wait for responses
                // (i) Although current is beacon for cache server, the role is still beacon node, which should be responsible for non-blocking placement deployment. And we don't want to resort cache server worker, which may degrade KV request processing
                // (ii) Although we may wait for responses, beacon server is blocking for recvreq port and we don't want to introduce another blocking for recvrsp port

                // NOTE: CoveredPlacementRedirectedGetResponse will be processed by covered beacon server (current edge node is a remote beacon node for neighbor edge node, as neighbor MUST NOT be a different edge node for request redirection)
            }
            else // No valid cache copy in edge layer
            {
                // We need hybrid fetching to resort the sender to fetch data from cloud for reducing edge-cloud bandwidth usage
                need_hybrid_fetching = true;
            }
        }

        return need_hybrid_fetching;
    }
}
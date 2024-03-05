#include "edge/edge_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "edge/basic_edge_wrapper.h"
#include "edge/covered_edge_wrapper.h"
#include "edge/beacon_server/beacon_server_base.h"
#include "edge/cache_server/cache_server_base.h"
#include "event/event.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string EdgeWrapperBase::kClassName("EdgeWrapperBase");

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

    void* EdgeWrapperBase::launchEdge(void* edge_wrapper_param_ptr)
    {
        assert(edge_wrapper_param_ptr != NULL);
        EdgeWrapperParam& edge_wrapper_param = *((EdgeWrapperParam*)edge_wrapper_param_ptr);

        uint32_t edge_idx = edge_wrapper_param.getEdgeIdx();
        EdgeCLI* edge_cli_ptr = edge_wrapper_param.getEdgeCLIPtr();

        EdgeWrapperBase* edge_wrapper_ptr = NULL;
        const std::string cache_name = edge_cli_ptr->getCacheName();
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            edge_wrapper_ptr = new CoveredEdgeWrapper(cache_name, edge_cli_ptr->getCapacityBytes(), edge_idx, edge_cli_ptr->getEdgecnt(), edge_cli_ptr->getHashName(), edge_cli_ptr->getKeycnt(), edge_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes(), edge_cli_ptr->getPercacheserverWorkercnt(), edge_cli_ptr->getCoveredPeredgeSyncedVictimcnt(), edge_cli_ptr->getCoveredPeredgeMonitoredVictimsetcnt(), edge_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes(), edge_cli_ptr->getCoveredPopularityCollectionChangeRatio(), edge_cli_ptr->getPropagationLatencyClientedgeUs(), edge_cli_ptr->getPropagationLatencyCrossedgeUs(), edge_cli_ptr->getPropagationLatencyEdgecloudUs(), edge_cli_ptr->getCoveredTopkEdgecnt());
        }
        else
        {
            edge_wrapper_ptr = new BasicEdgeWrapper(cache_name, edge_cli_ptr->getCapacityBytes(), edge_idx, edge_cli_ptr->getEdgecnt(), edge_cli_ptr->getHashName(), edge_cli_ptr->getKeycnt(), edge_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes(), edge_cli_ptr->getPercacheserverWorkercnt(), edge_cli_ptr->getCoveredPeredgeSyncedVictimcnt(), edge_cli_ptr->getCoveredPeredgeMonitoredVictimsetcnt(), edge_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes(), edge_cli_ptr->getCoveredPopularityCollectionChangeRatio(), edge_cli_ptr->getPropagationLatencyClientedgeUs(), edge_cli_ptr->getPropagationLatencyCrossedgeUs(), edge_cli_ptr->getPropagationLatencyEdgecloudUs(), edge_cli_ptr->getCoveredTopkEdgecnt());
        }
        assert(edge_wrapper_ptr != NULL);
        edge_wrapper_ptr->start();

        delete edge_wrapper_ptr;
        edge_wrapper_ptr = NULL;
        
        pthread_exit(NULL);
        return NULL;
    }

    EdgeWrapperBase::EdgeWrapperBase(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt) : NodeWrapperBase(NodeWrapperBase::EDGE_NODE_ROLE, edge_idx, edgecnt, true), cache_name_(cache_name), capacity_bytes_(capacity_bytes), percacheserver_workercnt_(percacheserver_workercnt), propagation_latency_crossedge_us_(propagation_latency_crossedge_us), propagation_latency_edgecloud_us_(propagation_latency_edgecloud_us), edge_background_counter_for_beacon_server_()
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        base_instance_name_ = oss.str();
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = new CacheWrapper(this, cache_name, edge_idx, capacity_bytes, keycnt, local_uncached_capacity_bytes, peredge_synced_victimcnt);
        assert(edge_cache_ptr_ != NULL);

        // Allocate cooperation wrapper for cooperative edge caching
        cooperation_wrapper_ptr_ = CooperationWrapperBase::getCooperationWrapperByCacheName(cache_name, edgecnt, edge_idx, hash_name);
        assert(cooperation_wrapper_ptr_ != NULL);

        // Allocate edge-to-client propagation simulator param
        edge_toclient_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_clientedge_us, Config::getPropagationItemBufferSizeEdgeToclient());
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);

        // Allocate edge-to-edge propagation simulator param
        edge_toedge_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_crossedge_us, Config::getPropagationItemBufferSizeEdgeToedge());
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);

        // Allocate edge-to-cloud propagation simulator param
        edge_tocloud_propagation_simulator_param_ptr_ = new PropagationSimulatorParam((NodeWrapperBase*)this, propagation_latency_edgecloud_us, Config::getPropagationItemBufferSizeEdgeTocloud());
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);

        // Allocate edge beacon server param
        edge_beacon_server_param_ptr_ = new EdgeComponentParam(this);
        assert(edge_beacon_server_param_ptr_ != NULL);

        // Allocate edge cache server param
        edge_cache_server_param_ptr_ = new EdgeComponentParam(this);
        assert(edge_cache_server_param_ptr_ != NULL);

        // Sub-threads
        edge_toclient_propagation_simulator_thread_ = 0;
        edge_toedge_propagation_simulator_thread_ = 0;
        edge_tocloud_propagation_simulator_thread_ = 0;
        beacon_server_thread_ = 0;
        cache_server_thread_ = 0;

        // Allocate ring buffer for local edge cache admissions
        const bool local_cache_admission_with_multi_providers = true; // Multiple providers (edge cache server workers after hybrid data fetching; edge cache server workers and edge beacon server for local placement notifications)
        local_cache_admission_buffer_ptr_ = new RingBuffer<LocalCacheAdmissionItem>(LocalCacheAdmissionItem(), Config::getEdgeCacheServerDataRequestBufferSize(), local_cache_admission_with_multi_providers);
        assert(local_cache_admission_buffer_ptr_ != NULL);

        // ONLY used by COVERED
        UNUSED(peredge_synced_victimcnt);
        UNUSED(peredge_monitored_victimsetcnt);
        UNUSED(popularity_aggregation_capacity_bytes);
        UNUSED(popularity_collection_change_ratio);
        UNUSED(topk_edgecnt);
    }
        
    EdgeWrapperBase::~EdgeWrapperBase()
    {
        #ifdef DEBUG_EDGE_WRAPPER_BASE
        uint64_t edge_cache_size = edge_cache_ptr_->getSizeForCapacity();
        uint64_t cooperation_size = cooperation_wrapper_ptr_->getSizeForCapacity();
        std::ostringstream oss;
        oss << "edge_cache_size: " << edge_cache_size << ", cooperation_size: " << cooperation_size;
        Util::dumpDebugMsg(base_instance_name_, oss.str());
        #endif

        // Release local edge cache
        assert(edge_cache_ptr_ != NULL);
        delete edge_cache_ptr_;
        edge_cache_ptr_ = NULL;

        // Release cooperative cache wrapper
        assert(cooperation_wrapper_ptr_ != NULL);
        delete cooperation_wrapper_ptr_;
        cooperation_wrapper_ptr_ = NULL;

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

        // Release edge beacon server param
        assert(edge_beacon_server_param_ptr_ != NULL);
        delete edge_beacon_server_param_ptr_;
        edge_beacon_server_param_ptr_ = NULL;

        // Release edge cache server param
        assert(edge_cache_server_param_ptr_ != NULL);
        delete edge_cache_server_param_ptr_;
        edge_cache_server_param_ptr_ = NULL;

        // Release local cache admission ring buffer
        assert(local_cache_admission_buffer_ptr_ != NULL);
        delete local_cache_admission_buffer_ptr_;
        local_cache_admission_buffer_ptr_ = NULL;
    }

    // (1) Const getters

    std::string EdgeWrapperBase::getCacheName() const
    {
        return cache_name_;
    }

    uint64_t EdgeWrapperBase::getCapacityBytes() const
    {
        return capacity_bytes_;
    }

    uint32_t EdgeWrapperBase::getPercacheserverWorkercnt() const
    {
        return percacheserver_workercnt_;
    }

    uint32_t EdgeWrapperBase::getPropagationLatencyCrossedgeUs() const
    {
        return propagation_latency_crossedge_us_;
    }

    uint32_t EdgeWrapperBase::getPropagationLatencyEdgecloudUs() const
    {
        return propagation_latency_edgecloud_us_;
    }

    CacheWrapper* EdgeWrapperBase::getEdgeCachePtr() const
    {
        assert(edge_cache_ptr_ != NULL);
        return edge_cache_ptr_;
    }

    CooperationWrapperBase* EdgeWrapperBase::getCooperationWrapperPtr() const
    {
        assert(cooperation_wrapper_ptr_ != NULL);
        return cooperation_wrapper_ptr_;
    }

    PropagationSimulatorParam* EdgeWrapperBase::getEdgeToclientPropagationSimulatorParamPtr() const
    {
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);
        return edge_toclient_propagation_simulator_param_ptr_;
    }

    PropagationSimulatorParam* EdgeWrapperBase::getEdgeToedgePropagationSimulatorParamPtr() const
    {
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);
        return edge_toedge_propagation_simulator_param_ptr_;
    }
    
    PropagationSimulatorParam* EdgeWrapperBase::getEdgeTocloudPropagationSimulatorParamPtr() const
    {
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);
        return edge_tocloud_propagation_simulator_param_ptr_;
    }

    EdgeComponentParam* EdgeWrapperBase::getEdgeBeaconServerParamPtr() const
    {
        assert(edge_beacon_server_param_ptr_ != NULL);
        return edge_beacon_server_param_ptr_;
    }

    EdgeComponentParam* EdgeWrapperBase::getEdgeCacheServerParamPtr() const
    {
        assert(edge_cache_server_param_ptr_ != NULL);
        return edge_cache_server_param_ptr_;
    }

    RingBuffer<LocalCacheAdmissionItem>* EdgeWrapperBase::getLocalCacheAdmissionBufferPtr() const
    {
        assert(local_cache_admission_buffer_ptr_ != NULL);
        return local_cache_admission_buffer_ptr_;
    }

    BackgroundCounter& EdgeWrapperBase::getEdgeBackgroundCounterForBeaconServerRef()
    {
        return edge_background_counter_for_beacon_server_;
    }

    // (2) Utility functions

    uint64_t EdgeWrapperBase::getSizeForCapacity() const
    {
        checkPointers_();

        uint64_t edge_cache_size = edge_cache_ptr_->getSizeForCapacity();
        uint64_t cooperation_size = cooperation_wrapper_ptr_->getSizeForCapacity();

        uint64_t size = edge_cache_size + cooperation_size;

        return size;
    }

    bool EdgeWrapperBase::currentIsBeacon(const Key& key) const
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

    bool EdgeWrapperBase::currentIsTarget(const DirectoryInfo& directory_info) const
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

    NetworkAddr EdgeWrapperBase::getBeaconDstaddr_(const Key& key) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        MYASSERT(!currentIsBeacon(key));

        // Get remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqAddr(key);

        return beacon_edge_beacon_server_recvreq_dst_addr;
    }

    NetworkAddr EdgeWrapperBase::getBeaconDstaddr_(const uint32_t& beacon_edge_idx) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        assert(beacon_edge_idx != getNodeIdx());

        // Get remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqAddr(beacon_edge_idx);

        return beacon_edge_beacon_server_recvreq_dst_addr;
    }

    NetworkAddr EdgeWrapperBase::getTargetDstaddr(const DirectoryInfo& directory_info) const
    {
        checkPointers_();

        // The current edge node must NOT be the target node
        bool current_is_target = currentIsTarget(directory_info);
        assert(!current_is_target);

        // Set remote address such that the current edge node can communicate with the target edge node
        const bool is_private_edge_ipstr = false; // NOTE: cross-edge communication for request redirection uses public IP address
        const bool is_launch_edge = false; // Just connect target edge node by the current edge node instead of launching the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx, getNodeCnt(), is_private_edge_ipstr, is_launch_edge);
        uint16_t target_edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(target_edge_idx, getNodeCnt());
        NetworkAddr target_edge_cache_server_recvreq_dst_addr(target_edge_ipstr, target_edge_cache_server_recvreq_port);

        return target_edge_cache_server_recvreq_dst_addr;
    }

    // (3) Invalidate and unblock for MSI protocol

    bool EdgeWrapperBase::parallelInvalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
    {
        assert(recvrsp_socket_server_ptr != NULL);
        assert(recvrsp_source_addr.isValidAddr());
        checkPointers_();

        bool is_finish = false;
        struct timespec invalidate_cache_copies_start_timestamp = Util::getCurrentTimespec();

        // Get dirinfo unordered set from dirinfo set
        std::list<DirectoryInfo> tmp_all_dirinfo_list;
        bool with_complete_dirinfo_set = all_dirinfo.getDirinfoSetIfComplete(tmp_all_dirinfo_list);
        assert(with_complete_dirinfo_set); // NOTE: dirinfo set from local directory table MUST be complete

        // Check if exist any dirinfo to invalidate
        uint32_t invalidate_edgecnt = tmp_all_dirinfo_list.size();
        if (invalidate_edgecnt == 0)
        {
            return is_finish;
        }
        assert(invalidate_edgecnt > 0);

        // Convert directory informations into destination network addresses
        std::unordered_map<uint32_t, NetworkAddr> percachecopy_dstaddr;
        for (std::list<DirectoryInfo>::const_iterator iter = tmp_all_dirinfo_list.begin(); iter != tmp_all_dirinfo_list.end(); iter++)
        {
            const bool is_private_edge_ipstr = false; // NOTE: cross-edge communication for cache invalidation uses public IP address
            const bool is_launch_edge = false; // Just connect neighbor to invalidate cache copies instead of launching the neighbor
            uint32_t tmp_edgeidx = iter->getTargetEdgeIdx();
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(tmp_edgeidx, node_cnt_, is_private_edge_ipstr, is_launch_edge);
            uint16_t tmp_edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(tmp_edgeidx, node_cnt_);
            NetworkAddr tmp_edge_cache_server_recvreq_dst_addr(tmp_edge_ipstr, tmp_edge_cache_server_recvreq_port);
            percachecopy_dstaddr.insert(std::pair<uint32_t, NetworkAddr>(tmp_edgeidx, tmp_edge_cache_server_recvreq_dst_addr));
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

                // NOTE: we only issue invalidation requests to neighbors
                const uint32_t& tmp_dst_edge_idx = iter_for_request->first;
                if (tmp_dst_edge_idx == node_idx_) // Invalidat local cache
                {
                    // Invalidate cached object in local edge cache
                    bool is_local_cached = edge_cache_ptr_->isLocalCached(key);
                    if (is_local_cached)
                    {
                        edge_cache_ptr_->invalidateKeyForLocalCachedObject(key);
                    }

                    // Update ack information
                    assert(acked_flags[tmp_dst_edge_idx] == false);
                    acked_flags[tmp_dst_edge_idx] = true;
                    acked_edgecnt += 1;
                }
                else // Issue request to invalidate remote cache
                {
                    MessageBase* invalidation_request_ptr = getInvalidationRequest_(key, recvrsp_source_addr, tmp_dst_edge_idx, extra_common_msghdr);
                    assert(invalidation_request_ptr != NULL);

                    // Push invalidation request into edge-to-edge propagation simulator to send to neighbor edge node
                    const NetworkAddr& tmp_edge_cache_server_recvreq_dst_addr = percachecopy_dstaddr[tmp_dst_edge_idx]; // cache server address of a blocked closest edge node
                    bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(invalidation_request_ptr, tmp_edge_cache_server_recvreq_dst_addr);
                    assert(is_successful);

                    // NOTE: invalidation_request_ptr will be released by edge-to-edge propagation simulator
                    invalidation_request_ptr = NULL;
                }
            } // End of edgeidx_for_request

            // Receive (invalidate_edgecnt - acked_edgecnt) control repsonses from involved edge nodes
            const uint32_t expected_rspcnt = invalidate_edgecnt - acked_edgecnt;
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < expected_rspcnt; edgeidx_for_response++)
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
                        std::ostringstream oss;
                        oss << "edge timeout to wait for InvalidationResponse for key " << key.getKeyDebugstr() << " from " << Util::getAckedStatusStr(acked_flags, "edge");
                        Util::dumpWarnMsg(base_instance_name_, oss.str());
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    uint32_t tmp_edgeidx = control_response_ptr->getSourceIndex();
                    NetworkAddr tmp_edge_cache_server_recvreq_source_addr = control_response_ptr->getSourceAddr();

                    // Mark the edge node has acknowledged the InvalidationRequest
                    bool is_match = false;
                    for (std::unordered_map<uint32_t, bool>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_edgeidx) // Match a neighbor edge node
                        {
                            assert(iter_for_response->second == false); // Original ack flag should be false
                            assert(percachecopy_dstaddr[iter_for_response->first] == tmp_edge_cache_server_recvreq_source_addr);

                            // Process invalidation response
                            processInvalidationResponse_(control_response_ptr);

                            // Update total bandwidth usage for received invalidation response
                            BandwidthUsage invalidation_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_invalidation_rsp_bandwidth_bytes = control_response_ptr->getMsgBandwidthSize();
                            invalidation_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_invalidation_rsp_bandwidth_bytes, 0, 0, 1, 0));
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
                        Util::dumpWarnMsg(base_instance_name_, oss.str());
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

    bool EdgeWrapperBase::parallelNotifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const ExtraCommonMsghdr& extra_common_msghdr) const
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
        std::unordered_map<NetworkAddr, std::pair<bool, uint32_t>, NetworkAddrHasher> acked_flags; // NOTE: bool refers to whether ACK is received, while uint32_t refers to edge index for debugging if with timeout
        for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_ackflag = blocked_edges.begin(); iter_for_ackflag != blocked_edges.end(); iter_for_ackflag++)
        {
            const bool is_private_edge_ipstr = false; // NOTE: IP address for finishing blocking under MSI comes from directory lookup/update and acquire/release writelock requests, which MUST be public due to cross-edge communication
            uint32_t tmp_dst_edge_idx = Util::getEdgeIdxFromCacheServerWorkerRecvreqAddr(*iter_for_ackflag, is_private_edge_ipstr, getNodeCnt());
            
            acked_flags.insert(std::pair<NetworkAddr, std::pair<bool, uint32_t>>(*iter_for_ackflag, std::pair(false, tmp_dst_edge_idx)));
        }

        // Issue all finish block requests simultaneously
        while (acked_edgecnt != blocked_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (blocked_edgecnt - acked_edgecnt) control requests to the closest edge nodes that have not acknowledged notifications
            for (std::unordered_map<NetworkAddr, std::pair<bool, uint32_t>, NetworkAddrHasher>::const_iterator iter_for_request = acked_flags.begin(); iter_for_request != acked_flags.end(); iter_for_request++)
            {
                if (iter_for_request->second.first) // Skip the closest edge node that has acknowledged the notification
                {
                    continue;
                }

                const NetworkAddr& tmp_edge_cache_server_worker_recvreq_dst_addr = iter_for_request->first; // cache server address of a blocked closest edge node

                // NOTE: dst edge idx to finish blocking MUST NOT be the current local/remote beacon edge node, as requests on a being-written object MUST poll instead of block if sender is beacon
                const uint32_t tmp_dst_edge_idx = iter_for_request->second.second;
                assert(tmp_dst_edge_idx != node_idx_);
   
                // Issue finish block request to dst edge node
                MessageBase* finish_block_request_ptr = getFinishBlockRequest_(key, recvrsp_source_addr, tmp_dst_edge_idx, extra_common_msghdr);
                assert(finish_block_request_ptr != NULL);

                // Push FinishBlockRequest into edge-to-edge propagation simulator to send to blocked edge node
                bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(finish_block_request_ptr, tmp_edge_cache_server_worker_recvreq_dst_addr);
                assert(is_successful);

                // NOTE: finish_block_request_ptr will be released by edge-to-edge propagation simulator
                finish_block_request_ptr = NULL;
            } // End of edgeidx_for_request

            // Receive (blocked_edgecnt - acked_edgecnt) control repsonses from the closest edge nodes
            const uint32_t expected_rspcnt = blocked_edgecnt - acked_edgecnt;
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < expected_rspcnt; edgeidx_for_response++)
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
                        Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for FinishBlockResponse from " + Util::getAckedStatusStr(acked_flags, "edge"));
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
                    uint32_t tmp_edgeidx = control_response_ptr->getSourceIndex();
                    NetworkAddr tmp_edge_cache_server_worker_recvreq_source_addr = control_response_ptr->getSourceAddr();

                    // Process finish block response
                    processFinishBlockResponse_(control_response_ptr);

                    // Mark the closest edge node has acknowledged the FinishBlockRequest
                    bool is_match = false;
                    for (std::unordered_map<NetworkAddr, std::pair<bool, uint32_t>, NetworkAddrHasher>::iterator iter_for_response = acked_flags.begin(); iter_for_response != acked_flags.end(); iter_for_response++)
                    {
                        if (iter_for_response->first == tmp_edge_cache_server_worker_recvreq_source_addr) // Match a blocked edge node
                        {
                            assert(iter_for_response->second.first == false); // Original ack flag should be false

                            // Update total bandwidth usage for received finish block response
                            BandwidthUsage finish_block_response_bandwidth_usage = control_response_ptr->getBandwidthUsageRef();
                            uint32_t cross_edge_finish_block_rsp_bandwidth_bytes = control_response_ptr->getMsgBandwidthSize();
                            finish_block_response_bandwidth_usage.update(BandwidthUsage(0, cross_edge_finish_block_rsp_bandwidth_bytes, 0, 0, 1, 0));
                            total_bandwidth_usage.update(finish_block_response_bandwidth_usage);

                            // Add the event of intermediate response if with event tracking
                            event_list.addEvents(control_response_ptr->getEventListRef());

                            // Update ack information
                            iter_for_response->second.first = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive FinishBlockResponse from edge node " << tmp_edgeidx << ", which is NOT in the block list!";
                        Util::dumpWarnMsg(base_instance_name_, oss.str());
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

    // (4) Benchmark process

    void EdgeWrapperBase::initialize_()
    {
        checkPointers_();

        // Launch edge-to-client propagation simulator
        //int pthread_returncode = pthread_create(&edge_toclient_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-client propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "edge-toclient-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &edge_toclient_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);

        // Launch edge-to-edge propagation simulator
        //pthread_returncode = pthread_create(&edge_toedge_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-toedge-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &edge_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);

        // Launch edge-to-cloud propagation simulator
        //pthread_returncode = pthread_create(&edge_tocloud_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-cloud propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-tocloud-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &edge_tocloud_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);

        // Launch beacon server
        //pthread_returncode = pthread_create(&beacon_server_thread_, NULL, BeaconServerBase::launchBeaconServer, (void*)(this));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch beacon server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-beacon-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &beacon_server_thread_, BeaconServerBase::launchBeaconServer, (void*)(edge_beacon_server_param_ptr_));

        // Launch cache server
        //pthread_returncode = pthread_create(&cache_server_thread_, NULL, CacheServerBase::launchCacheServer, (void*)(this));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch cache server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(base_instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(ThreadLauncher::EDGE_THREAD_ROLE, tmp_thread_name, &cache_server_thread_, CacheServerBase::launchCacheServer, (void*)(edge_cache_server_param_ptr_));

        // Wait edge-to-client propagation simulator to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait edge-to-client propagation simulator to finish initialization...");
        while (!edge_toclient_propagation_simulator_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait edge-to-edge propagation simulator to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait edge-to-edge propagation simulator to finish initialization...");
        while (!edge_toedge_propagation_simulator_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait edge-to-cloud propagation simulator to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait edge-to-cloud propagation simulator to finish initialization...");
        while (!edge_tocloud_propagation_simulator_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait edge beacon server to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait edge beacon server to finish initialization...");
        while (!edge_beacon_server_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        // Wait edge cache server to finish initialization
        Util::dumpNormalMsg(base_instance_name_, "wait edge cache server to finish initialization...");
        while (!edge_cache_server_param_ptr_->isFinishInitialization())
        {
            usleep(SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US);
        }

        return;
    }

    void EdgeWrapperBase::processFinishrunRequest_()
    {
        checkPointers_();

        // Mark the current node as NOT running to finish benchmark
        resetNodeRunning_();

        // Send back SimpleFinishrunResponse to evaluator
        SimpleFinishrunResponse simple_finishrun_response(node_idx_, node_recvmsg_source_addr_, EventList());
        node_sendmsg_socket_client_ptr_->send((MessageBase*)&simple_finishrun_response, evaluator_recvmsg_dst_addr_);

        return;
    }

    void EdgeWrapperBase::processOtherBenchmarkControlRequest_(MessageBase* control_request_ptr)
    {
        checkPointers_();
        assert(control_request_ptr != NULL);
        
        std::ostringstream oss;
        oss << "invalid message type " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " for startInternal_()";
        Util::dumpErrorMsg(base_instance_name_, oss.str());
        exit(1);
        return;
    }

    void EdgeWrapperBase::cleanup_()
    {
        checkPointers_();

        int pthread_returncode = 0;

        // Wait edge-to-client propagation simulator
        pthread_returncode = pthread_join(edge_toclient_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-client propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait edge-to-edge propagation simulator
        pthread_returncode = pthread_join(edge_toedge_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait edge-to-cloud propagation simulator
        pthread_returncode = pthread_join(edge_tocloud_propagation_simulator_thread_, NULL);
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << " failed to join edge-to-cloud propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for beacon server
        pthread_returncode = pthread_join(beacon_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join beacon server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server
        pthread_returncode = pthread_join(cache_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join cache server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // (5) Other utilities

    void EdgeWrapperBase::checkPointers_() const
    {
        NodeWrapperBase::checkPointers_();

        assert(edge_cache_ptr_ != NULL);
        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_toclient_propagation_simulator_param_ptr_ != NULL);
        assert(edge_toedge_propagation_simulator_param_ptr_ != NULL);
        assert(edge_tocloud_propagation_simulator_param_ptr_ != NULL);
        assert(local_cache_admission_buffer_ptr_ != NULL);

        return;
    }

    // (6) Common utility functions

    // (6.1) For local edge cache access

    // (6.2) For local directory admission

    // (6.3) For cache size usage

    uint64_t EdgeWrapperBase::getCacheMarginBytes() const
    {
        checkPointers_();

        // Get local edge margin bytes
        uint64_t used_bytes = getSizeForCapacity();

        uint64_t capacity_bytes = getCapacityBytes();
        uint64_t local_cache_margin_bytes = (capacity_bytes >= used_bytes) ? (capacity_bytes - used_bytes) : 0;

        return local_cache_margin_bytes;
    }
}
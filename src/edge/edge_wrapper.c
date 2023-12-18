#include "edge/edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "cache/covered_local_cache.h"
#include "common/config.h"
#include "common/thread_launcher.h"
#include "common/util.h"
#include "edge/beacon_server/beacon_server_base.h"
#include "edge/cache_server/cache_server.h"
#include "edge/invalidation_server/invalidation_server_base.h"
#include "edge/synchronization_server/covered_synchronization_server.h"
#include "event/event.h"
#include "message/data_message.h"
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

        EdgeWrapper local_edge(edge_cli_ptr->getCacheName(), edge_cli_ptr->getCapacityBytes(), edge_idx, edge_cli_ptr->getEdgecnt(), edge_cli_ptr->getHashName(), edge_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes(), edge_cli_ptr->getPercacheserverWorkercnt(), edge_cli_ptr->getCoveredPeredgeSyncedVictimcnt(), edge_cli_ptr->getCoveredPeredgeMonitoredVictimsetcnt(), edge_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes(), edge_cli_ptr->getCoveredPopularityCollectionChangeRatio(), edge_cli_ptr->getPropagationLatencyClientedgeUs(), edge_cli_ptr->getPropagationLatencyCrossedgeUs(), edge_cli_ptr->getPropagationLatencyEdgecloudUs(), edge_cli_ptr->getCoveredTopkEdgecnt());
        local_edge.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EdgeWrapper::EdgeWrapper(const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& edge_idx, const uint32_t& edgecnt, const std::string& hash_name, const uint64_t& local_uncached_capacity_bytes, const uint32_t& percacheserver_workercnt, const uint32_t& peredge_synced_victimcnt, const uint32_t& peredge_monitored_victimsetcnt, const uint64_t& popularity_aggregation_capacity_bytes, const double& popularity_collection_change_ratio, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& topk_edgecnt) : NodeWrapperBase(NodeWrapperBase::EDGE_NODE_ROLE, edge_idx,edgecnt, true), cache_name_(cache_name), capacity_bytes_(capacity_bytes), percacheserver_workercnt_(percacheserver_workercnt), propagation_latency_crossedge_us_(propagation_latency_crossedge_us), propagation_latency_edgecloud_us_(propagation_latency_edgecloud_us), topk_edgecnt_for_placement_(topk_edgecnt), edge_background_counter_for_beacon_server_(), weight_tuner_(edge_idx, edgecnt, propagation_latency_clientedge_us, propagation_latency_crossedge_us, propagation_latency_edgecloud_us)
    {
        // Get source address of beacon server recvreq for non-blocking placement deployment
        const bool is_launch_edge = true; // The beacon server belongs to the logical edge node launched in the current physical machine
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_launch_edge);
        uint16_t edge_beacon_server_recvreq_port = Util::getEdgeBeaconServerRecvreqPort(edge_idx, edgecnt);
        edge_beacon_server_recvreq_source_addr_for_placement_ = NetworkAddr(edge_ipstr, edge_beacon_server_recvreq_port);

        // Get destination address towards the corresponding cloud recvreq for non-blocking placement deployment
        const bool is_launch_cloud = false; // Just connect cloud by the logical edge node instead of launching the cloud
        std::string cloud_ipstr = Config::getCloudIpstr(is_launch_cloud);
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        corresponding_cloud_recvreq_dst_addr_for_placement_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = new CacheWrapper(this, cache_name, edge_idx, capacity_bytes, local_uncached_capacity_bytes, peredge_synced_victimcnt);
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

        // Allocate synchronization server param
        synchronization_server_param_ptr_ = new SynchronizationServerParam(this, Config::getEdgeCacheServerDataRequestBufferSize());
        assert(synchronization_server_param_ptr_ != NULL);

        // Allocate covered cache manager ONLY for COVERED
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            covered_cache_manager_ptr_ = new CoveredCacheManager(edge_idx, edgecnt, peredge_synced_victimcnt, peredge_monitored_victimsetcnt, popularity_aggregation_capacity_bytes, popularity_collection_change_ratio, topk_edgecnt);
            assert(covered_cache_manager_ptr_ != NULL);
        }
        else
        {
            covered_cache_manager_ptr_ = NULL;
        }

        // Sub-threads
        edge_toclient_propagation_simulator_thread_ = 0;
        edge_toedge_propagation_simulator_thread_ = 0;
        edge_tocloud_propagation_simulator_thread_ = 0;
        beacon_server_thread_ = 0;
        cache_server_thread_ = 0;
        invalidation_server_thread_ = 0;

        // Allocate ring buffer for local edge cache admissions
        const bool local_cache_admission_with_multi_providers = true; // Multiple providers (edge cache server workers after hybrid data fetching; edge cache server workers and edge beacon server for local placement notifications)
        local_cache_admission_buffer_ptr_ = new RingBuffer<LocalCacheAdmissionItem>(LocalCacheAdmissionItem(), Config::getEdgeCacheServerDataRequestBufferSize(), local_cache_admission_with_multi_providers);
        assert(local_cache_admission_buffer_ptr_ != NULL);
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // TMPDEBUG231211
        uint64_t edge_cache_size = edge_cache_ptr_->getSizeForCapacity();
        uint64_t cooperation_size = cooperation_wrapper_ptr_->getSizeForCapacity();
        uint64_t cache_manager_size = 0;
        uint64_t weight_tuner_size = 0;
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            cache_manager_size = covered_cache_manager_ptr_->getSizeForCapacity();
            weight_tuner_size = weight_tuner_.getSizeForCapacity();
        }
        std::ostringstream oss;
        oss << "edge_cache_size: " << edge_cache_size << ", cooperation_size: " << cooperation_size << ", cache_manager_size: " << cache_manager_size << ", weight_tuner_size: " << weight_tuner_size;
        Util::dumpDebugMsg(instance_name_, oss.str());

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

        // Release synchronization server param
        assert(synchronization_server_param_ptr_ != NULL);
        delete synchronization_server_param_ptr_;
        synchronization_server_param_ptr_ = NULL;

        // Release local cache admission ring buffer
        assert(local_cache_admission_buffer_ptr_ != NULL);
        delete local_cache_admission_buffer_ptr_;
        local_cache_admission_buffer_ptr_ = NULL;
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

    uint32_t EdgeWrapper::getPropagationLatencyCrossedgeUs() const
    {
        return propagation_latency_crossedge_us_;
    }

    uint32_t EdgeWrapper::getPropagationLatencyEdgecloudUs() const
    {
        return propagation_latency_edgecloud_us_;
    }

    uint32_t EdgeWrapper::getTopkEdgecntForPlacement() const
    {
        return topk_edgecnt_for_placement_;
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

    SynchronizationServerParam* EdgeWrapper::getSynchronizationServerParamPtr() const
    {
        assert(synchronization_server_param_ptr_ != NULL);
        return synchronization_server_param_ptr_;
    }

    CoveredCacheManager* EdgeWrapper::getCoveredCacheManagerPtr() const
    {
        // NOTE: non-COVERED caches should NOT call this function
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        assert(covered_cache_manager_ptr_ != NULL);
        return covered_cache_manager_ptr_;
    }

    BackgroundCounter& EdgeWrapper::getEdgeBackgroundCounterForBeaconServerRef()
    {
        return edge_background_counter_for_beacon_server_;
    }

    WeightTuner& EdgeWrapper::getWeightTunerRef()
    {
        return weight_tuner_;
    }

    RingBuffer<LocalCacheAdmissionItem>* EdgeWrapper::getLocalCacheAdmissionBufferPtr() const
    {
        assert(local_cache_admission_buffer_ptr_ != NULL);
        return local_cache_admission_buffer_ptr_;
    }

    // (2) Utility functions

    uint64_t EdgeWrapper::getSizeForCapacity() const
    {
        checkPointers_();

        uint64_t edge_cache_size = edge_cache_ptr_->getSizeForCapacity();
        uint64_t cooperation_size = cooperation_wrapper_ptr_->getSizeForCapacity();
        uint64_t cache_manager_size = 0;
        uint64_t weight_tuner_size = 0;
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            cache_manager_size = covered_cache_manager_ptr_->getSizeForCapacity();
            weight_tuner_size = weight_tuner_.getSizeForCapacity();
        }

        uint64_t size = edge_cache_size + cooperation_size + cache_manager_size + weight_tuner_size;

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

    NetworkAddr EdgeWrapper::getBeaconDstaddr_(const Key& key) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        MYASSERT(!currentIsBeacon(key));

        // Get remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqAddr(key);

        return beacon_edge_beacon_server_recvreq_dst_addr;
    }

    NetworkAddr EdgeWrapper::getBeaconDstaddr_(const uint32_t& beacon_edge_idx) const
    {
        checkPointers_();

        // The current edge node must NOT be the beacon node for the key
        assert(beacon_edge_idx != getNodeIdx());

        // Get remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqAddr(beacon_edge_idx);

        return beacon_edge_beacon_server_recvreq_dst_addr;
    }

    NetworkAddr EdgeWrapper::getTargetDstaddr(const DirectoryInfo& directory_info) const
    {
        checkPointers_();

        // The current edge node must NOT be the target node
        bool current_is_target = currentIsTarget(directory_info);
        assert(!current_is_target);

        // Set remote address such that the current edge node can communicate with the target edge node
        const bool is_launch_edge = false; // Just connect target edge node by the current edge node instead of launching the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx, getNodeCnt(), is_launch_edge);
        uint16_t target_edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(target_edge_idx, getNodeCnt());
        NetworkAddr target_edge_cache_server_recvreq_dst_addr(target_edge_ipstr, target_edge_cache_server_recvreq_port);

        return target_edge_cache_server_recvreq_dst_addr;
    }

    // (3) Invalidate and unblock for MSI protocol

    bool EdgeWrapper::parallelInvalidateCacheCopies(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const DirinfoSet& all_dirinfo, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
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
            const bool is_launch_edge = false; // Just connect neighbor to invalidate cache copies instead of launching the neighbor
            uint32_t tmp_edgeidx = iter->getTargetEdgeIdx();
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(tmp_edgeidx, node_cnt_, is_launch_edge);
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

                const uint32_t& tmp_dst_edge_idx = iter_for_request->first;
                const NetworkAddr& tmp_edge_invalidation_server_recvreq_dst_addr = percachecopy_dstaddr[tmp_dst_edge_idx]; // cache server address of a blocked closest edge node          
                sendInvalidationRequest_(key, recvrsp_source_addr, tmp_dst_edge_idx, tmp_edge_invalidation_server_recvreq_dst_addr, skip_propagation_latency);
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
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for InvalidationResponse");
                        break; // Break to resend the remaining control requests not acked yet
                    }
                } // End of (is_timeout == true)
                else
                {
                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL);
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

                            // Process invalidation response
                            processInvalidationResponse_(control_response_ptr);

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

    void EdgeWrapper::sendInvalidationRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const NetworkAddr& edge_invalidation_server_recvreq_dst_addr, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;
        MessageBase* invalidation_request_ptr = NULL;
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // Prepare victim syncset for piggybacking-based victim synchronization
            assert(dst_edge_idx_for_compression != edge_idx);
            VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, getCacheMarginBytes());

            // Prepare invalidation request to invalidate the cache copy
            invalidation_request_ptr = new CoveredInvalidationRequest(key, victim_syncset, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        }
        else // Baselines
        {
            // Prepare invalidation request to invalidate the cache copy
            invalidation_request_ptr = new InvalidationRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        }
        assert(invalidation_request_ptr != NULL);

        // Push invalidation request into edge-to-edge propagation simulator to send to neighbor edge node
        bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(invalidation_request_ptr, edge_invalidation_server_recvreq_dst_addr);
        assert(is_successful);

        // NOTE: invalidation_request_ptr will be released by edge-to-edge propagation simulator
        invalidation_request_ptr = NULL;

        return;
    }

    void EdgeWrapper::processInvalidationResponse_(MessageBase* invalidation_response_ptr) const
    {
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            assert(invalidation_response_ptr != NULL);
            assert(invalidation_response_ptr->getMessageType() == MessageType::kCoveredInvalidationResponse);

            const CoveredInvalidationResponse* covered_invalidation_response_ptr = static_cast<const CoveredInvalidationResponse*>(invalidation_response_ptr);
            const uint32_t source_edge_idx = covered_invalidation_response_ptr->getSourceIndex();

            // Victim synchronization
            const VictimSyncset& neighbor_victim_syncset = covered_invalidation_response_ptr->getVictimSyncsetRef();
            updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
        }
        else
        {
            assert(invalidation_response_ptr != NULL);
            assert(invalidation_response_ptr->getMessageType() == MessageType::kInvalidationResponse);
        }
        return;
    }

    bool EdgeWrapper::parallelNotifyEdgesToFinishBlock(UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, const Key& key, const std::unordered_set<NetworkAddr, NetworkAddrHasher>& blocked_edges, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
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

                // NOTE: dst edge idx to finish blocking MUST NOT be the current local/remote beacon edge node, as requests on a being-written object MUST poll instead of block if sender is beacon
                uint32_t tmp_dst_edge_idx = Util::getEdgeIdxFromCacheServerWorkerRecvreqAddr(tmp_edge_cache_server_worker_recvreq_dst_addr, getNodeCnt());
                assert(tmp_dst_edge_idx != node_idx_);
   
                // Issue finish block request to dst edge node
                sendFinishBlockRequest_(key, recvrsp_source_addr, tmp_dst_edge_idx, tmp_edge_cache_server_worker_recvreq_dst_addr, skip_propagation_latency);     
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
                        Util::dumpWarnMsg(instance_name_, "edge timeout to wait for FinishBlockResponse");
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

    void EdgeWrapper::sendFinishBlockRequest_(const Key& key, const NetworkAddr& recvrsp_source_addr, const uint32_t& dst_edge_idx_for_compression, const NetworkAddr& edge_cache_server_worker_recvreq_dst_addr, const bool& skip_propagation_latency) const
    {
        checkPointers_();

        uint32_t edge_idx = node_idx_;
        MessageBase* finish_block_request_ptr = NULL;
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // Prepare victim syncset for piggybacking-based victim synchronization
            assert(dst_edge_idx_for_compression != edge_idx);
            VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(dst_edge_idx_for_compression, getCacheMarginBytes());

            // Prepare finish block request to finish blocking for writes in all closest edge nodes
            finish_block_request_ptr = new CoveredFinishBlockRequest(key, victim_syncset, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        }
        else // Baselines
        {
            // Prepare finish block request to finish blocking for writes in all closest edge nodes
            finish_block_request_ptr = new FinishBlockRequest(key, edge_idx, recvrsp_source_addr, skip_propagation_latency);
        }
        assert(finish_block_request_ptr != NULL);

        // Push FinishBlockRequest into edge-to-edge propagation simulator to send to blocked edge node
        bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(finish_block_request_ptr, edge_cache_server_worker_recvreq_dst_addr);
        assert(is_successful);

        // NOTE: finish_block_request_ptr will be released by edge-to-edge propagation simulator
        finish_block_request_ptr = NULL;

        return;
    }

    void EdgeWrapper::processFinishBlockResponse_(MessageBase* finish_block_response_ptr) const
    {
        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            assert(finish_block_response_ptr != NULL);
            assert(finish_block_response_ptr->getMessageType() == MessageType::kCoveredFinishBlockResponse);

            const CoveredFinishBlockResponse* covered_finish_block_response_ptr = static_cast<const CoveredFinishBlockResponse*>(finish_block_response_ptr);
            const uint32_t source_edge_idx = covered_finish_block_response_ptr->getSourceIndex();

            // Victim synchronization
            const VictimSyncset& neighbor_victim_syncset = covered_finish_block_response_ptr->getVictimSyncsetRef();
            updateCacheManagerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset);
        }
        else
        {
            assert(finish_block_response_ptr != NULL);
            assert(finish_block_response_ptr->getMessageType() == MessageType::kFinishBlockResponse);
        }
        return;
    }

    // (4) Benchmark process

    void EdgeWrapper::initialize_()
    {
        checkPointers_();

        // Launch edge-to-client propagation simulator
        //int pthread_returncode = pthread_create(&edge_toclient_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-client propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        std::string tmp_thread_name = "edge-toclient-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &edge_toclient_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toclient_propagation_simulator_param_ptr_);

        // Launch edge-to-edge propagation simulator
        //pthread_returncode = pthread_create(&edge_toedge_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-edge propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-toedge-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &edge_toedge_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_toedge_propagation_simulator_param_ptr_);

        // Launch edge-to-cloud propagation simulator
        //pthread_returncode = pthread_create(&edge_tocloud_propagation_simulator_thread_, NULL, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << " failed to launch edge-to-cloud propagation simulator (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-tocloud-propagation-simulator-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &edge_tocloud_propagation_simulator_thread_, PropagationSimulator::launchPropagationSimulator, (void*)edge_tocloud_propagation_simulator_param_ptr_);

        // Launch beacon server
        //pthread_returncode = pthread_create(&beacon_server_thread_, NULL, BeaconServerBase::launchBeaconServer, (void*)(this));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch beacon server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-beacon-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &beacon_server_thread_, BeaconServerBase::launchBeaconServer, (void*)(this));

        // Launch cache server
        //pthread_returncode = pthread_create(&cache_server_thread_, NULL, CacheServer::launchCacheServer, (void*)(this));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch cache server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-cache-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &cache_server_thread_, CacheServer::launchCacheServer, (void*)(this));

        // Launch invalidation server
        //pthread_returncode = pthread_create(&invalidation_server_thread_, NULL, InvalidationServerBase::launchInvalidationServer, (void*)(this));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch invalidation server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-invalidation-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateLowPriority(tmp_thread_name, &invalidation_server_thread_, InvalidationServerBase::launchInvalidationServer, (void*)(this));

        // Launch synchronization server
        //pthread_returncode = pthread_create(&synchronization_server_thread_, NULL, CoveredSynchronizationServer::launchCoveredSynchronizationServer, (void*)(synchronization_server_param_ptr_));
        // if (pthread_returncode != 0)
        // {
        //     std::ostringstream oss;
        //     oss << "edge " << node_idx_ << " failed to launch synchronization server (error code: " << pthread_returncode << ")" << std::endl;
        //     Util::dumpErrorMsg(instance_name_, oss.str());
        //     exit(1);
        // }
        tmp_thread_name = "edge-synchronization-server-" + std::to_string(node_idx_);
        ThreadLauncher::pthreadCreateHighPriority(tmp_thread_name, &synchronization_server_thread_, CoveredSynchronizationServer::launchCoveredSynchronizationServer, (void*)(synchronization_server_param_ptr_));

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

        // Wait for synchronization server
        pthread_returncode = pthread_join(synchronization_server_thread_, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << node_idx_ << " failed to join synchronization server (error code: " << pthread_returncode << ")" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    // (5) Other utilities

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

    // (6.1) For local edge cache access

    bool EdgeWrapper::getLocalEdgeCache_(const Key& key, const bool& is_redirected, Value& value) const
    {
        checkPointers_();

        bool affect_victim_tracker = false; // If key was a local synced victim before or is a local synced victim now
        bool is_local_cached_and_valid = edge_cache_ptr_->get(key, is_redirected, value, affect_victim_tracker);

        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // Avoid unnecessary VictimTracker update by checking affect_victim_tracker
            updateCacheManagerForLocalSyncedVictims(affect_victim_tracker);
        }
        
        return is_local_cached_and_valid;
    }

    // (6.2) For local directory admission

    void EdgeWrapper::admitLocalDirectory_(const Key& key, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached, const bool& skip_propagation_latency) const
    {
        // Foreground local directory admission is triggered by independent admission
        // Background local directory admission is triggered by local placement notification at local/remote beacon edge node)

        checkPointers_();

        uint32_t current_edge_idx = getNodeIdx();
        const bool is_admit = true; // Admit content directory
        MetadataUpdateRequirement metadata_update_requirement;
        cooperation_wrapper_ptr_->updateDirectoryTable(key, current_edge_idx, is_admit, directory_info, is_being_written, is_neighbor_cached, metadata_update_requirement);

        if (cache_name_ == Util::COVERED_CACHE_NAME) // ONLY for COVERED
        {
            // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation -> while here NOT need piggyacking-based popularity collection and victim synchronization for local directory update

            // Issue metadata update request if necessary, update victim dirinfo, and clear preserved edgeset after local directory admission
            afterDirectoryAdmissionHelper_(key, current_edge_idx, metadata_update_requirement, directory_info, skip_propagation_latency);

            // NOTE: NO need to clear directory metadata cache, as key is a local-beaconed uncached object
        }

        return;
    }

    // (7) covered-specific utility functions (invoked by edge cache server or edge beacon server of closest/beacon edge node)

    // (7.1) For victim synchronization

    uint64_t EdgeWrapper::getCacheMarginBytes() const
    {
        checkPointers_();
        //assert(cache_name_ == Util::COVERED_CACHE_NAME);

        // Get local edge margin bytes
        uint64_t used_bytes = getSizeForCapacity();

        uint64_t capacity_bytes = getCapacityBytes();
        uint64_t local_cache_margin_bytes = (capacity_bytes >= used_bytes) ? (capacity_bytes - used_bytes) : 0;

        return local_cache_margin_bytes;
    }

    void EdgeWrapper::updateCacheManagerForLocalSyncedVictims(const bool& affect_victim_tracker) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);

        // Get local edge margin bytes
        uint64_t local_cache_margin_bytes = getCacheMarginBytes();

        if (affect_victim_tracker) // Need to update victim cacheinfos and dirinfos of local synced victims besides local cache margin bytes
        {
            // Get victim cacheinfos of local synced victims for the current edge node
            std::list<VictimCacheinfo> local_synced_victim_cacheinfos = edge_cache_ptr_->getLocalSyncedVictimCacheinfos(); // NOTE: victim cacheinfos from local edge cache MUST be complete

            // Update local synced victims for the current edge node
            covered_cache_manager_ptr_->updateVictimTrackerForLocalSyncedVictims(local_cache_margin_bytes, local_synced_victim_cacheinfos, cooperation_wrapper_ptr_);
        }
        else // ONLY update local cache margin bytes
        {
            // Update local cache margin bytes
            covered_cache_manager_ptr_->updateVictimTrackerForLocalCacheMarginBytes(local_cache_margin_bytes);
        }

        return;
    }

    void EdgeWrapper::updateCacheManagerForNeighborVictimSyncset(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset) const
    {
        #ifndef ENABLE_BACKGROUND_VICTIM_SYNCHRONIZATION
        // Foreground victim synchronization
        // NOTE: we always perform victim synchronization before popularity aggregation, as we need the latest synced victim information for placement calculation
        covered_cache_manager_ptr_->updateVictimTrackerForNeighborVictimSyncset(source_edge_idx, neighbor_victim_syncset, cooperation_wrapper_ptr_);
        #else
        // Background victim synchronization
        bool is_successful = getSynchronizationServerParamPtr()->getSynchronizationServerItemBufferPtr()->push(SynchronizationServerItem(source_edge_idx, neighbor_victim_syncset));
        assert(is_successful); // Ring buffer must not be full
        #endif

        return;
    }

    // (7.2) For non-blocking placement deployment (ONLY invoked by beacon edge node)

    //bool EdgeWrapper::nonblockDataFetchForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const
    void EdgeWrapper::nonblockDataFetchForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency, const bool& sender_is_beacon, bool& need_hybrid_fetching) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        need_hybrid_fetching = false;

        // Check local edge cache in local/remote beacon node first
        // NOTE: NOT update aggregated uncached popularity to avoid recursive placement calculation even if local uncached popularity is cached
        Value value;
        const bool is_redirected = !sender_is_beacon; // Approximately treat local control message as local cache access if sender is beacon, yet treat remote control message as remote cache access if sender is NOT beacon
        bool is_local_cached_and_valid = getLocalEdgeCache_(key, is_redirected, value);

        if (is_local_cached_and_valid) // Directly get valid value from local edge cache in local/remote beacon node
        {
            // Perform non-blocking placement notification for local data fetching
            nonblockNotifyForPlacement(key, value, best_placement_edgeset, skip_propagation_latency);
        }
        else // No valid value in local edge cache in local/remote beacon node
        {
            if (sender_is_beacon) // Sender and beacon is the same edge node
            {
                // NOTE: we always resort sender for hybrid data fetching if sender and beacon is the same edge node, due to zero extra latency between sender and beacon!
                need_hybrid_fetching = true;
            }
            else // Sender and beacon are two different dge nodes
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

                    // Send CoveredPlacementRedirectedGetRequest to the neighbor node
                    // NOTE: we use edge_beacon_server_recvreq_source_addr_ as the source address even if invoker is cache server to wait for responses
                    // (i) Although current is beacon for cache server, the role is still beacon node, which should be responsible for non-blocking placement deployment. And we don't want to resort cache server worker, which may degrade KV request processing
                    // (ii) Although we may wait for responses, beacon server is blocking for recvreq port and we don't want to introduce another blocking for recvrsp port
                    const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(directory_info.getTargetEdgeIdx(), getCacheMarginBytes());
                    CoveredPlacementRedirectedGetRequest* covered_placement_redirected_get_request_ptr = new CoveredPlacementRedirectedGetRequest(key, victim_syncset, best_placement_edgeset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, skip_propagation_latency);
                    assert(covered_placement_redirected_get_request_ptr != NULL);
                    NetworkAddr target_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(directory_info); // Send to cache server of the target edge node for cache server worker
                    bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_placement_redirected_get_request_ptr, target_edge_cache_server_recvreq_dst_addr);
                    assert(is_successful);
                    covered_placement_redirected_get_request_ptr = NULL; // NOTE: covered_placement_redirected_get_request_ptr will be released by edge-to-edge propagation simulator

                    // NOTE: CoveredPlacementRedirectedGetResponse will be processed by covered beacon server in the current edge node (current edge node is a remote beacon node for neighbor edge node, as neighbor MUST NOT be a different edge node for request redirection)
                }
                else // No valid cache copy in edge layer
                {
                    // We need hybrid fetching to resort the sender to fetch data from cloud for reducing edge-cloud bandwidth usage
                    // NOTE: extra latency between sender and beacon is limited compared with edge-cloud latency (i.e., w1 - w2 / w1)
                    need_hybrid_fetching = true;
                }
            }
        }

        return;
    }

    void EdgeWrapper::nonblockDataFetchFromCloudForPlacement(const Key& key, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        // NOTE: as we have replied the sender without hybrid data fetching before, beacon server directly fetches data from cloud by itself here in a non-blocking manner (this is a corner case, as valid dirinfo has cooperative hit in most time)

        // Send CoveredPlacementGlobalGetRequest to cloud
        // NOTE: we use edge_beacon_server_recvreq_source_addr_ as the source address even if the invoker (i.e., beacon server) is waiting for global responses
        // (i) Although wait for global responses, beacon server is blocking for recvreq port and we don't want to introduce another blocking for recvrsp port
        uint32_t current_edge_idx = getNodeIdx();
        CoveredPlacementGlobalGetRequest* covered_placement_global_get_request_ptr = new CoveredPlacementGlobalGetRequest(key, best_placement_edgeset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, skip_propagation_latency);
        assert(covered_placement_global_get_request_ptr != NULL);
        // Push the global request into edge-to-cloud propagation simulator to cloud
        bool is_successful = edge_tocloud_propagation_simulator_param_ptr_->push(covered_placement_global_get_request_ptr, corresponding_cloud_recvreq_dst_addr_for_placement_);
        assert(is_successful);
        covered_placement_global_get_request_ptr = NULL; // NOTE: covered_placement_global_get_request_ptr will be released by edge-to-cloud propagation simulator

        // NOTE: CoveredPlacementRedirectedGetResponse will be processed by covered beacon server in the current edge node

        return;
    }

    //bool EdgeWrapper::nonblockNotifyForPlacement(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const NetworkAddr& source_addr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const bool& skip_propagation_latency) const
    void EdgeWrapper::nonblockNotifyForPlacement(const Key& key, const Value& value, const Edgeset& best_placement_edgeset, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        assert(best_placement_edgeset.size() <= topk_edgecnt_for_placement_); // At most k placement edge nodes each time

        //bool is_finish = false;
        //BandwidthUsage total_bandwidth_usage;
        //EventList event_list;
        //const bool is_background = true;

        // Check writelock for validity of cache placement
        bool is_being_written = cooperation_wrapper_ptr_->isBeingWritten(key);
        bool is_valid = !is_being_written;

        // Send placement notification for each non-local edge node in best_placement_edgeset in a non-blocking manner
        const uint32_t current_edge_idx = getNodeIdx();
        std::unordered_set<uint32_t>::const_iterator edgeset_const_iter_for_local_notification = best_placement_edgeset.end();
        for (std::unordered_set<uint32_t>::const_iterator edgeset_const_iter_for_remote_notification = best_placement_edgeset.begin(); edgeset_const_iter_for_remote_notification != best_placement_edgeset.end(); edgeset_const_iter_for_remote_notification++)
        {
            const uint32_t& tmp_edge_idx = *edgeset_const_iter_for_remote_notification;
            if (tmp_edge_idx == current_edge_idx) // Skip local edge node
            {
                edgeset_const_iter_for_local_notification = edgeset_const_iter_for_remote_notification;
                continue;
            }

            // Send CoveredPlacementNotifyRequest to remote edge node
            // NOTE: source addr will NOT be used by placement processor of remote edge node due to without explicit notification ACK (we use directory update request with is_admit = true as the implicit ACK for placement notification)
            const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(tmp_edge_idx, getCacheMarginBytes());
            CoveredPlacementNotifyRequest* covered_placement_notify_request_ptr = new CoveredPlacementNotifyRequest(key, value, is_valid, victim_syncset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, skip_propagation_latency);
            assert(covered_placement_notify_request_ptr != NULL);

            // Push the global request into edge-to-edge propagation simulator to the remote edge node
            NetworkAddr remote_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(DirectoryInfo(tmp_edge_idx)); // Send to cache server of the target edge node for cache server placement processor
            bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_placement_notify_request_ptr, remote_edge_cache_server_recvreq_dst_addr);
            assert(is_successful);
            covered_placement_notify_request_ptr = NULL; // NOTE: covered_placement_notify_request_ptr will be released by edge-to-edge propagation simulator
        }

        // Local placement notification if necessary
        if (edgeset_const_iter_for_local_notification != best_placement_edgeset.end()) // If current edge node is also in best_placement_edgeset
        {
            // Current edge node MUST be the beacon node for the given key
            MYASSERT(currentIsBeacon(key));

            // Admit local directory information
            // NOTE: NO need to update aggregated uncached popularity due to admitting a cached object
            // NOTE: we cannot optimistically admit valid object into local edge cache first before admiting local dirinfo, as clients may get incorrect value if key is being written
            bool tmp_is_being_written = false;
            bool tmp_is_neighbor_cached = false; // NOTE: we directly use coooperation wrapper to get is_neighbor_cached, as target edge node for local placement notification MUST be beacon here
            admitLocalDirectory_(key, DirectoryInfo(current_edge_idx), tmp_is_being_written, tmp_is_neighbor_cached, skip_propagation_latency); // Local directory update for local placement notification
            if (tmp_is_being_written) // Double-check is_being_written to udpate is_valid if necessary
            {
                // NOTE: ONLY update is_valid if tmp_is_being_written is true; if tmp_is_being_written is false (i.e., key is NOT being written now), we still keep original is_valid, as the value MUST be stale if is_being_written was true before
                is_being_written = true;
                is_valid = false;
            }

            // NOTE: we need to notify placement processor of the current local/remote beacon edge node for non-blocking placement deployment of local placement notification to avoid blocking subsequent placement calculation (similar as CacheServerWorkerBase::notifyBeaconForPlacementAfterHybridFetch_() invoked by sender edge node)

            // Notify placement processor to admit local edge cache (NOTE: NO need to admit directory) and trigger local cache eviciton, to avoid blocking cache server worker / beacon server for subsequent placement calculation
            bool is_successful = getLocalCacheAdmissionBufferPtr()->push(LocalCacheAdmissionItem(key, value, tmp_is_neighbor_cached, is_valid, skip_propagation_latency));
            assert(is_successful);

            /* (OBSOLETE for non-blocking placement deployment)
            // Perform cache admission for local edge cache (equivalent to local placement notification)
            admitLocalEdgeCache_(key, value, is_valid);

            // Perform background cache eviction if necessary in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
            // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
            is_finish = evictForCapacity_(key, source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
            */
        }

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        //edge_background_counter_for_beacon_server_.updateBandwidthUsgae(total_bandwidth_usage);
        //edge_background_counter_for_beacon_server_.addEvents(event_list);

        //return is_finish;

        return;
    }

    // (7.3) For beacon-based cached metadata update (non-blocking notification-based)
    void EdgeWrapper::processMetadataUpdateRequirement(const Key& key, const MetadataUpdateRequirement& metadata_update_requirement, const bool& skip_propagation_latency) const
    {
        checkPointers_();
        assert(cache_name_ == Util::COVERED_CACHE_NAME);
        MYASSERT(currentIsBeacon(key));

        const bool is_from_single_to_multiple = metadata_update_requirement.isFromSingleToMultiple();
        const bool is_from_multiple_to_single = metadata_update_requirement.isFromMultipleToSingle();
        const uint32_t notify_edge_idx = metadata_update_requirement.getNotifyEdgeIdx();
        assert(!(is_from_single_to_multiple && is_from_multiple_to_single)); // Cannot be both true

        const bool need_update = is_from_single_to_multiple || is_from_multiple_to_single;
        if (need_update)
        {
            bool is_neighbor_cached = false;
            if (is_from_single_to_multiple)
            {
                is_neighbor_cached = true; // Enable is_neighbor_cached flag
            }
            else
            {
                is_neighbor_cached = false; // Disable is_neighbor_cached flag
            }

            const uint32_t current_edge_idx = getNodeIdx();
            if (notify_edge_idx == current_edge_idx) // If beacon itself is the first/last cache copy
            {
                // Directly enable/disable is_neighbor_cached flag locally
                edge_cache_ptr_->metadataUpdate(key, CoveredLocalCache::UPDATE_IS_NEIGHBOR_CACHED_FLAG_FUNC_NAME, &is_neighbor_cached);
            }
            else // The first/last cache copy is a neighbor edge node
            {
                // Prepare metadata update request
                // NOTE: edge_beacon_server_recvreq_source_addr_for_placement_ will NOT be used by edge cache server metadata update processor due to no response
                const VictimSyncset victim_syncset = covered_cache_manager_ptr_->accessVictimTrackerForLocalVictimSyncset(notify_edge_idx, getCacheMarginBytes());
                MessageBase* covered_metadata_update_request_ptr = new CoveredMetadataUpdateRequest(key, is_neighbor_cached, victim_syncset, current_edge_idx, edge_beacon_server_recvreq_source_addr_for_placement_, skip_propagation_latency);
                assert(covered_metadata_update_request_ptr != NULL);

                // Push the control request into edge-to-edge propagation simulator to the remote edge node
                NetworkAddr remote_edge_cache_server_recvreq_dst_addr = getTargetDstaddr(DirectoryInfo(notify_edge_idx)); // Send to cache server of the remote target edge node for cache server metadata update processor
                bool is_successful = edge_toedge_propagation_simulator_param_ptr_->push(covered_metadata_update_request_ptr, remote_edge_cache_server_recvreq_dst_addr);
                assert(is_successful);
                covered_metadata_update_request_ptr = NULL; // NOTE: covered_metadata_update_request_ptr will be released by edge-to-edge propagation simulator
            }
        }

        return;
    }

    // (7.4) Reward calculation for local reward and cache placement (delta reward of admission benefit / eviction cost)

    Reward EdgeWrapper::calcLocalCachedReward(const Popularity& local_cached_popularity, const Popularity& redirected_cached_popularity, const bool& is_last_copies) const
    {
        // Get current weight information from weight tuner
        WeightInfo cur_weight_info = weight_tuner_.getWeightInfo();
        const Weight local_hit_weight = cur_weight_info.getLocalHitWeight();
        const Weight cooperative_hit_weight = cur_weight_info.getCooperativeHitWeight();

        // Calculte local reward or eviction cost (i.e., decreased reward)
        Reward tmp_reward = 0.0;
        if (is_last_copies) // Key is the last cache copies
        {
            // Local cache hits become global cache misses for the victim edge node(s), while redirected cache hits become global cache misses for other edge nodes
            tmp_reward = static_cast<Reward>(Util::popularityMultiply(local_hit_weight, local_cached_popularity)) + static_cast<Reward>(Util::popularityMultiply(cooperative_hit_weight, redirected_cached_popularity)); // w1 * local_cached_popularity + w2 * redirected_cached_popularity
        }
        else // Key is NOT the last cache copies
        {
            // Local cache hits become redirected cache hits for the victim edge node(s)
            const Weight w1_minus_w2 = Util::popularityNonegMinus(local_hit_weight, cooperative_hit_weight);
            tmp_reward = static_cast<Reward>(Util::popularityMultiply(w1_minus_w2, local_cached_popularity)); // (w1 - w2) * local_cached_popularity
        }

        return tmp_reward;
    }

    Reward EdgeWrapper::calcLocalUncachedReward(const Popularity& local_uncached_popularity, const bool& is_global_cached, const Popularity& redirected_uncached_popularity) const
    {
        // Get current weight information from weight tuner
        WeightInfo cur_weight_info = weight_tuner_.getWeightInfo();
        const Weight local_hit_weight = cur_weight_info.getLocalHitWeight();
        const Weight cooperative_hit_weight = cur_weight_info.getCooperativeHitWeight();

        // Calculte local reward or admission benefit (i.e., increased reward)
        Reward tmp_reward = 0.0;
        if (is_global_cached) // Key is global cached
        {
            // Redirected cache hits become local cache hits for the placement edge node(s)
            const Weight w1_minus_w2 = Util::popularityNonegMinus(local_hit_weight, cooperative_hit_weight);
            tmp_reward = Util::popularityMultiply(w1_minus_w2, local_uncached_popularity); // w1 - w2

            #ifdef ENABLE_COMPLETE_DUPLICATION_AVOIDANCE_FOR_DEBUG
            tmp_reward = 0.0;
            #endif
        }
        else // Key is NOT global cached
        {
            // Global cache misses become local cache hits for the placement edge node(s)
            Reward tmp_reward0 = Util::popularityMultiply(local_hit_weight, local_uncached_popularity); // w1

            // Global cache misses become redirected cache hits for other edge nodes
            // NOTE: redirected_uncached_popularity MUST be zero for local reward of local uncached objects
            Reward tmp_reward1 = Util::popularityMultiply(cooperative_hit_weight, redirected_uncached_popularity); // w2

            tmp_reward = Util::popularityAdd(tmp_reward0, tmp_reward1);
        }

        return tmp_reward;
    }

    // (7.5) Helper functions after local/remote directory lookup/admission/eviction and writelock acquire/release

    bool EdgeWrapper::afterDirectoryLookupHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, FastPathHint* fast_path_hint_ptr, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool need_placement_calculation = true;
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency, fast_path_hint_ptr); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node (source = current if sender is beacon)
        if (is_finish)
        {
            return is_finish; // Edge node is NOT running now
        }

        // NOTE: (i) if sender is beacon, need_hybrid_fetching with best_placement_edgeset is processed in CacheServerWorkerBase::processLocalGetRequest_(), as we do NOT have value yet when lookuping directory information; (ii) otherwise, need_hybrid_fetching with best_placement_edgeset is processed in CoveredBeaconServer::getRspToLookupLocalDirectory_() to issue corresponding control response message for hybrid data fetching

        return is_finish;
    }

    void EdgeWrapper::afterDirectoryAdmissionHelper_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const bool& skip_propagation_latency) const
    {
        const bool is_admit = true;

        // Issue local/remote metadata update request for beacon-based cached metadata update if necessary (for local/remote directory admission)
        processMetadataUpdateRequirement(key, metadata_update_requirement, skip_propagation_latency);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim (here we update victim dirinfo in victim tracker before access popularity aggregator)
        covered_cache_manager_ptr_->updateVictimTrackerForLocalBeaconedVictimDirinfo(key, is_admit, directory_info);

        // Clear preserved edge nodes for the given key at the source edge node for metadata releasing after local/remote admission notification
        covered_cache_manager_ptr_->clearPopularityAggregatorForPreservedEdgesetAfterAdmission(key, source_edge_idx);

        return;
    }

    bool EdgeWrapper::afterDirectoryEvictionHelper_(const Key& key, const uint32_t& source_edge_idx, const MetadataUpdateRequirement& metadata_update_requirement, const DirectoryInfo& directory_info, const CollectedPopularity& collected_popularity, const bool& is_global_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const bool& is_background) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool is_admit = false;
        bool need_placement_calculation = !is_admit; // MUST be true here for is_admit = false
        if (is_background) // If local/remote directory eviction is triggered by background cache placement
        {
            // NOTE: DISABLE recursive cache placement
            need_placement_calculation = false;
        }
        const bool is_source_cached = false; // NOTE: source edge node MUST NOT cache the object after directory eviction for itself
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        // NOTE: here sender_is_beacon for non-blocking placement deployment focuses on the victim key, which is different from the key triggering eviction, e.g., update invalid/valid value by local get/put, indepdent admission, local placement notification at local/remote beacon edge node, remote placement nofication
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Issue local/remote metadata update request for beacon-based cached metadata update if necessary (for local/remote directory eviction)
        processMetadataUpdateRequirement(key, metadata_update_requirement, skip_propagation_latency);

        // Update directory info in victim tracker if the local beaconed key is a local/neighbor synced victim
        covered_cache_manager_ptr_->updateVictimTrackerForLocalBeaconedVictimDirinfo(key, is_admit, directory_info);

        // NOTE: MUST NO old local uncached popularity for the newly evicted object at the source edge node before popularity aggregation
        covered_cache_manager_ptr_->assertNoLocalUncachedPopularity(key, source_edge_idx);

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in source edge node

        if (is_background) // Background local/remote directory eviction (triggered by remote placement nofication and local placement notification at local/remote beacon edge node)
        {
            // NOTE: background local/remote directory eviction MUST NOT need hybrid data fetching due to NO need for placement calculation (we DISABLE recursive cache placement)
            assert(!is_finish);
            assert(best_placement_edgeset.size() == 0);
            assert(!need_hybrid_fetching);
        }

        return is_finish;
    }

    bool EdgeWrapper::afterWritelockAcquireHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        // NOTE: we do NOT perform placement calculation for local/remote acquire writelock request, as newly-admitted cache copies will still be invalid even after cache placement
        const bool need_placement_calculation = false;
        Edgeset best_placement_edgeset; // Used for non-blocking placement notification if need hybrid data fetching for COVERED
        bool need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
    
        // NOTE: local/remote acquire writelock MUST NOT need hybrid data fetching due to NO need for placement calculation (newly-admitted cache copies will still be invalid after cache placement)
        assert(!is_finish); // NO victim fetching due to NO placement calculation
        assert(best_placement_edgeset.size() == 0); // NO placement deployment due to NO placement calculation
        assert(!need_hybrid_fetching); // Also NO hybrid data fetching

        return is_finish;
    }

    bool EdgeWrapper::afterWritelockReleaseHelper_(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_source_cached, Edgeset& best_placement_edgeset, bool& need_hybrid_fetching, UdpMsgSocketServer* recvrsp_socket_server_ptr, const NetworkAddr& recvrsp_source_addr, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency) const
    {
        bool is_finish = false;

        // Prepare for selective popularity aggregation
        const bool is_global_cached = true; // NOTE: receiving local/remote release writelock means that the result of acquiring write lock is LockResult::kSuccess -> the given key MUST be global cached
        const bool need_placement_calculation = true;
        best_placement_edgeset.clear();
        need_hybrid_fetching = false;

        // If sender and beacon is the same edge node for placement calculation or NOT
        const bool sender_is_beacon = (source_edge_idx == getNodeIdx());

        // Selective popularity aggregation
        is_finish = covered_cache_manager_ptr_->updatePopularityAggregatorForAggregatedPopularity(key, source_edge_idx, collected_popularity, is_global_cached, is_source_cached, need_placement_calculation, sender_is_beacon, best_placement_edgeset, need_hybrid_fetching, this, recvrsp_source_addr, recvrsp_socket_server_ptr, total_bandwidth_usage, event_list, skip_propagation_latency); // Update aggregated uncached popularity, to add/update latest local uncached popularity or remove old local uncached popularity, for key in current edge node
        if (is_finish)
        {
            return is_finish; // Edge node is finished
        }
        
        return is_finish;
    }
}
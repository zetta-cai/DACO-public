#include "edge/cache_server/cache_server_placement_processor_base.h"

#include <assert.h>

#include "common/config.h"
#include "edge/cache_server/basic_cache_server_placement_processor.h"
#include "edge/cache_server/covered_cache_server_placement_processor.h"
#include "message/control_message.h"

namespace covered
{
    const std::string CacheServerPlacementProcessorBase::kClassName = "CacheServerPlacementProcessorBase";

    void* CacheServerPlacementProcessorBase::launchCacheServerPlacementProcessor(void* cache_server_placement_processor_param_ptr)
    {
        assert(cache_server_placement_processor_param_ptr != NULL);

        CacheServerProcessorParam* tmp_cache_server_processor_param_ptr = (CacheServerProcessorParam*)cache_server_placement_processor_param_ptr;
        const std::string cache_name = tmp_cache_server_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr()->getCacheName();
        CacheServerPlacementProcessorBase* cache_server_placement_processor_ptr = NULL;
        if (cache_name == Util::COVERED_CACHE_NAME)
        {
            cache_server_placement_processor_ptr = new CoveredCacheServerPlacementProcessor(tmp_cache_server_processor_param_ptr);
        }
        else
        {
            cache_server_placement_processor_ptr = new BasicCacheServerPlacementProcessor(tmp_cache_server_processor_param_ptr);
        }
        assert(cache_server_placement_processor_ptr != NULL);
        cache_server_placement_processor_ptr->start();

        delete cache_server_placement_processor_ptr;
        cache_server_placement_processor_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerPlacementProcessorBase::CacheServerPlacementProcessorBase(CacheServerProcessorParam* cache_server_placement_processor_param_ptr) : cache_server_placement_processor_param_ptr_(cache_server_placement_processor_param_ptr)
    {
        assert(cache_server_placement_processor_param_ptr != NULL);

        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr->getCacheServerPtr()->getEdgeWrapperPtr();
        const uint32_t edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        const uint32_t edgecnt = tmp_edge_wrapper_ptr->getNodeCnt();

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-placement-processor";
        base_instance_name_ = oss.str();

        // For receiving control responses

        // Get source address of cache server placement processor to receive control responses and redirected data responses
        const bool is_private_edge_ipstr = false; // NOTE: cross-edge communication for directory admission/eviction uses public IP address
        const bool is_launch_edge = true; // The edge cache server placement processor belongs to the logical edge node launched in the current physical machine
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt, is_private_edge_ipstr, is_launch_edge);
        uint16_t edge_cache_server_placement_processor_recvrsp_port = Util::getEdgeCacheServerPlacementProcessorRecvrspPort(edge_idx, edgecnt);
        edge_cache_server_placement_processor_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_placement_processor_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_placement_processor_recvrsp_port);
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
    }

    CacheServerPlacementProcessorBase::~CacheServerPlacementProcessorBase()
    {
        // NOTE: no need to release cache_server_placement_processor_param_ptr_, which will be released outside CacheServerPlacementProcessorBase (by CacheServer)

        // Release the socket server to receive control responses,
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_placement_processor_recvrsp_socket_server_ptr_;
        edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ = NULL;
    }

    void CacheServerPlacementProcessorBase::start()
    {
        checkPointers_();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr()->getEdgeWrapperPtr();

        // Notify edge cache server that the current edge cache server placement processor has finished initialization
        cache_server_placement_processor_param_ptr_->markFinishInitialization();

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the placement notification request from ring buffer partitioned by cache server
            CacheServerItem tmp_cache_server_item;
            bool is_successful_for_notify_placement = cache_server_placement_processor_param_ptr_->pop(tmp_cache_server_item, (void*)tmp_edge_wrapper_ptr);
            if (is_successful_for_notify_placement) // Receive an item for placement notification successfully
            {
                MessageBase* data_request_ptr = tmp_cache_server_item.getRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->getMessageType() == MessageType::kCoveredBgplacePlacementNotifyRequest || data_request_ptr->getMessageType() == MessageType::kBestGuessBgplacePlacementNotifyRequest) // Placement notification
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // A beacon edge node
                    is_finish = processPlacementNotifyRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
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
            } // End of (is_successful_for_notify_placement == true)

            // Try to get local cache admission from ring buffer pushed by edge cache server workers or edge beacon server
            LocalCacheAdmissionItem tmp_local_cache_admission_item;
            //bool is_successful_for_local_cache_admission = cache_server_placement_processor_param_ptr_->getLocalCacheAdmissionBufferPtr()->pop(tmp_local_cache_admission_item);
            bool is_successful_for_local_cache_admission = tmp_edge_wrapper_ptr->getLocalCacheAdmissionBufferPtr()->pop(tmp_local_cache_admission_item);
            if (is_successful_for_local_cache_admission) // Receive a local cache admission successfully
            {
                is_finish = processLocalCacheAdmission_(tmp_local_cache_admission_item);

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if edge is still running
                }
            } // End of (is_successful_for_local_cache_admission == true)
        } // End of while loop

        return;
    }

    bool CacheServerPlacementProcessorBase::processPlacementNotifyRequestInternal_(const Key& key, const Value& value, const bool& is_valid, BandwidthUsage& total_bandwidth_usage, EventList& event_list, const bool& skip_propagation_latency, const NetworkAddr& recvrsp_dst_addr)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        const bool is_background = true;

        struct timespec placement_notify_start_timestamp = Util::getCurrentTimespec();

        // Current edge node MUST NOT be the beacon node for the given key due to remote placement notification
        MYASSERT(!tmp_edge_wrapper_ptr->currentIsBeacon(key));

        // Issue directory update request with is_admit = true
        // NOTE: remote beacon edge node sends remote placement notification to notify the current edge node for edge cache admission, so we should use current edge index for directory admission instead of source edge index
        // NOTE: we cannot optimistically admit valid object into local edge cache first before issuing dirinfo admission request, as clients may get incorrect value if key is being written -> we admit object after directory admission to get is_being_written from beacon node
        const uint32_t current_edge_idx = tmp_edge_wrapper_ptr->getNodeIdx();
        bool is_being_written = false;
        bool is_neighbor_cached = false;
        is_finish = tmp_cache_server_ptr->admitBeaconDirectory_(key, DirectoryInfo(current_edge_idx), is_being_written, is_neighbor_cached, edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background);
        if (is_finish)
        {
            return is_finish;
        }

        bool tmp_is_valid = is_valid;
        if (is_being_written) // Double-check is_being_written to update is_valid if necessary
        {
            // NOTE: ONLY update is_valid if is_being_written is true; if is_being_written is false (i.e., key is NOT being written now), we still keep original is_valid, as the value MUST be stale if is_being_written was true before
            tmp_is_valid = false;
        }

        // Admit into local edge cache for the received remote placement notification
        tmp_cache_server_ptr->admitLocalEdgeCache_(key, value, is_neighbor_cached, tmp_is_valid); // May update local synced victims

        // Perform background cache eviction in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
        // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
        is_finish = tmp_cache_server_ptr->evictForCapacity_(key, edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // May update local synced victims

        struct timespec placement_notify_end_timestamp = Util::getCurrentTimespec();
        uint32_t placement_notify_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(placement_notify_end_timestamp, placement_notify_start_timestamp));
        event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_PLACEMENT_NOTIFY_EVENT_NAME, placement_notify_latency_us); // Add placement notify event if with event tracking

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    bool CacheServerPlacementProcessorBase::processLocalCacheAdmissionInternal_(const LocalCacheAdmissionItem& local_cache_admission_item)
    {
        checkPointers_();
        CacheServerBase* tmp_cache_server_ptr = cache_server_placement_processor_param_ptr_->getCacheServerPtr();
        EdgeWrapperBase* tmp_edge_wrapper_ptr = tmp_cache_server_ptr->getEdgeWrapperPtr();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;
        const bool is_background = true;

        struct timespec admission_start_timestamp = Util::getCurrentTimespec();

        // Admit into local edge cache for the received local cache admission decision
        const Key tmp_key = local_cache_admission_item.getKey();
        const Value tmp_value = local_cache_admission_item.getValue();
        const bool is_neighbor_cached = local_cache_admission_item.isNeighborCached();
        const bool is_valid = local_cache_admission_item.isValid();
        tmp_cache_server_ptr->admitLocalEdgeCache_(tmp_key, tmp_value, is_neighbor_cached, is_valid); // May update local synced victims

        // Perform background cache eviction in a blocking manner for consistent directory information (note that cache eviction happens after non-blocking placement notification)
        // NOTE: we update aggregated uncached popularity yet DISABLE recursive cache placement for metadata preservation during cache eviction
        const bool skip_propagation_latency = local_cache_admission_item.skipPropagationLatency();
        is_finish = tmp_cache_server_ptr->evictForCapacity_(tmp_key, edge_cache_server_placement_processor_recvrsp_source_addr_, edge_cache_server_placement_processor_recvrsp_socket_server_ptr_, total_bandwidth_usage, event_list, skip_propagation_latency, is_background); // May update local synced victims

        struct timespec admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(admission_end_timestamp, admission_start_timestamp));
        event_list.addEvent(Event::BG_EDGE_CACHE_SERVER_PLACEMENT_PROCESSOR_LOCAL_CACHE_ADMISSION_EVENT_NAME, admission_latency_us); // Add placement notify event if with event tracking

        // Get background eventlist and bandwidth usage to update background counter for beacon server
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().updateBandwidthUsgae(total_bandwidth_usage);
        tmp_edge_wrapper_ptr->getEdgeBackgroundCounterForBeaconServerRef().addEvents(event_list);

        return is_finish;
    }

    void CacheServerPlacementProcessorBase::checkPointers_() const
    {
        assert(cache_server_placement_processor_param_ptr_ != NULL);
        assert(edge_cache_server_placement_processor_recvrsp_socket_server_ptr_ != NULL);

        return;
    }
}
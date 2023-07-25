#include "edge/cache_server/cache_server_worker_base.h"

#include <assert.h>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "edge/cache_server/basic_cache_server_worker.h"
#include "edge/cache_server/covered_cache_server_worker.h"
#include "event/event.h"
#include "message/control_message.h"
#include "message/data_message.h"
#include "network/network_addr.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CacheServerWorkerBase::kClassName("CacheServerWorkerBase");

    void* CacheServerWorkerBase::launchCacheServerWorker(void* cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);

        CacheServerWorkerBase* cache_server_worker_ptr = getCacheServerWorkerByCacheName_((CacheServerWorkerParam*)cache_server_worker_param_ptr);
        assert(cache_server_worker_ptr != NULL);
        cache_server_worker_ptr->start();

        assert(cache_server_worker_ptr != NULL);
        delete cache_server_worker_ptr;
        cache_server_worker_ptr = NULL;

        pthread_exit(NULL);
        return NULL;
    }

    CacheServerWorkerBase* CacheServerWorkerBase::getCacheServerWorkerByCacheName_(CacheServerWorkerParam* cache_server_worker_param_ptr)
    {
        CacheServerWorkerBase* cache_server_worker_ptr = NULL;

        assert(cache_server_worker_param_ptr != NULL);
        std::string cache_name = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_->cache_name_;
        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            cache_server_worker_ptr = new CoveredCacheServerWorker(cache_server_worker_param_ptr);
        }
        else
        {
            cache_server_worker_ptr = new BasicCacheServerWorker(cache_server_worker_param_ptr);
        }

        assert(cache_server_worker_ptr != NULL);
        return cache_server_worker_ptr;
    }

    CacheServerWorkerBase::CacheServerWorkerBase(CacheServerWorkerParam* cache_server_worker_param_ptr) : cache_server_worker_param_ptr_(cache_server_worker_param_ptr)
    {
        assert(cache_server_worker_param_ptr != NULL);
        const uint32_t edge_idx = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_->edge_param_ptr_->getNodeIdx();
        const uint32_t edgecnt = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_->edgecnt_;
        const uint32_t local_cache_server_worker_idx = cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();
        const uint32_t percacheserver_workercnt = cache_server_worker_param_ptr->getCacheServerPtr()->edge_wrapper_ptr_->percacheserver_workercnt_;

        // Differentiate cache servers of different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx << "-worker" << cache_server_worker_param_ptr->getLocalCacheServerWorkerIdx();
        base_instance_name_ = oss.str();

        // Prepare destination address to the corresponding cloud
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(0); // TODO: only support 1 cloud node now!
        corresponding_cloud_recvreq_dst_addr_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // For receiving control responses and redirected data responses

        // Get source address of cache server worker to receive control responses and redirected data responses
        std::string edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
        uint16_t edge_cache_server_worker_recvrsp_port = Util::getEdgeCacheServerWorkerRecvrspPort(edge_idx, edgecnt, local_cache_server_worker_idx, percacheserver_workercnt);
        edge_cache_server_worker_recvrsp_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_worker_recvrsp_port);

        // Prepare a socket server to receive control responses and redirected data responses
        NetworkAddr recvrsp_host_addr(Util::ANY_IPSTR, edge_cache_server_worker_recvrsp_port);
        edge_cache_server_worker_recvrsp_socket_server_ptr_ = new UdpMsgSocketServer(recvrsp_host_addr);
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);

        // For receiving finish block requests

        // Get source address of cache server worker to receive finish block requests
        uint16_t edge_cache_server_worker_recvreq_port = Util::getEdgeCacheServerWorkerRecvreqPort(edge_idx, edgecnt, local_cache_server_worker_idx, percacheserver_workercnt);
        edge_cache_server_worker_recvreq_source_addr_ = NetworkAddr(edge_ipstr, edge_cache_server_worker_recvreq_port);

        // Prepare a socket server to receive finish block requests
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, edge_cache_server_worker_recvreq_port);
        edge_cache_server_worker_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
    }

    CacheServerWorkerBase::~CacheServerWorkerBase()
    {
        // NOTE: no need to release cache_server_worker_param_ptr_, which will be released outside CacheServerWorkerBase (by CacheServer)

        // Release the socket server to receive control responses, redirected data responses, and global responses
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);
        delete edge_cache_server_worker_recvrsp_socket_server_ptr_;
        edge_cache_server_worker_recvrsp_socket_server_ptr_ = NULL;

        // Release the socket server to receive finish block requests
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
        delete edge_cache_server_worker_recvreq_socket_server_ptr_;
        edge_cache_server_worker_recvreq_socket_server_ptr_ = NULL;
    }

    void CacheServerWorkerBase::start()
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false; // Mark if edge node is finished
        while (tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning()) // edge_running_ is set as true by default
        {
            // Try to get the data request from ring buffer partitioned by cache server
            CacheServerWorkerItem tmp_cache_server_worker_item;
            bool is_successful = cache_server_worker_param_ptr_->getDataRequestBufferPtr()->pop(tmp_cache_server_worker_item);
            if (!is_successful)
            {
                continue; // Retry to receive an item if edge is still running
            } // End of (is_successful == true)
            else
            {
                MessageBase* data_request_ptr = tmp_cache_server_worker_item.getDataRequestPtr();
                assert(data_request_ptr != NULL);

                if (data_request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    NetworkAddr recvrsp_dst_addr = data_request_ptr->getSourceAddr(); // client worker or cache server worker
                    is_finish = processDataRequest_(data_request_ptr, recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(base_instance_name_, oss.str());
                    exit(1);
                }

                // Release data request by the corresponding cache server worker thread
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

    // (1) Process data requests

    bool CacheServerWorkerBase::processDataRequest_(MessageBase* data_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(data_request_ptr != NULL && data_request_ptr->isDataRequest());
        assert(recvrsp_dst_addr.isValidAddr());

        bool is_finish = false; // Mark if edge node is finished

        if (data_request_ptr->isLocalRequest()) // Local request
        {
            if (data_request_ptr->getMessageType() == MessageType::kLocalGetRequest)
            {
                is_finish = processLocalGetRequest_(data_request_ptr, recvrsp_dst_addr);
            }
            else // Local put/del request
            {
                is_finish = processLocalWriteRequest_(data_request_ptr, recvrsp_dst_addr);
            }
        }
        else if (data_request_ptr->isRedirectedRequest()) // Redirected request
        {
            is_finish = processRedirectedRequest_(data_request_ptr, recvrsp_dst_addr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(data_request_ptr->getMessageType()) << " for processDataRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    bool CacheServerWorkerBase::processLocalGetRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr) const
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL && local_request_ptr->getMessageType() == MessageType::kLocalGetRequest);
        assert(recvrsp_dst_addr.isValidAddr());
        const LocalGetRequest* const local_get_request_ptr = static_cast<const LocalGetRequest*>(local_request_ptr);
        Key tmp_key = local_get_request_ptr->getKey();
        Value tmp_value;

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "receive a local get request;", "type:", MessageBase::messageTypeToString(local_request_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false; // Mark if edge node is finished
        Hitflag hitflag = Hitflag::kGlobalMiss;
        EventList event_list;

        // Access local edge cache (current edge node is the closest edge node)
        struct timespec get_local_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_local_cached_and_valid = tmp_edge_wrapper_ptr->edge_cache_ptr_->get(tmp_key, tmp_value);
        if (is_local_cached_and_valid) // local cached and valid
        {
            hitflag = Hitflag::kLocalHit;
        }
        struct timespec get_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t get_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_local_cache_end_timestamp, get_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_LOCAL_CACHE_EVENT_NAME, get_local_cache_latency_us); // Add intermediate event if with event tracking

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "acesss local edge cache;", "is_local_cached_and_valid:", Util::toString(is_local_cached_and_valid).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        // Access cooperative edge cache for local cache miss or invalid object
        struct timespec get_cooperative_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_cooperative_cached_and_valid = false;
        if (!is_local_cached_and_valid) // not local cached or invalid
        {
            // Get data from some target edge node for local cache miss (add events of intermediate responses if with event tracking)
            is_finish = fetchDataFromNeighbor_(tmp_key, tmp_value, is_cooperative_cached_and_valid, event_list);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
            if (is_cooperative_cached_and_valid) // cooperative cached and valid
            {
                hitflag = Hitflag::kCooperativeHit;
            }
        }
        struct timespec get_cooperative_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t get_cooperative_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_cooperative_cache_end_timestamp, get_cooperative_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_COOPERATIVE_CACHE_EVENT_NAME, get_cooperative_cache_latency_us); // Add intermediate event if with event tracking

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "acesss cooperative edge cache;", "is_cooperative_cached_and_valid:", Util::toString(is_cooperative_cached_and_valid).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision

        // Get data from cloud for global cache miss
        struct timespec get_cloud_start_timestamp = Util::getCurrentTimespec();
        if (!is_local_cached_and_valid && !is_cooperative_cached_and_valid) // (not cached or invalid) in both local and cooperative cache
        {
            is_finish = fetchDataFromCloud_(tmp_key, tmp_value, event_list);
            if (is_finish) // Edge node is NOT running
            {
                return is_finish;
            }
        }
        struct timespec get_cloud_end_timestamp = Util::getCurrentTimespec();
        uint32_t get_cloud_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(get_cloud_end_timestamp, get_cloud_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_GET_CLOUD_EVENT_NAME, get_cloud_latency_us); // Add intermediate event if with event tracking

        // Update invalid object of local edge cache if necessary
        struct timespec update_invalid_local_cache_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToUpdateInvalidLocalEdgeCache_(tmp_key, tmp_value, event_list); // Add events of intermediate response if with event tracking
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec update_invalid_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_invalid_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_invalid_local_cache_end_timestamp, update_invalid_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_UPDATE_INVALID_LOCAL_CACHE_EVENT_NAME, update_invalid_local_cache_latency_us); // Add intermediate event if with event tracking

        // Trigger independent cache admission for local/global cache miss if necessary
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value, event_list); // Add events of intermediate responses if with event tracking
        if (is_finish)
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Prepare LocalGetResponse for client
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_cache_server_recvreq_source_addr_;
        MessageBase* local_get_response_ptr = new LocalGetResponse(tmp_key, tmp_value, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, event_list);
        assert(local_get_response_ptr != NULL);

        // Push local response message into edge-to-client propagation simulator to a client
        bool is_successful = tmp_edge_wrapper_ptr->edge_toclient_propagation_simulator_param_ptr_->push(local_get_response_ptr, recvrsp_dst_addr);
        assert(is_successful);

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 5, "issue a local response;", "type:", MessageBase::messageTypeToString(local_get_response_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str());
        #endif

        // NOTE: local_get_response_ptr will be released by edge-to-client propagation simulator
        local_get_response_ptr = NULL;

        return is_finish;
    }

    bool CacheServerWorkerBase::processLocalWriteRequest_(MessageBase* local_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        // Get key and value from local request if any
        assert(local_request_ptr != NULL);
        assert(local_request_ptr->getMessageType() == MessageType::kLocalPutRequest || local_request_ptr->getMessageType() == MessageType::kLocalDelRequest);
        assert(recvrsp_dst_addr.isValidAddr());
        Key tmp_key;
        Value tmp_value;
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            const LocalPutRequest* const local_put_request_ptr = static_cast<const LocalPutRequest*>(local_request_ptr);
            tmp_key = local_put_request_ptr->getKey();
            tmp_value = local_put_request_ptr->getValue();
            assert(tmp_value.isDeleted() == false);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            const LocalDelRequest* const local_del_request_ptr = static_cast<const LocalDelRequest*>(local_request_ptr);
            tmp_key = local_del_request_ptr->getKey();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(local_request_ptr->getMessageType()) << " for processLocalWriteRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 9, "receive a local write request;", "type:", MessageBase::messageTypeToString(local_request_ptr->getMessageType()).c_str(), "keystr:", tmp_key.getKeystr().c_str(), "valuesize:", std::to_string(tmp_value.getValuesize()).c_str(), "is deleted:", Util::toString(tmp_value.isDeleted()).c_str());
        #endif
        
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false; // Mark if edge node is finished
        const Hitflag hitflag = Hitflag::kGlobalMiss; // Must be global miss due to write-through policy
        EventList event_list;

        // Acquire write lock from beacon node no matter the locally cached object is valid or not, where the beacon will invalidate all other cache copies for cache coherence
        struct timespec acquire_writelock_start_timestamp = Util::getCurrentTimespec();
        LockResult lock_result = LockResult::kFailure;
        is_finish = acquireWritelock_(tmp_key, lock_result, event_list);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        else
        {
            assert(lock_result == LockResult::kSuccess || lock_result == LockResult::kNoneed);
        }
        struct timespec acquire_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_writelock_end_timestamp, acquire_writelock_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_WRITELOCK_EVENT_NAME, acquire_writelock_latency_us); // Add intermediate event if with event tracking

        // Send request to cloud for write-through policy
        struct timespec write_cloud_start_timestamp = Util::getCurrentTimespec();
        is_finish = writeDataToCloud_(tmp_key, tmp_value, local_request_ptr->getMessageType(), event_list);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        struct timespec write_cloud_end_timestamp = Util::getCurrentTimespec();
        uint32_t write_cloud_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(write_cloud_end_timestamp, write_cloud_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_WRITE_CLOUD_EVENT_NAME, write_cloud_latency_us); // Add intermediate event if with event tracking

        // Try to update/remove local edge cache
        struct timespec write_local_cache_start_timestamp = Util::getCurrentTimespec();
        bool is_local_cached = false;
        uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
        NetworkAddr edge_cache_server_recvreq_source_addr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_cache_server_recvreq_source_addr_;
        // NOTE: message type has been checked, which must be one of the following two types
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            // Add events of intermediate response if with event tracking
            is_finish = updateLocalEdgeCache_(tmp_key, tmp_value, is_local_cached, event_list);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            removeLocalEdgeCache_(tmp_key, is_local_cached);
        }
        UNUSED(is_local_cached);
        struct timespec write_local_cache_end_timestamp = Util::getCurrentTimespec();
        uint32_t write_local_cache_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(write_local_cache_end_timestamp, write_local_cache_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_WRITE_LOCAL_CACHE_EVENT_NAME, write_local_cache_latency_us); // Add intermediate event if with event tracking

        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }

        // Trigger independent cache admission for local/global cache miss if necessary
        struct timespec independent_admission_start_timestamp = Util::getCurrentTimespec();
        is_finish = tryToTriggerIndependentAdmission_(tmp_key, tmp_value, event_list);
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        struct timespec independent_admission_end_timestamp = Util::getCurrentTimespec();
        uint32_t independent_admission_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(independent_admission_end_timestamp, independent_admission_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_INDEPENDENT_ADMISSION_EVENT_NAME, independent_admission_latency_us); // Add intermediate event if with event tracking

        // Notify beacon node to finish writes if acquiring write lock successfully
        if (lock_result == LockResult::kSuccess)
        {
            struct timespec release_writelock_start_timestamp = Util::getCurrentTimespec();

            is_finish = releaseWritelock_(tmp_key, event_list);

            // Add intermediate event if with event tracking
            struct timespec release_writelock_end_timestamp = Util::getCurrentTimespec();
            uint32_t release_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_writelock_end_timestamp, release_writelock_start_timestamp));
            event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_RELEASE_WRITELOCK_EVENT_NAME, release_writelock_latency_us);
        }
        if (is_finish) // Edge node is NOT running
        {
            return is_finish;
        }
        
        // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision

        // Prepare local response
        MessageBase* local_response_ptr = NULL;
        if (local_request_ptr->getMessageType() == MessageType::kLocalPutRequest)
        {
            // Prepare LocalPutResponse for client
            local_response_ptr = new LocalPutResponse(tmp_key, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, event_list);
        }
        else if (local_request_ptr->getMessageType() == MessageType::kLocalDelRequest)
        {
            // Prepare LocalDelResponse for client
            local_response_ptr = new LocalDelResponse(tmp_key, hitflag, edge_idx, edge_cache_server_recvreq_source_addr, event_list);
        }

        if (!is_finish) // // Edge node is STILL running
        {
            // Push local response message into edge-to-client propagation simulator to a client
            assert(local_response_ptr != NULL);
            assert(local_response_ptr->getMessageType() == MessageType::kLocalPutResponse || local_response_ptr->getMessageType() == MessageType::kLocalDelResponse);
            bool is_successful = tmp_edge_wrapper_ptr->edge_toclient_propagation_simulator_param_ptr_->push(local_response_ptr, recvrsp_dst_addr);
            assert(is_successful);
        }

        // NOTE: local_response_ptr will be released by edge-to-client propagation simulator
        local_response_ptr = NULL;

        return is_finish;
    }

    bool CacheServerWorkerBase::processRedirectedRequest_(MessageBase* redirected_request_ptr, const NetworkAddr& recvrsp_dst_addr)
    {
        assert(redirected_request_ptr != NULL && redirected_request_ptr->isRedirectedRequest());

        bool is_finish = false; // Mark if local edge node is finished

        MessageType message_type = redirected_request_ptr->getMessageType();
        if (message_type == MessageType::kRedirectedGetRequest)
        {
            is_finish = processRedirectedGetRequest_(redirected_request_ptr, recvrsp_dst_addr);
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for processRedirectedRequest_()!";
            Util::dumpErrorMsg(base_instance_name_, oss.str());
            exit(1);
        }
        
        return is_finish;
    }

    // (2) Access cooperative edge cache

    // (2.1) Fetch data from neighbor edge nodes

    bool CacheServerWorkerBase::fetchDataFromNeighbor_(const Key& key, Value& value, bool& is_cooperative_cached_and_valid, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;

        // Update remote address of edge_cache_server_worker_sendreq_tocloud_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);

        #ifdef DEBUG_CACHE_SERVER
        Util::dumpVariablesForDebug(base_instance_name_, 4, "current_is_beacon:", Util::toString(current_is_beacon).c_str(), "keystr:", key.getKeystr().c_str());
        #endif

        // Check if beacon node is the current edge node and lookup directory information
        struct timespec lookup_directory_start_timestamp = Util::getCurrentTimespec();
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        while (true) // Wait for valid directory after writes by polling or interruption
        {
            if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            is_being_written = false;
            is_valid_directory_exist = false;
            if (current_is_beacon) // Get target edge index from local directory information
            {
                // Frequent polling
                tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->lookupLocalDirectoryByCacheServer(key, is_being_written, is_valid_directory_exist, directory_info);
                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    continue; // Continue to lookup local directory info
                }
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                is_finish = lookupBeaconDirectory_(key, is_being_written, is_valid_directory_exist, directory_info, event_list); // Add events of intermediate responses if with event tracking
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate DirectoryLookupRequest
                    is_finish = blockForWritesByInterruption_(key, event_list); // Add intermediate event if with event tracking
                    if (is_finish) // Edge is NOT running
                    {
                        return is_finish;
                    }
                    else
                    {
                        continue; // Continue to lookup remote directory info
                    }
                }
            } // End of current_is_beacon

            // key must NOT being written here
            assert(!is_being_written);

            // Add intermediate event if with event tracking
            // NOTE: too large latency to lookup local directory may indicate polling for writes, while too large latency to lookup remote directory may indicate blocking for writes
            struct timespec lookup_directory_end_timestamp = Util::getCurrentTimespec();
            uint32_t lookup_directory_latency_us = Util::getDeltaTimeUs(lookup_directory_end_timestamp, lookup_directory_start_timestamp);
            event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_LOCAL_DIRECTORY_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_LOOKUP_REMOTE_DIRECTORY_EVENT_NAME, lookup_directory_latency_us);

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 4, "is_valid_directory_exist:", Util::toString(is_valid_directory_exist).c_str(), "keystr:", key.getKeystr().c_str());
            #endif

            if (is_valid_directory_exist) // The object is cached by some target edge node
            {
                struct timespec redirect_get_start_timestamp = Util::getCurrentTimespec();

                // NOTE: the target node should not be the current edge node, as CooperationWrapperBase::get() can only be invoked if is_local_cached = false (i.e., the current edge node does not cache the object and hence is_valid_directory_exist should be false)
                bool current_is_target = tmp_edge_wrapper_ptr->currentIsTarget_(directory_info);
                if (current_is_target)
                {
                    std::ostringstream oss;
                    oss << "current edge node " << directory_info.getTargetEdgeIdx() << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                    return is_finish; // NOTE: is_finish is still false, as edge is STILL running
                }

                // Get data from the target edge node if any and update is_cooperative_cached_and_valid
                bool is_cooperative_cached = false;
                bool is_valid = false;
                is_finish = redirectGetToTarget_(directory_info, key, value, is_cooperative_cached, is_valid, event_list); // Add events of intermediate responses if with event tracking
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                // Add intermediate event if with event tracking
                struct timespec redirect_get_end_timestamp = Util::getCurrentTimespec();
                uint32_t redirect_get_latency_us = Util::getDeltaTimeUs(redirect_get_end_timestamp, redirect_get_start_timestamp);
                event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_REDIRECT_GET_EVENT_NAME, redirect_get_latency_us);

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(base_instance_name_, 9, "issue redirected get request:", "target:", Util::toString(directory_info.getTargetEdgeIdx()).c_str(), "is_cooperative_cached:", Util::toString(is_cooperative_cached).c_str(), "is_valid", Util::toString(is_valid).c_str(), "keystr:", key.getKeystr().c_str());
                #endif

                // Check is_cooperative_cached and is_valid
                if (!is_cooperative_cached) // Target edge node does not cache the object
                {
                    assert(!is_valid);
                    is_cooperative_cached_and_valid = false; // Closest edge node will fetch data from cloud
                    break;
                }
                else if (is_valid) // Target edge node caches a valid object
                {
                    is_cooperative_cached_and_valid = true; // Closest edge node will directly reply clients
                    break;
                }
                else // Target edge node caches an invalid object
                {
                    lookup_directory_start_timestamp = Util::getCurrentTimespec(); // Reset start timestamp for the next round
                    continue; // Go back to look up local/remote directory info again
                }
            }
            else // The object is not cached by any target edge node
            {
                is_cooperative_cached_and_valid = false;
                break;
            }
        } // End of while (true)

        return is_finish;
    }

    // (2.2) Update content directory information

    bool CacheServerWorkerBase::updateDirectory_(const Key& key, const bool& is_admit, bool& is_being_written, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;
        struct timespec update_directory_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);

        // Check if beacon node is the current edge node and update directory information
        DirectoryInfo directory_info(tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx());
        if (current_is_beacon) // Update target edge index of local directory information
        {
            tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->updateLocalDirectory(key, is_admit, directory_info, is_being_written);
        }
        else // Update remote directory information at the beacon node
        {
            // Add events of intermediate responses if with event tracking
            is_finish = updateBeaconDirectory_(key, is_admit, directory_info, is_being_written, event_list);
        }

        // Add intermediate event if with event tracking
        struct timespec update_directory_end_timestamp = Util::getCurrentTimespec();
        uint32_t update_directory_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(update_directory_end_timestamp, update_directory_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_UPDATE_LOCAL_DIRECTORY_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_UPDATE_REMOTE_DIRECTORY_EVENT_NAME, update_directory_latency_us);

        return is_finish;
    }

    // (2.3) Process writes and block for MSI protocol

    bool CacheServerWorkerBase::acquireWritelock_(const Key& key, LockResult& lock_result, EventList& event_list)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;
        struct timespec acquire_local_or_remote_writelock_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);

        // Check if beacon node is the current edge node and acquire write permission
        std::unordered_set<DirectoryInfo, DirectoryInfoHasher> all_dirinfo;
        while (true) // Wait for write permission by polling or interruption
        {
            if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            if (current_is_beacon) // Get target edge index from local directory information
            {
                // Frequent polling
                lock_result = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->acquireLocalWritelockByCacheServer(key, all_dirinfo);
                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    continue; // Continue to try to acquire the write lock
                }
                else if (lock_result == LockResult::kSuccess) // If acquire write permission successfully
                {
                    // Invalidate all cache copies
                    tmp_edge_wrapper_ptr->invalidateCacheCopies_(edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, key, all_dirinfo, event_list); // Add events of intermediate response if with event tracking
                }
                // NOTE: will directly break if lock result is kNoneed
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                // Add events of intermediate response if with event tracking
                is_finish = acquireBeaconWritelock_(key, lock_result, event_list);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (lock_result == LockResult::kFailure) // If key has been locked by any other edge node
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate AcquireWritelockRequest
                    is_finish = blockForWritesByInterruption_(key, event_list);
                    if (is_finish) // Edge is NOT running
                    {
                        return is_finish;
                    }
                    else
                    {
                        continue; // Continue to try to acquire the write lock
                    }
                }
                // NOTE: If lock_result == kSuccess, beacon server of beacon node has already invalidated all cache copies -> NO need to invalidate them again in cache server
                // NOTE: will directly break if lock result is kNoneed
            } // End of current_is_beacon

            assert(lock_result == LockResult::kSuccess || lock_result == LockResult::kNoneed);
            break;
        } // End of while (true)

        // Add intermediate event if with event tracking
        struct timespec acquire_local_or_remote_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t acquire_local_or_remote_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(acquire_local_or_remote_writelock_end_timestamp, acquire_local_or_remote_writelock_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_LOCAL_WRITELOCK_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_ACQUIRE_REMOTE_WRITELOCK_EVENT_NAME, acquire_local_or_remote_writelock_latency_us);

        return is_finish;
    }

    bool CacheServerWorkerBase::blockForWritesByInterruption_(const Key& key, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // Closest edge node must NOT be the beacon node if with interruption
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        bool is_finish = false;
        struct timespec block_for_writes_start_timestamp = Util::getCurrentTimespec();

        NetworkAddr recvrsp_dstaddr; // cache server worker or beacon server
        while (true) // Wait for FinishBlockRequest from beacon node by interruption
        {
            // Receive the control repsonse from the beacon node
            DynamicArray control_request_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvreq_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning()) // Edge is not running
                {
                    is_finish = true;
                    break;
                }
                else // Retry to receive a message if edge is still running
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for FinishBlockRequest");
                    continue;
                }
            } // End of (is_timeout == true)
            else
            {
                // Receive the control request message successfully
                MessageBase* control_request_ptr = MessageBase::getResponseFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL && control_request_ptr->getMessageType() == MessageType::kFinishBlockRequest);
                recvrsp_dstaddr = control_request_ptr->getSourceAddr();

                // Get key from FinishBlockRequest
                const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_request_ptr);
                Key tmp_key = finish_block_request_ptr->getKey();

                // Release the control request message
                delete control_request_ptr;
                control_request_ptr = NULL;

                // Double-check if key matches
                if (key == tmp_key) // key matches
                {
                    break; // Break the while loop to send back FinishBlockResponse
                    
                }
                else // key NOT match
                {
                    std::ostringstream oss;
                    oss << "wait for key " << key.getKeystr() << " != received key " << tmp_key.getKeystr();
                    Util::dumpWarnMsg(base_instance_name_, oss.str());

                    continue; // Receive the next FinishBlockRequest
                }
            } // End of is_timeout
        } // End of while(true)

        if (!is_finish)
        {
            // Prepare FinishBlockResponse
            // NOTE: we just execute break to finish block, so no need to add an event for FinishBlockResponse
            uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
            MessageBase* finish_block_response_ptr = new FinishBlockResponse(key, edge_idx, edge_cache_server_worker_recvreq_source_addr_, EventList());
            assert(finish_block_response_ptr != NULL);

            // Push FinishBlockResponse into edge-to-edge propagation simulator to cache server worker or beacon server
            bool is_successful = tmp_edge_wrapper_ptr->edge_toedge_propagation_simulator_param_ptr_->push(finish_block_response_ptr, recvrsp_dstaddr);
            assert(is_successful);

            // NOTE: finish_block_response_ptr will be released by edge-to-edge propagation simulator
            finish_block_response_ptr = NULL;
        }

        // Add intermediate event if with event tracking
        struct timespec block_for_writes_end_timestamp = Util::getCurrentTimespec();
        uint32_t block_for_writes_latency_us = Util::getDeltaTimeUs(block_for_writes_end_timestamp, block_for_writes_start_timestamp);
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_BLOCK_FOR_WRITES_EVENT_NAME, block_for_writes_latency_us);

        return is_finish;
    }

    bool CacheServerWorkerBase::releaseWritelock_(const Key& key, EventList& event_list)
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;
        struct timespec release_local_or_remote_writelock_start_timestamp = Util::getCurrentTimespec();

        // Update remote address of edge_cache_server_worker_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);

        // Check if beacon node is the current edge node and lookup directory information
    
        if (current_is_beacon) // Get target edge index from local directory information
        {
            DirectoryInfo current_directory_info(tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx());
            std::unordered_set<NetworkAddr, NetworkAddrHasher> blocked_edges = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->releaseLocalWritelock(key, current_directory_info);

            is_finish = tmp_edge_wrapper_ptr->notifyEdgesToFinishBlock_(edge_cache_server_worker_recvrsp_socket_server_ptr_, edge_cache_server_worker_recvrsp_source_addr_, key, blocked_edges, event_list); // Add events of intermediate response if with event tracking
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        }
        else // Get target edge index from remote directory information at the beacon node
        {
            // NOTE: beacon server of beacon node has already notified all blocked edges -> NO need to notify them again in cache server
            is_finish = releaseBeaconWritelock_(key, event_list); // Add events of intermediate response if with event tracking
            if (is_finish) // Edge is NOT running
            {
                return is_finish;
            }
        } // End of current_is_beacon

        // Add intermediate event if with event tracking
        struct timespec release_local_or_remote_writelock_end_timestamp = Util::getCurrentTimespec();
        uint32_t release_local_or_remote_writelock_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(release_local_or_remote_writelock_end_timestamp, release_local_or_remote_writelock_start_timestamp));
        event_list.addEvent(current_is_beacon?Event::EDGE_CACHE_SERVER_WORKER_RELEASE_LOCAL_WRITELOCK_EVENT_NAME:Event::EDGE_CACHE_SERVER_WORKER_RELEASE_REMOTE_WRITELOCK_EVENT_NAME, release_local_or_remote_writelock_latency_us);

        return is_finish;
    }

    // (2.4) Utility functions for cooperative caching

    NetworkAddr CacheServerWorkerBase::getBeaconDstaddr_(const Key& key) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the beacon node for the key
        bool current_is_beacon = tmp_edge_wrapper_ptr->currentIsBeacon_(key);
        assert(!current_is_beacon);

        // Get remote address such that current edge node can communicate with the beacon node for the key
        NetworkAddr beacon_edge_beacon_server_recvreq_dst_addr = tmp_edge_wrapper_ptr->cooperation_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqAddr(key);

        return beacon_edge_beacon_server_recvreq_dst_addr;
    }

    NetworkAddr CacheServerWorkerBase::getTargetDstaddr_(const DirectoryInfo& directory_info) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        // The current edge node must NOT be the target node
        bool current_is_target = tmp_edge_wrapper_ptr->currentIsTarget_(directory_info);
        assert(!current_is_target);

        // Set remote address such that the current edge node can communicate with the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx, tmp_edge_wrapper_ptr->edgecnt_);
        uint16_t target_edge_cache_server_recvreq_port = Util::getEdgeCacheServerRecvreqPort(target_edge_idx, tmp_edge_wrapper_ptr->edgecnt_);
        NetworkAddr target_edge_cache_server_recvreq_dst_addr(target_edge_ipstr, target_edge_cache_server_recvreq_port);

        return target_edge_cache_server_recvreq_dst_addr;
    }

    // (3) Access cloud

    bool CacheServerWorkerBase::fetchDataFromCloud_(const Key& key, Value& value, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false; // Mark if edge node is finished
        struct timespec issue_global_get_req_start_timestamp = Util::getCurrentTimespec();

        while (true) // Timeout-and-retry
        {
            // Prepare global get request to cloud
            uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
            MessageBase* global_get_request_ptr = new GlobalGetRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
            assert(global_get_request_ptr != NULL);

            #ifdef DEBUG_CACHE_SERVER
            Util::dumpVariablesForDebug(base_instance_name_, 5, "issue a global request;", "type:", MessageBase::messageTypeToString(global_get_request_ptr->getMessageType()).c_str(), "keystr:", key.getKeystr().c_str());
            #endif

            // Push the global request into edge-to-cloud propagation simulator to cloud
            bool is_successful = tmp_edge_wrapper_ptr->edge_tocloud_propagation_simulator_param_ptr_->push(global_get_request_ptr, corresponding_cloud_recvreq_dst_addr_);
            assert(is_successful);

            // NOTE: global_get_request_ptr will be released by edge-to-cloud propagation simulator
            global_get_request_ptr = NULL;

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for GlobalGetResponse");
                    continue; // Resend the global request message
                }
            }
            else
            {
                // Receive the global response message successfully
                MessageBase* global_response_ptr = MessageBase::getResponseFromMsgPayload(global_response_msg_payload);
                assert(global_response_ptr != NULL && global_response_ptr->getMessageType() == MessageType::kGlobalGetResponse);
                
                // Get value from global response message
                const GlobalGetResponse* const global_get_response_ptr = static_cast<const GlobalGetResponse*>(global_response_ptr);
                value = global_get_response_ptr->getValue();

                // Add events of intermediate response if with event tracking
                event_list.addEvents(global_get_response_ptr->getEventListRef());

                #ifdef DEBUG_CACHE_SERVER
                Util::dumpVariablesForDebug(base_instance_name_, 5, "receive a global response", "type:", MessageBase::messageTypeToString(global_response_ptr->getMessageType()).c_str(), "keystr:", global_get_response_ptr->getKey().getKeystr().c_str());
                #endif

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop

        // Add intermediate event if with event tracking
        struct timespec issue_global_get_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_global_get_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_global_get_req_end_timestamp, issue_global_get_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_GET_REQ_EVENT_NAME, issue_global_get_req_latency_us);
        
        return is_finish;
    }

    bool CacheServerWorkerBase::writeDataToCloud_(const Key& key, const Value& value, const MessageType& message_type, EventList& event_list)
    {        
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false; // Mark if edge node is finished
        struct timespec issue_global_write_req_start_timestamp = Util::getCurrentTimespec();

        while (true) // Timeout-and-retry
        {
            // Prepare global write request message
            MessageBase* global_request_ptr = NULL;
            uint32_t edge_idx = tmp_edge_wrapper_ptr->edge_param_ptr_->getNodeIdx();
            if (message_type == MessageType::kLocalPutRequest)
            {
                global_request_ptr = new GlobalPutRequest(key, value, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
            }
            else if (message_type == MessageType::kLocalDelRequest)
            {
                global_request_ptr = new GlobalDelRequest(key, edge_idx, edge_cache_server_worker_recvrsp_source_addr_);
            }
            else
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(message_type) << " for writeDataToCloud_()!";
                Util::dumpErrorMsg(base_instance_name_, oss.str());
                exit(1);
            }
            assert(global_request_ptr != NULL);

            // Push the global request into edge-to-cloud propagation simulator to cloud
            bool is_successful = tmp_edge_wrapper_ptr->edge_tocloud_propagation_simulator_param_ptr_->push(global_request_ptr, corresponding_cloud_recvreq_dst_addr_);
            assert(is_successful);

            // NOTE: global_request_ptr will be released by edge-to-cloud propagation simulator
            global_request_ptr = NULL;

            // Receive the global response message from cloud
            DynamicArray global_response_msg_payload;
            bool is_timeout = edge_cache_server_worker_recvrsp_socket_server_ptr_->recv(global_response_msg_payload);
            if (is_timeout)
            {
                if (!tmp_edge_wrapper_ptr->edge_param_ptr_->isNodeRunning())
                {
                    is_finish = true;
                    break; // Edge is NOT running
                }
                else
                {
                    Util::dumpWarnMsg(base_instance_name_, "edge timeout to wait for GlobalPutResponse/GlobalDelResponse");
                    continue; // Resend the global request message
                }
            }
            else
            {
                // Receive the global response message successfully
                MessageBase* global_response_ptr = MessageBase::getResponseFromMsgPayload(global_response_msg_payload);
                assert(global_response_ptr != NULL);
                assert(global_response_ptr->getMessageType() == MessageType::kGlobalPutResponse || global_response_ptr->getMessageType() == MessageType::kGlobalDelResponse);

                // Add events of intermediate response if with event tracking
                event_list.addEvents(global_response_ptr->getEventListRef());

                // Release global response message
                delete global_response_ptr;
                global_response_ptr = NULL;
                break;
            }
        } // End of while loop

        // Add intermediate event if with event tracking
        struct timespec issue_global_write_req_end_timestamp = Util::getCurrentTimespec();
        uint32_t issue_global_write_req_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(issue_global_write_req_end_timestamp, issue_global_write_req_start_timestamp));
        event_list.addEvent(Event::EDGE_CACHE_SERVER_WORKER_ISSUE_GLOBAL_WRITE_REQ_EVENT_NAME, issue_global_write_req_latency_us);
        
        return is_finish;
    }

    // (4) Update cached objects in local edge cache

    bool CacheServerWorkerBase::tryToUpdateInvalidLocalEdgeCache_(const Key& key, const Value& value, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;
        
        bool is_local_cached = tmp_edge_wrapper_ptr->edge_cache_ptr_->isLocalCached(key);
        bool is_cached_and_invalid = false;
        if (is_local_cached)
        {
            bool is_valid = tmp_edge_wrapper_ptr->edge_cache_ptr_->isValidKeyForLocalCachedObject(key);
            is_cached_and_invalid = is_local_cached && !is_valid;
        }

        if (is_cached_and_invalid)
        {
            bool is_local_cached_after_udpate = false;
            if (value.isDeleted()) // value is deleted
            {
                removeLocalEdgeCache_(key, is_local_cached_after_udpate);
            }
            else // non-deleted value
            {
                // Add events of intermediate response if with event tracking
                is_finish = updateLocalEdgeCache_(key, value, is_local_cached_after_udpate, event_list);
            }
            assert(is_local_cached_after_udpate);
        }

        return is_finish;
    }

    bool CacheServerWorkerBase::updateLocalEdgeCache_(const Key& key, const Value& value, bool& is_local_cached_after_udpate, EventList& event_list) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_() or processLocalWriteRequest_()

        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;

        is_local_cached_after_udpate = tmp_edge_wrapper_ptr->edge_cache_ptr_->update(key, value);

        while (true) // Evict until used bytes <= capacity bytes
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint64_t used_bytes = tmp_edge_wrapper_ptr->getSizeForCapacity_();
            if (used_bytes <= tmp_edge_wrapper_ptr->capacity_bytes_) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                Key victim_key;
                Value victim_value;
                tmp_edge_wrapper_ptr->edge_cache_ptr_->evict(victim_key, victim_value);
                bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
                is_finish = updateDirectory_(victim_key, false, _unused_is_being_written, event_list);
                if (is_finish)
                {
                    return is_finish;
                }

                continue;
            }
        }

        return is_finish;
    }

    void CacheServerWorkerBase::removeLocalEdgeCache_(const Key& key, bool& is_local_cached_after_udpate) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        is_local_cached_after_udpate = tmp_edge_wrapper_ptr->edge_cache_ptr_->remove(key);

        // NOTE: no need to check capacity, as remove() only replaces the original value (value size + is_deleted) with a deleted value (zero value size + is_deleted of true), where deleted value uses minimum bytes and remove() cannot increase used bytes to trigger any eviction

        return;
    }

    // (5) Admit uncached objects in local edge cache

    bool CacheServerWorkerBase::tryToTriggerIndependentAdmission_(const Key& key, const Value& value, EventList& event_list) const
    {
        checkPointers_();
        EdgeWrapper* tmp_edge_wrapper_ptr = cache_server_worker_param_ptr_->getCacheServerPtr()->edge_wrapper_ptr_;

        bool is_finish = false;

        bool is_local_cached = tmp_edge_wrapper_ptr->edge_cache_ptr_->isLocalCached(key);
        if (!is_local_cached && tmp_edge_wrapper_ptr->edge_cache_ptr_->needIndependentAdmit(key))
        {
            is_finish = triggerIndependentAdmission_(key, value, event_list);
        }

        return is_finish;
    }

    // (6) Utility functions

    void CacheServerWorkerBase::checkPointers_() const
    {
        assert(cache_server_worker_param_ptr_ != NULL);
        assert(cache_server_worker_param_ptr_->getCacheServerPtr() != NULL);
        
        cache_server_worker_param_ptr_->getCacheServerPtr()->checkPointers_();

        assert(cache_server_worker_param_ptr_->getDataRequestBufferPtr() != NULL);
        assert(edge_cache_server_worker_recvrsp_socket_server_ptr_ != NULL);
        assert(edge_cache_server_worker_recvreq_socket_server_ptr_ != NULL);
    }
}
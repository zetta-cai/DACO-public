#include "cloud/data_server/data_server.h"

#include "common/bandwidth_usage.h"
#include "common/config.h"
#include "common/util.h"
#include "core/popularity/edgeset.h"
#include "message/data_message.h"

namespace covered
{
    const std::string DataServer::kClassName("DataServer");

    void* DataServer::launchDataServer(void* cloud_wrapper_ptr)
    {
        DataServer data_server((CloudWrapper*)cloud_wrapper_ptr);
        data_server.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    DataServer::DataServer(CloudWrapper* cloud_wrapper_ptr) : cloud_wrapper_ptr_(cloud_wrapper_ptr)
    {
        assert(cloud_wrapper_ptr != NULL);
        uint32_t cloud_idx = cloud_wrapper_ptr->getNodeIdx();
        uint32_t cloudcnt = cloud_wrapper_ptr->getNodeCnt();
        assert(cloud_idx == 0 && cloudcnt == 1); // TODO: only 1 cloud node now

        // Differentiate cache servers in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " cloud" << cloud_idx;
        instance_name_ = oss.str();

        // For receiving global requests

        // Get source address of cloud recvreq
        const bool is_private_cloud_ipstr = false; // NOTE: cloud communicates with edges via public IP address
        const bool is_launch_cloud = true; // The cloud data server belons to the logical cloud node launched in the current physical machine
        std::string cloud_ipstr = Config::getCloudIpstr(is_private_cloud_ipstr, is_launch_cloud);
        uint16_t cloud_recvreq_port = Util::getCloudRecvreqPort(cloud_idx);
        cloud_recvreq_source_addr_ = NetworkAddr(cloud_ipstr, cloud_recvreq_port);

        // Prepare a socket server on recvreq port
        NetworkAddr recvreq_host_addr(Util::ANY_IPSTR, cloud_recvreq_port);
        cloud_recvreq_socket_server_ptr_ = new UdpMsgSocketServer(recvreq_host_addr);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
    }

    DataServer::~DataServer()
    {
        // NO need to release cloud_wrapper_ptr_, which is released by CloudWrapper itself

        // Release the socket server on recvreq port
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
        delete cloud_recvreq_socket_server_ptr_;
        cloud_recvreq_socket_server_ptr_ = NULL;
    }

    void DataServer::start()
    {
        checkPointers_();

        bool is_finish = false; // Mark if local cloud node is finished
        while (cloud_wrapper_ptr_->isNodeRunning()) // cloud_running_ is set as true by default
        {
            // Receive the global request message
            DynamicArray global_request_msg_payload;
            bool is_timeout = cloud_recvreq_socket_server_ptr_->recv(global_request_msg_payload);
            if (is_timeout == true)
            {
                continue; // Retry to receive global request if cloud is still running
            } // End of (is_timeout == true)
            else
            {
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(global_request_msg_payload);
                assert(request_ptr != NULL);

                if (request_ptr->isGlobalDataRequest()) // Global requests
                {
                    NetworkAddr edge_cache_server_worker_recvrsp_dst_addr = request_ptr->getSourceAddr();
                    is_finish = processGlobalRequest_(request_ptr, edge_cache_server_worker_recvrsp_dst_addr);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << MessageBase::messageTypeToString(request_ptr->getMessageType()) << " for start()!";
                    Util::dumpErrorMsg(instance_name_, oss.str());
                    exit(1);
                }

                // Release messages
                assert(request_ptr != NULL);
                delete request_ptr;
                request_ptr = NULL;

                if (is_finish) // Check is_finish
                {
                    continue; // Go to check if cloud is still running
                }
            } // End of (is_timeout == false)
        } // End of while loop
        
        return;
    }

    bool DataServer::processGlobalRequest_(MessageBase* global_request_ptr, const NetworkAddr& edge_cache_server_worker_recvrsp_dst_addr)
    {
        checkPointers_();

        const uint32_t cloud_idx = cloud_wrapper_ptr_->getNodeIdx();

        bool is_finish = false;
        BandwidthUsage total_bandwidth_usage;
        EventList event_list;

        // Update total bandwidth usage for received global get/put/del request
        uint32_t edge_cloud_global_req_bandwidth_bytes = global_request_ptr->getMsgPayloadSize();
        total_bandwidth_usage.update(BandwidthUsage(0, 0, edge_cloud_global_req_bandwidth_bytes, 0, 0, 1));

        struct timespec access_rocksdb_start_timestamp = Util::getCurrentTimespec();

        // Process global requests by RocksDB KVS
        MessageType global_request_message_type = global_request_ptr->getMessageType();
        Key tmp_key;
        Value tmp_value;
        Edgeset tmp_placement_edgeset;
        const bool skip_propagation_latency = global_request_ptr->isSkipPropagationLatency();
        std::string event_name;
        switch (global_request_message_type)
        {
            case MessageType::kGlobalGetRequest:
            {
                const GlobalGetRequest* const global_get_request_ptr = static_cast<const GlobalGetRequest*>(global_request_ptr);
                tmp_key = global_get_request_ptr->getKey();

                // Get value from RocksDB KVS
                cloud_wrapper_ptr_->getCloudRocksdbPtr()->get(tmp_key, tmp_value);

                event_name = Event::CLOUD_GET_ROCKSDB_EVENT_NAME;
                break;
            }
            case MessageType::kGlobalPutRequest:
            {
                const GlobalPutRequest* const global_put_request_ptr = static_cast<const GlobalPutRequest*>(global_request_ptr);
                tmp_key = global_put_request_ptr->getKey();
                tmp_value = global_put_request_ptr->getValue();
                assert(tmp_value.isDeleted() == false);

                // Put value into RocksDB KVS
                cloud_wrapper_ptr_->getCloudRocksdbPtr()->put(tmp_key, tmp_value);

                event_name = Event::CLOUD_PUT_ROCKSDB_EVENT_NAME;
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                const GlobalDelRequest* const global_del_request_ptr = static_cast<const GlobalDelRequest*>(global_request_ptr);
                tmp_key = global_del_request_ptr->getKey();

                // Remove value from RocksDB KVS
                cloud_wrapper_ptr_->getCloudRocksdbPtr()->remove(tmp_key);

                event_name = Event::CLOUD_DEL_ROCKSDB_EVENT_NAME;
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetRequest: // ONLY used by COVERED
            {
                const CoveredPlacementGlobalGetRequest* const covered_placement_global_get_request_ptr = static_cast<const CoveredPlacementGlobalGetRequest*>(global_request_ptr);
                tmp_key = covered_placement_global_get_request_ptr->getKey();
                tmp_placement_edgeset = covered_placement_global_get_request_ptr->getEdgesetRef();

                // Get value from RocksDB KVS
                cloud_wrapper_ptr_->getCloudRocksdbPtr()->get(tmp_key, tmp_value);

                event_name = Event::BG_CLOUD_GET_ROCKSDB_EVENT_NAME;
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "invalid message type " << MessageBase::messageTypeToString(global_request_message_type) << " for processGlobalRequest_()!";
                Util::dumpErrorMsg(instance_name_, oss.str());
                exit(1);
            }
        }

        #ifdef DEBUG_DATA_SERVER
        std::ostringstream oss;
        oss << "receive a global request; message type: " << MessageBase::messageTypeToString(global_request_message_type) << "; keystr: " << tmp_key.getKeystr() << "; valuesize: " << tmp_value.getValuesize();
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        // Add intermediate event if with event tracking
        struct timespec access_rocksdb_end_timestamp = Util::getCurrentTimespec();
        uint32_t access_rocksdb_latency_us = static_cast<uint32_t>(Util::getDeltaTimeUs(access_rocksdb_end_timestamp, access_rocksdb_start_timestamp));
        event_list.addEvent(event_name, access_rocksdb_latency_us);

        if (is_finish) // Check is_finish
        {
            return is_finish;
        }

        // Prepare global response
        MessageBase* global_response_ptr = NULL;
        switch (global_request_message_type)
        {
            case MessageType::kGlobalGetRequest:
            {
                // Prepare global get response message
                global_response_ptr = new GlobalGetResponse(tmp_key, tmp_value, cloud_idx, cloud_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kGlobalPutRequest:
            {
                // Prepare global put response message
                global_response_ptr = new GlobalPutResponse(tmp_key, cloud_idx, cloud_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kGlobalDelRequest:
            {
                // Prepare global del response message
                global_response_ptr = new GlobalDelResponse(tmp_key, cloud_idx, cloud_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
                assert(global_response_ptr != NULL);
                break;
            }
            case MessageType::kCoveredPlacementGlobalGetRequest: // ONLY used by COVERED
            {
                // NOTE: NOT assert here as cloud does NOT need to know topk_edgecnt_
                //assert(tmp_placement_edgeset.size() <= topk_edgecnt_); // At most k placement edge nodes each time

                // Prepare covered placement global get response message
                global_response_ptr = new CoveredPlacementGlobalGetResponse(tmp_key, tmp_value, tmp_placement_edgeset, cloud_idx, cloud_recvreq_source_addr_, total_bandwidth_usage, event_list, skip_propagation_latency);
                assert(global_response_ptr != NULL);
                break;
            }
            default:
            {
                Util::dumpErrorMsg(instance_name_, "cannot arrive here!");
                exit(1);
            }
        }

        // Push the global response message into cloud-to-edge propagation simulator to edge cache server worker
        assert(global_response_ptr != NULL);
        assert(global_response_ptr->isGlobalDataResponse());
        bool is_successful = cloud_wrapper_ptr_->getCloudToedgePropagationSimulatorParamPtr()->push(global_response_ptr, edge_cache_server_worker_recvrsp_dst_addr);
        assert(is_successful);

        // NOTE: global_response_ptr will be released by cloud-to-edge propagation simulator
        global_response_ptr = NULL;

        return is_finish;
    }

    void DataServer::checkPointers_() const
    {
        assert(cloud_wrapper_ptr_ != NULL);
        assert(cloud_recvreq_socket_server_ptr_ != NULL);
        return;
    }
}
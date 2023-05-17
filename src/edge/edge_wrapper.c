#include "edge/edge_wrapper.h"

#include <assert.h>

#include "common/util.h"
#include "message/local_get_request.h"
#include "message/local_put_request.h"
#include "message/local_del_request.h"
#include "network/network_addr.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    const std::string EdgeWrapper::kClassName("EdgeWrapper");

    void* EdgeWrapper::launchEdge(void* local_edge_param_ptr)
    {
        EdgeWrapper local_edge((EdgeParam*)local_edge_param_ptr);
        local_edge.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EdgeWrapper::EdgeWrapper(EdgeParam* local_edge_param_ptr)
    {
        if (local_edge_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "local_edge_param_ptr is NULL!");
            exit(1);
        }
        local_edge_param_ptr_ = local_edge_param_ptr;
        
        // Allocate local edge cache
        local_edge_cache_ptr_ = CacheWrapperBase::getEdgeCache(Param::getCacheName(), Param::getCapacity());
        assert(local_edge_cache_ptr_ != NULL);
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // NOTE: no need to delete local_edge_param_ptr, as it is maintained outside EdgeWrapper

        // Free local edge cache
        assert(local_edge_param_ptr_ != NULL);
        delete local_edge_param_ptr_;
        local_edge_param_ptr_ = NULL;
    }

    void EdgeWrapper::start()
    {
        // Calculate local_edge_recvreq_port
        assert(local_edge_param_ptr_ != NULL);
        uint32_t global_edge_idx = local_edge_param_ptr_->getGlobalEdgeIdx();
        uint16_t local_edge_recvreq_port = Util::getLocalEdgeRecvreqPort(global_edge_idx);

        // Listen on local_edge_recvreq_port to receive request messages and reply response messages
        NetworkAddr host_addr(Util::ANY_IPSTR, local_edge_recvreq_port, true);
        UdpSocketWrapper edge_recvreq_socket_server(SocketRole::kSocketServer, host_addr);
        bool is_timeout = false;
        while (local_edge_param_ptr_->isEdgeRunning()) // local_edge_running_ is set as true by default
        {
            DynamicArray request_msg_payload;
            is_timeout = edge_recvreq_socket_server.recv(request_msg_payload);
            if (is_timeout == true)
            {
                if (!local_edge_param_ptr_->isEdgeRunning()) // Check local_edge_running_ to break
                {
                    break;
                }
                else
                {
                    continue;
                }
            } // End of (is_timeout == true)
            else
            {
                MessageBase* request_ptr = MessageBase::getRequestFromMsgPayload(request_msg_payload);
                assert(request_ptr != NULL);

                MessageBase* response_ptr = NULL;
                if (request_ptr->isDataRequest()) // Data requests (e.g., local/redirected requests)
                {
                    response_ptr = processDataRequest_(request_ptr);
                    assert(response_ptr != NULL && response_ptr->isDataResponse());
                }
                else if (request_ptr->isControlRequest()) // Control requests (e.g., invalidation and cache admission/eviction requests)
                {
                    response_ptr = processControlRequest_(request_ptr);
                    assert(response_ptr != NULL && response_ptr->isControlResponse());
                }
                else
                {
                    std::ostringstream oss;
                    oss << "invalid message type " << static_cast<uint32_t>(request_ptr->message_type_) << " for start()!";
                    Util::dumpErrorMsg(kClassName, oss.str());
                    exit(1);
                }

                // Reply response message to sender (the remote address set by the most recent recv)
                DynamicArray response_msg_payload(response_ptr->getMsgPayloadSize());
                response_ptr->serialize(response_msg_payload);
                edge_recvreq_socket_server.send(response_msg_payload);

                // Free messages
                assert(request_ptr != NULL);
                delete request_ptr;
                request_ptr = NULL;
                assert(response_ptr != NULL);
                delete response_ptr;
                response_ptr = NULL;
            } // End of (is_timeout == false)
        } // End of while loop
    }

    MessageBase* EdgeWrapper::processDataRequest_(MessageBase* request_ptr)
    {
        assert(request_ptr != NULL && request_ptr->isDataRequest());
        assert(local_edge_cache_ptr_ != NULL);

        bool is_valid_message_type = true;
        MessageBase* response_ptr = NULL;

        bool is_cached = false;
        if (request_ptr->isLocalRequest()) // Local request
        {
            // Get key from local request
            Key tmp_key = MessageBase::getKeyFromMessage(request_ptr);
            Value tmp_value();

            // Block until not invalidated
            blockForInvalidation_(tmp_key);

            if (request_ptr->message_type_ == MessageType::kLocalGetRequest)
            {
                is_cached = local_edge_cache_ptr_->get(tmp_key, tmp_value);

                // TODO: Get data from neighbor / cloud for cache miss
                // TODO: For COVERED, beacon node will tell the edge node whether to admit, w/o independent decision

                // Trigger independent cache admission for cache miss if necessary
                if (!is_cached && local_edge_cache_ptr_->needIndependentAdmit(tmp_key))
                {
                    triggerIndependentAdmission(tmp_key, tmp_value);
                }

                // Prepare LocalGetResponse for client
                response_ptr = new LocalGetResponse(tmp_key, tmp_value);
            }
            else // Local put/del request
            {
                // TODO: Acquire write lock from beacon node

                // TODO: Wait for beacon node to invalidate all other cache copies

                // TODO: Send request to cloud for write-through policy

                // Update/remove local edge cache
                if (request_ptr->message_type_ == MessageType::kLocalPutRequest)
                {
                    const LocalPutRequest* const local_put_request_ptr = static_cast<const LocalPutRequest*>(request_ptr);
                    Value tmp_value = local_put_request_ptr->getValue();
                    is_cached = local_edge_cache_ptr_->update(tmp_key, tmp_value);

                    // Prepare LocalPutResponse for client
                    response_ptr = new LocalPutResponse(tmp_key);
                }
                else if (request_ptr->message_type_ == MessageType::kLocalDelRequest)
                {
                    is_cached = local_edge_cache_ptr_->remove(tmp_key);

                    // Prepare LocalDelResponse for client
                    response_ptr = new LocalDelResponse(tmp_key);
                }
                else
                {
                    is_valid_message_type = false;
                }

                // Trigger independent cache admission for cache miss if necessary
                if (!is_cached && local_edge_cache_ptr_->needIndependentAdmit(tmp_key))
                {
                    triggerIndependentAdmission(tmp_key, tmp_value);
                }

                // TODO: Notify beacon node to validate all other cache copies
                // TODO: For COVERED, beacon node will tell the edge node if to admit, w/o independent decision
            }
        }
        else if (request_ptr->isRedirectedRequest()) // Redirected request
        {
            // TODO: redirected request for data message (the same as local requests???)
        }
        else
        {
            is_valid_message_type = false;
        }

        if (!is_valid_message_type)
        {
            std::ostringstream oss;
            oss << "invalid message type " << static_cast<uint32_t>(request_ptr->message_type_) << " for processDataRequest_()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        
        assert(response_ptr != NULL);
        return response_ptr;
    }

    MessageBase* EdgeWrapper::processControlRequest_(MessageBase* request_ptr)
    {
        assert(request_ptr != NULL && request_ptr->isControlRequest());
        assert(local_edge_cache_ptr_ != NULL);

        // TODO: invalidation and cache admission/eviction requests for control message
        return NULL;
    }

    void EdgeWrapper::blockForInvalidation_(const Key& key)
    {
        bool is_invalidated = false;
        while (true)
        {
            is_invalidated = local_edge_cache_ptr_->isInvalidated(key);
            if (is_invalidated)
            {
                // TODO: sleep a short time to avoid frequent polling
                continue;
            }
            else
            {
                break;
            }
        }
        assert(!is_invalidated);
        return;
    }

    void EdgeWrapper::triggerIndependentAdmission(const Key& key, const Value& value)
    {
        // NOTE: COVERED must NOT trigger any independent admission
        assert(Param::getCacheName != CacheWrapperBase::COVERED_CACHE_NAME);

        // Independently admit the new key-value pair into local edge cache
        local_edge_cache_ptr->admit(key, value);

        // Evict until cache size <= cache capacity
        uint32_t cache_size = 0;
        uint32_t cache_capacity = local_edge_cache_ptr->getCapacity();
        while (true)
        {
            current_cache_size = local_edge_cache_ptr->getSize();
            if (current_cache_size <= cache_capacity) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                local_edge_cache_ptr->evict();
                continue;
            }
        }

        return;
    }
}
#include "edge/edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "edge/beacon_server/beacon_server_base.h"
#include "edge/cache_server/cache_server_base.h"
#include "edge/invalidation_server/invalidation_server_base.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string EdgeWrapper::kClassName("EdgeWrapper");

    EdgeWrapper::EdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr, const uint32_t& capacity_bytes) : cache_name_(cache_name), edge_param_ptr_(edge_param_ptr), capacity_bytes_(capacity_bytes)
    {
        if (edge_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "edge_param_ptr is NULL!");
            exit(1);
        }

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = CacheWrapperBase::getEdgeCache(cache_name_, edge_param_ptr);
        assert(edge_cache_ptr_ != NULL);

        // Allocate cooperation wrapper for cooperative edge caching
        cooperation_wrapper_ptr_ = CooperationWrapperBase::getCooperationWrapper(cache_name, hash_name, edge_param_ptr);
        assert(cooperation_wrapper_ptr_ != NULL);
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // NOTE: no need to delete edge_param_ptr, as it is maintained outside EdgeWrapper

        // Release local edge cache
        assert(edge_cache_ptr_ != NULL);
        delete edge_cache_ptr_;
        edge_cache_ptr_ = NULL;

        // Release cooperative cache wrapper
        assert(cooperation_wrapper_ptr_ != NULL);
        delete cooperation_wrapper_ptr_;
        cooperation_wrapper_ptr_ = NULL;
    }

    void EdgeWrapper::start()
    {
        assert(edge_param_ptr_ != NULL);
        uint32_t edge_idx = edge_param_ptr_->getEdgeIdx();

        int pthread_returncode;
        pthread_t beacon_server_thread;
        pthread_t cache_server_thread;
        pthread_t invalidation_server_thread;

        // Launch beacon server
        pthread_returncode = pthread_create(&beacon_server_thread, NULL, launchBeaconServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch beacon server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch cache server
        pthread_returncode = pthread_create(&cache_server_thread, NULL, launchCacheServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch cache server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch invalidations server
        pthread_returncode = pthread_create(&invalidation_server_thread, NULL, launchInvalidationServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for beacon server
        pthread_returncode = pthread_join(beacon_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join beacon server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server
        pthread_returncode = pthread_join(cache_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for invalidation server
        pthread_returncode = pthread_join(invalidation_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void* EdgeWrapper::launchBeaconServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        BeaconServerBase* beacon_server_ptr = BeaconServerBase::getBeaconServer((EdgeWrapper*)edge_wrapper_ptr);
        assert(beacon_server_ptr != NULL);
        beacon_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchCacheServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        CacheServerBase* cache_server_ptr = CacheServerBase::getCacheServer((EdgeWrapper*)edge_wrapper_ptr);
        assert(cache_server_ptr != NULL);
        cache_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchInvalidationServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        InvalidationServerBase* invalidation_server_ptr = InvalidationServerBase::getInvalidationServer((EdgeWrapper*)edge_wrapper_ptr);
        assert(invalidation_server_ptr != NULL);
        invalidation_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }

    uint32_t EdgeWrapper::getSizeForCapacity_() const
    {
        uint32_t size = edge_cache_ptr_->getSizeForCapacity() + cooperation_wrapper_ptr_->getSizeForCapacity();
        return size;
    }

    // (1) Utility functions

    bool EdgeWrapper::currentIsBeacon_(const Key& key) const
    {
        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);

        bool current_is_beacon = false;
        uint32_t beacon_edge_idx = cooperation_wrapper_ptr_->getBeaconEdgeIdx(key);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        if (beacon_edge_idx == current_edge_idx)
        {
            current_is_beacon = true;
        }

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "beacon edge idx: " << beacon_edge_idx << "; current edge idx: " << current_edge_idx;
        //Util::dumpDebugMsg(instance_name_, oss.str());

        return current_is_beacon;
    }

    bool EdgeWrapper::currentIsTarget_(const DirectoryInfo& directory_info) const
    {
        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);

        bool current_is_target = false;
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        if (target_edge_idx == current_edge_idx)
        {
            current_is_target = true;
        }

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "target edge idx: " << target_edge_idx << "; current edge idx: " << current_edge_idx;
        //Util::dumpDebugMsg(instance_name_, oss.str());

        return current_is_target;
    }

    // (2) Invalidate for MSI protocol

    bool EdgeWrapper::invalidateCacheCopies_(const Key& key, const std::unordered_set<DirectoryInfo, DirectoryInfoHasher>& all_dirinfo) const
    {
        assert(edge_param_ptr_ != NULL);

        bool is_finish = false;

        uint32_t invalidate_edgecnt = all_dirinfo.size();
        if (invalidate_edgecnt == 0)
        {
            return is_finish;
        }
        assert(invalidate_edgecnt > 0);

        // Prepare a temporary socket client to invalidate cached copies for the given key
        // NOTE: use edge0 as default remote address, but will reset remote address based on dirinfo later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_invalidation_server_port = Util::getEdgeInvalidationServerRecvreqPort(0);
        NetworkAddr edge0_invalidation_server_addr(edge0_ipstr, edge0_invalidation_server_port);
        UdpSocketWrapper edge_sendreq_toinvalidate_socket_client(SocketRole::kSocketClient, edge0_invalidation_server_addr);

        // Convert directory informations into network addresses
        std::unordered_set<NetworkAddr, NetworkAddrHasher> all_networkaddr;
        for (std::unordered_set<DirectoryInfo, DirectoryInfoHasher>::const_iterator iter = all_dirinfo.begin(); iter != all_dirinfo.end(); iter++)
        {
            uint32_t tmp_edgeidx = iter->getTargetEdgeIdx();
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(tmp_edgeidx);
            uint16_t tmp_edge_invaliation_server_port = Util::getEdgeInvalidationServerRecvreqPort(tmp_edgeidx);
            NetworkAddr tmp_network_addr(tmp_edge_ipstr, tmp_edge_invaliation_server_port);
            all_networkaddr.insert(tmp_network_addr);
        }

        // Track whether invalidation requests to all involved edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> acked_flags;
        for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_ackflag = all_networkaddr.begin(); iter_for_ackflag != all_networkaddr.end(); iter_for_ackflag++)
        {
            acked_flags.insert(std::pair<NetworkAddr, bool>(*iter_for_ackflag, false));
        }

        // Issue all invalidation requests simultaneously
        while (acked_edgecnt != invalidate_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (invalidate_edgecnt - acked_edgecnt) control requests to the involved edge nodes that have not acknowledged invalidation requests
            for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_request = all_networkaddr.begin(); iter_for_request != all_networkaddr.end(); iter_for_request++)
            {
                if (acked_flags[*iter_for_request]) // Skip the edge node that has acknowledged the invalidation request
                {
                    continue;
                }

                const NetworkAddr& tmp_network_addr = *iter_for_request; // cache server address of a blocked closest edge node          
                sendInvalidationRequest_(&edge_sendreq_toinvalidate_socket_client, key, tmp_network_addr);     
            } // End of edgeidx_for_request

            // Receive (invalidate_edgecnt - acked_edgecnt) control repsonses from involved edge nodes
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < invalidate_edgecnt - acked_edgecnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                NetworkAddr control_response_addr;
                bool is_timeout = edge_sendreq_toinvalidate_socket_client.recv(control_response_msg_payload, control_response_addr);
                if (is_timeout)
                {
                    if (!edge_param_ptr_->isEdgeRunning())
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
                    assert(control_response_addr.isValidAddr());

                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kInvalidationResponse);

                    // Mark the edge node has acknowledged the InvalidationRequest
                    bool is_match = false;
                    for (std::unordered_set<NetworkAddr, NetworkAddrHasher>::const_iterator iter_for_response = all_networkaddr.begin(); iter_for_response != all_networkaddr.end(); iter_for_response++)
                    {
                        if (*iter_for_response == control_response_addr) // Match a closest edge node
                        {
                            assert(acked_flags[*iter_for_response] == false); // Original ack flag should be false

                            // Update ack information
                            acked_flags[*iter_for_response] = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive InvalidationResponse from an edge node " << control_response_addr.getIpstr() << ":" << control_response_addr.getPort() << ", which is NOT in the content directory list!";
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

        return is_finish;
    }

    void EdgeWrapper::sendInvalidationRequest_(UdpSocketWrapper* edge_sendreq_toinvalidate_socket_client_ptr, const Key& key, const NetworkAddr network_addr) const
    {
        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_toinvalidate_socket_client_ptr != NULL);

        // Prepare invalidation request to invalidate the cache copy
        uint32_t edge_idx = edge_param_ptr_->getEdgeIdx();
        InvalidationRequest invalidation_request(key, edge_idx);
        DynamicArray control_request_msg_payload(invalidation_request.getMsgPayloadSize());
        invalidation_request.serialize(control_request_msg_payload);

        // Set remote address to the closest edge node
        edge_sendreq_toinvalidate_socket_client_ptr->setRemoteAddrForClient(network_addr);

        // Send FinishBlockRequest to the closest edge node
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_sendreq_toinvalidate_socket_client_ptr->send(control_request_msg_payload);

        return;
    }
}
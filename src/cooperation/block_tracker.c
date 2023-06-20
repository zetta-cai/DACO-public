#include "cooperation/block_tracker.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/util.h"
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BlockTracker::kClassName("BlockTracker");

    BlockTracker::BlockTracker(EdgeParam* edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();

        edge_param_ptr_ = edge_param_ptr;
        assert(edge_param_ptr_ != NULL);

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_cooperation_metadata_ptr_";
        rwlock_for_cooperation_metadata_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);

        perkey_writeflags_.clear();
        perkey_edge_blocklist_.clear();

        // NOTE: we use edge0 as default remote address, but we will reset remote address of the socket clients based on the key later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_port = Util::getEdgeRecvreqPort(0);
        NetworkAddr edge0_addr(edge0_ipstr, edge0_port);
        edge_sendreq_toclosest_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_addr);
        assert(edge_sendreq_toclosest_socket_client_ptr_ != NULL);
    }

    BlockTracker::~BlockTracker()
    {
        // NOTE: no need to release edge_param_ptr_, which is maintained outside BlockTracker

        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);
        delete rwlock_for_cooperation_metadata_ptr_;
        rwlock_for_cooperation_metadata_ptr_ = NULL;

        assert(edge_sendreq_toclosest_socket_client_ptr_ != NULL);
        delete edge_sendreq_toclosest_socket_client_ptr_;
        edge_sendreq_toclosest_socket_client_ptr_ = NULL;
    }

    bool BlockTracker::isBeingWritten(const Key& key) const
    {
        // Acquire a read lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_cooperation_metadata_ptr_->try_lock_shared("isBeingWritten()"))
            {
                break;
            }
        }

        bool is_being_written = false;
        std::unordered_map<Key, bool>::const_iterator iter = perkey_writeflags_.find(key);
        if (iter != perkey_writeflags_.end())
        {
            is_being_written = iter->second;
        }

        rwlock_for_cooperation_metadata_ptr_->unlock_shared();
        return is_being_written;
    }

    void BlockTracker::block(const Key& key, const NetworkAddr& network_addr)
    {
        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_cooperation_metadata_ptr_->try_lock("addEdgeIntoBlocklist()"))
            {
                break;
            }
        }

        bool is_successful = false;

        perkey_edge_blocklist_t::iterator iter = perkey_edge_blocklist_.find(key);
        if (iter != perkey_edge_blocklist_.end()) // key does not exist
        {
            NetworkAddr default_network_addr;
            RingBuffer<NetworkAddr> tmp_edge_blocklist(default_network_addr);
            perkey_edge_blocklist_.insert(std::pair<Key, RingBuffer<NetworkAddr>>(key, tmp_edge_blocklist));
        }

        iter = perkey_edge_blocklist_.find(key);
        assert(iter != perkey_edge_blocklist_.end()); // key must exist now
        
        is_successful = iter->second.push(network_addr);
        assert(is_successful);

        rwlock_for_cooperation_metadata_ptr_->unlock();
        return;
    }

    bool BlockTracker::unblock(const Key& key)
    {
        bool is_finish = false;

        // Acquire a write lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_cooperation_metadata_ptr_->try_lock("addEdgeIntoBlocklist()"))
            {
                break;
            }
        }

        // Double-check if key is being written again atomically
        bool is_being_written = false;
        std::unordered_map<Key, bool>::const_iterator iter = perkey_writeflags_.find(key);
        if (iter != perkey_writeflags_.end())
        {
            is_being_written = iter->second;
        }

        if (!is_being_written) // key is still NOT being written
        {
            perkey_edge_blocklist_t::iterator iter = perkey_edge_blocklist_.find(key);
            RingBuffer<NetworkAddr>& tmp_edge_blocklist = iter->second;

            // Notify each blocked edge node to finish blocking for writes
            std::vector<NetworkAddr> blocked_edges;
            while (true)
            {
                // Get all closest edge nodes in block list
                NetworkAddr tmp_edge_addr;
                bool is_successful = tmp_edge_blocklist.pop(tmp_edge_addr);
                if (!is_successful) // No blocked edge node
                {
                    break;
                }
                else
                {
                    blocked_edges.push_back(tmp_edge_addr);
                }
            }

            // Notify all closest edge nodes simultaneously to finish blocking for writes
            is_finish = notifyEdgesToFinishBlock_(key, blocked_edges);

            // Remove the key and block list from perkey_edge_blocklist_
            perkey_edge_blocklist_.erase(iter);
        }

        rwlock_for_cooperation_metadata_ptr_->unlock();
        return is_finish;
    }

    uint32_t BlockTracker::getSizeForCapacity() const
    {
        // Acquire a read lock for cooperation metadata before accessing cooperation metadata
        assert(rwlock_for_cooperation_metadata_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_cooperation_metadata_ptr_->try_lock_shared("isBeingWritten()"))
            {
                break;
            }
        }

        // NOTE: we do NOT count key size in BlockTracker, as the size of keys managed by beacon edge node has been counted by DirectoryTable
        uint32_t size = 0;
        size += perkey_writeflags_.size() * sizeof(bool); // Size of write flags
        for (perkey_edge_blocklist_t::const_iterator iter = perkey_edge_blocklist_.begin(); iter != perkey_edge_blocklist_.end(); iter++)
        {
            size += iter->second.getSizeForCapacity(); // Size of per-key block list
        }

        rwlock_for_cooperation_metadata_ptr_->unlock_shared();
        return size;
    }

    bool BlockTracker::notifyEdgesToFinishBlock_(const Key& key, const std::vector<NetworkAddr>& closest_edges) const
    {
        // No need to acquire a lock, as unblock() has acquired a write lock

        assert(edge_sendreq_toclosest_socket_client_ptr_ != NULL);

        bool is_finish = false;

        uint32_t closest_edgecnt = closest_edges.size();
        if (closest_edgecnt == 0)
        {
            return is_finish;
        }
        assert(closest_edgecnt > 0);

        // Track whether notifictionas to all closest edge nodes have been acknowledged
        uint32_t acked_edgecnt = 0;
        std::vector<bool> acked_flags(closest_edgecnt, false);

        while (acked_edgecnt != closest_edgecnt) // Timeout-and-retry mechanism
        {
            // Send (closest_edgecnt - acked_edgecnt) control requests to the closest edge nodes that have not acknowledged notifications
            for (uint32_t edgeidx_for_request = 0; edgeidx_for_request < closest_edgecnt; edgeidx_for_request++)
            {
                if (acked_flags[edgeidx_for_request]) // Skip the closest edge node that has acknowledged the notification
                {
                    continue;
                }

                const NetworkAddr& tmp_network_addr = closest_edges[edgeidx_for_request];           
                sendFinishBlockRequest_(key, tmp_network_addr);     
            } // End of edgeidx_for_request

            // Receive (closest_edgecnt - acked_edgecnt) control repsonses from the beacon node
            for (uint32_t edgeidx_for_response = 0; edgeidx_for_response < closest_edgecnt - acked_edgecnt; edgeidx_for_response++)
            {
                DynamicArray control_response_msg_payload;
                NetworkAddr control_response_addr;
                bool is_timeout = edge_sendreq_toclosest_socket_client_ptr_->recv(control_response_msg_payload, control_response_addr);
                if (is_timeout)
                {
                    if (!edge_param_ptr_->isEdgeRunning())
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
                    assert(control_response_addr.isValid());

                    // Receive the control response message successfully
                    MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                    assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kFinishBlockResponse);

                    // Mark the closest edge node has acknowledged the FinishBlockRequest
                    bool is_match = false;
                    for (uint32_t edgeidx_for_ack = 0; edgeidx_for_ack < closest_edgecnt; edgeidx_for_ack++)
                    {
                        if (closest_edges[edgeidx_for_ack] == control_response_addr) // Match a closest edge node
                        {
                            assert(acked_flags[edgeidx_for_ack] == false); // Original ack flag should be false

                            // Update ack information
                            acked_flags[edgeidx_for_ack] = true;
                            acked_edgecnt += 1;
                            is_match = true;
                            break;
                        }
                    } // End of edgeidx_for_ack

                    if (!is_match) // Must match at least one closest edge node
                    {
                        std::ostringstream oss;
                        oss << "receive FinishBlockResponse from an edge node " << control_response_addr.getIpstr() << ":" << control_response_addr.getPort() << ", which is NOT in the block list!";
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
        } // End of while(acked_edgecnt != closest_edgecnt)

        return is_finish;
    }

    void BlockTracker::sendFinishBlockRequest_(const Key& key, const NetworkAddr& closest_edge_addr) const
    {
        // No need to acquire a lock, as unblock() has acquired a write lock

        // Prepare finish block request to finish blocking for writes in all closest edge nodes
        FinishBlockRequest finish_block_request(key);
        DynamicArray control_request_msg_payload(finish_block_request.getMsgPayloadSize());
        finish_block_request.serialize(control_request_msg_payload);

        // Set remote address to the closest edge node
        edge_sendreq_toclosest_socket_client_ptr_->setRemoteAddrForClient(closest_edge_addr);

        // Send FinishBlockRequest to the closest edge node
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_sendreq_toclosest_socket_client_ptr_->send(control_request_msg_payload);

        return;
    }
}
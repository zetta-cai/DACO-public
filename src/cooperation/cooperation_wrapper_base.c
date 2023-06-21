#include "cooperation/cooperation_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/config.h"
#include "common/param.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_wrapper.h"
#include "network/network_addr.h"

// TODO: remove if notifyEdgeToFinishBlock_ and blockForWritesByInterruption_ are pure virtual functions
#include "message/control_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CooperationWrapperBase::kClassName("CooperationWrapperBase");

    CooperationWrapperBase* CooperationWrapperBase::getCooperationWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr)
    {
        CooperationWrapperBase* cooperation_wrapper_ptr = NULL;

        if (cache_name == Param::COVERED_CACHE_NAME)
        {
            //cooperation_wrapper_ptr = new CoveredCooperationWrapper(hash_name, edge_param_ptr);
        }
        else
        {
            cooperation_wrapper_ptr = new BasicCooperationWrapper(hash_name, edge_param_ptr);
        }

        assert(cooperation_wrapper_ptr != NULL);
        return cooperation_wrapper_ptr;
    }

    CooperationWrapperBase::CooperationWrapperBase(const std::string& hash_name, EdgeParam* edge_param_ptr) : block_tracker_(edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        base_instance_name_ = oss.str();

        edge_param_ptr_ = edge_param_ptr;
        assert(edge_param_ptr_ != NULL);

        dht_wrapper_ptr_ = new DhtWrapper(hash_name, edge_param_ptr);
        assert(dht_wrapper_ptr_ != NULL);

        // NOTE: we use beacon server of edge0 as default remote address, but we will reset remote address of the socket clients based on the key later
        std::string edge0_ipstr = Config::getEdgeIpstr(0);
        uint16_t edge0_beacon_server_port = Util::getEdgeBeaconServerRecvreqPort(0);
        NetworkAddr edge0_beacon_server_addr(edge0_ipstr, edge0_beacon_server_port);
        uint16_t edge0_cache_server_port = Util::getEdgeCacheServerRecvreqPort(0);
        NetworkAddr edge0_cache_server_addr(edge0_ipstr, edge0_cache_server_port);

        edge_sendreq_tobeacon_socket_client_ptr_  = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_beacon_server_addr);
        assert(edge_sendreq_tobeacon_socket_client_ptr_  != NULL);

        edge_sendreq_totarget_socket_client_ptr_ = new UdpSocketWrapper(SocketRole::kSocketClient, edge0_cache_server_addr);
        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);

        // Allocate directory information
        uint32_t seed_for_directory_selection = edge_param_ptr_->getEdgeIdx();
        directory_table_ptr_ = new DirectoryTable(seed_for_directory_selection, edge_param_ptr_->getEdgeIdx());
        assert(directory_table_ptr_ != NULL);
    }

    CooperationWrapperBase::~CooperationWrapperBase()
    {
        // NOTE: no need to release edge_param_ptr_, which is maintained outside CooperationWrapperBase

        assert(dht_wrapper_ptr_ != NULL);
        delete dht_wrapper_ptr_;
        dht_wrapper_ptr_ = NULL;

        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);
        delete edge_sendreq_tobeacon_socket_client_ptr_ ;
        edge_sendreq_tobeacon_socket_client_ptr_ = NULL;

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);
        delete edge_sendreq_totarget_socket_client_ptr_;
        edge_sendreq_totarget_socket_client_ptr_ = NULL;

        // Release directory information
        assert(directory_table_ptr_ != NULL);
        delete directory_table_ptr_;
        directory_table_ptr_ = NULL;
    }

    bool CooperationWrapperBase::get(const Key& key, Value& value, bool& is_cooperative_cached_and_valid) const
    {
        // No need to acquire a read lock for cooperation metadata, which will be done in isBeingWritten_() in lookupLocalDirectory()

        bool is_finish = false;

        // Update remote address of edge_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = false;
        locateBeaconNode_(key, current_is_beacon);

        // Check if beacon node is the current edge node and lookup directory information
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        while (true)
        {
            if (!edge_param_ptr_->isEdgeRunning()) // edge node is NOT running
            {
                is_finish = true;
                return is_finish;
            }

            is_being_written = false;
            is_valid_directory_exist = false;
            if (current_is_beacon) // Get target edge index from local directory information
            {
                lookupLocalDirectory(key, is_being_written, is_valid_directory_exist, directory_info);
                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by polling
                    // TODO: sleep a short time to avoid frequent polling
                    continue; // Continue to lookup local directory info
                }
            }
            else // Get target edge index from remote directory information at the beacon node
            {
                is_finish = lookupBeaconDirectory_(key, is_being_written, is_valid_directory_exist, directory_info);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

                if (is_being_written) // If key is being written, we need to wait for writes
                {
                    // Wait for writes by interruption instead of polling to avoid duplicate DirectoryLookupRequest
                    is_finish = blockForWritesByInterruption_(key);
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

            if (is_valid_directory_exist) // The object is cached by some target edge node
            {
                // NOTE: the target node should not be the current edge node, as CooperationWrapperBase::get() can only be invoked if is_local_cached = false (i.e., the current edge node does not cache the object and hence is_valid_directory_exist should be false)
                assert(edge_param_ptr_ != NULL);
                uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
                if (directory_info.getTargetEdgeIdx() == current_edge_idx)
                {
                    std::ostringstream oss;
                    oss << "current edge node " << current_edge_idx << " should not be the target edge node for cooperative edge caching under a local cache miss!";
                    Util::dumpWarnMsg(base_instance_name_, oss.str());
                    return is_finish; // NOTE: is_finish is still false, as edge is STILL running
                }

                // Update remote address of edge_sendreq_totarget_socket_client_ptr_ as the target edge node
                locateTargetNode_(directory_info);

                // Get data from the target edge node if any and update is_cooperative_cached_and_valid
                bool is_cooperative_cached = false;
                bool is_valid = false;
                is_finish = redirectGetToTarget_(key, value, is_cooperative_cached, is_valid);
                if (is_finish) // Edge is NOT running
                {
                    return is_finish;
                }

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

    void CooperationWrapperBase::lookupLocalDirectory(const Key& key, bool& is_being_written, bool& is_valid_directory_exist, DirectoryInfo& directory_info) const
    {
        // No need to acquire a read lock for cooperation metadata due to accessing const shared variables and non-const yet thread-safe shared variables, yet will be done in isBeingWritten_()

        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        is_being_written = block_tracker_.isBeingWritten(key);
        if (!is_being_written) // if key is NOT being written
        {
            assert(directory_table_ptr_ != NULL);
            directory_table_ptr_->lookup(key, is_valid_directory_exist, directory_info);
        }
        else // key is being written
        {
            is_valid_directory_exist = false;
            directory_info = DirectoryInfo();
        }
        return;
    }

    bool CooperationWrapperBase::updateDirectory(const Key& key, const bool& is_admit, bool& is_being_written)
    {
        // No need to acquire a read lock for cooperation metadata, which will be done in isBeingWritten_() in updateLocalDirectory()

        bool is_finish = false;

        // Update remote address of edge_sendreq_tobeacon_socket_client_ptr_ as the beacon node for the key if remote
        bool current_is_beacon = false;
        locateBeaconNode_(key, current_is_beacon);

        // Check if beacon node is the current edge node and update directory information
        assert(edge_param_ptr_ != NULL);
        DirectoryInfo directory_info(edge_param_ptr_->getEdgeIdx());
        if (current_is_beacon) // Update target edge index of local directory information
        {
            updateLocalDirectory(key, is_admit, directory_info, is_being_written);
        }
        else // Update remote directory information at the beacon node
        {
            is_finish = updateBeaconDirectory_(key, is_admit, directory_info, is_being_written);
        }

        return is_finish;
    }

    void CooperationWrapperBase::updateLocalDirectory(const Key& key, const bool& is_admit, const DirectoryInfo& directory_info, bool& is_being_written)
    {
        // No need to acquire a read lock for cooperation metadata due to accessing const shared variables and non-const yet thread-safe shared variables, yet will be done in isBeingWritten_()

        // The current edge node must be the beacon node for the key
        verifyCurrentIsBeacon_(key);

        if (is_admit) // is_being_written affects validity of both directory info and cached object for cache admission
        {
            is_being_written = block_tracker_.isBeingWritten(key);
        }
        else // is_being_written does NOT affect cache eviction
        {
            is_being_written = false;
        }
        DirectoryMetadata directory_metadata(!is_being_written); // valid if not being written

        assert(directory_table_ptr_ != NULL);
        directory_table_ptr_->update(key, is_admit, directory_info, directory_metadata);
        return;
    }

    void CooperationWrapperBase::addEdgeIntoBlocklist(const Key& key, const NetworkAddr& network_addr)
    {
        verifyCurrentIsBeacon_(key); // Beacon node blocks closest edge nodes

        block_tracker_.block(key, network_addr);
        return;
    }

    void CooperationWrapperBase::tryToNotifyEdgesFromBlocklist(const Key& key)
    {
        verifyCurrentIsBeacon_(key); // Beacon node blocks closest edge nodes

        bool is_being_written = block_tracker_.isBeingWritten(key); // isBeingWritten_() acquires a read lock

        if (!is_being_written) // key is NOT being written
        {
            block_tracker_.unblock(key);
        }

        return;
    }

    uint32_t CooperationWrapperBase::getSizeForCapacity() const
    {
        assert(directory_table_ptr_ != NULL);
        uint32_t directory_table_size = directory_table_ptr_->getSizeForCapacity();
        uint32_t block_tracker_size = block_tracker_.getSizeForCapacity();
        return directory_table_size + block_tracker_size;
    }

    void CooperationWrapperBase::locateBeaconNode_(const Key& key, bool& current_is_beacon) const
    {
        // No need to acquire a write lock for cooperation metadata due to updating non-const individual variables

        assert(dht_wrapper_ptr_ != NULL);
        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);

        // Check if the current edge node is the beacon node for the key
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        if (beacon_edge_idx == current_edge_idx)
        {
            current_is_beacon = true;
        }
        else
        {
            current_is_beacon = false;

            // Set remote address such that current edge node can communicate with the beacon node for the key
            std::string beacon_edge_ipstr = dht_wrapper_ptr_->getBeaconEdgeIpstr(key);
            uint16_t beacon_edge_beacon_server_port = dht_wrapper_ptr_->getBeaconEdgeBeaconServerRecvreqPort(key);
            NetworkAddr beacon_edge_beacon_server_addr(beacon_edge_ipstr, beacon_edge_beacon_server_port);
            edge_sendreq_tobeacon_socket_client_ptr_->setRemoteAddrForClient(beacon_edge_beacon_server_addr);
        }

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "beacon edge idx: " << beacon_edge_idx << "; current edge idx: " << current_edge_idx;
        //Util::dumpDebugMsg(base_instance_name_, oss.str());

        return;
    }

    bool CooperationWrapperBase::blockForWritesByInterruption_(const Key& key) const
    {
        // No need to acquire a read lock due to accessing const shared variables and non-const individual variables

        verifyCurrentIsNotBeacon_(key); // Closest edge node must NOT be the beacon node if with interruption

        assert(edge_param_ptr_ != NULL);
        assert(edge_sendreq_tobeacon_socket_client_ptr_ != NULL);

        bool is_finish = false;

        // Wait for FinishBlockRequest from beacon node
        while (true)
        {
            // Receive the control repsonse from the beacon node
            DynamicArray control_response_msg_payload;
            bool is_timeout = edge_sendreq_tobeacon_socket_client_ptr_->recv(control_response_msg_payload);
            if (is_timeout)
            {
                if (!edge_param_ptr_->isEdgeRunning()) // Edge is not running
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
                // Receive the control response message successfully
                MessageBase* control_response_ptr = MessageBase::getResponseFromMsgPayload(control_response_msg_payload);
                assert(control_response_ptr != NULL && control_response_ptr->getMessageType() == MessageType::kFinishBlockRequest);

                // Get key from FinishBlockRequest
                const FinishBlockRequest* const finish_block_request_ptr = static_cast<const FinishBlockRequest*>(control_response_ptr);
                Key tmp_key = finish_block_request_ptr->getKey();

                // Release the control response message
                delete control_response_ptr;
                control_response_ptr = NULL;

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
            FinishBlockResponse finish_block_response(key);
            DynamicArray control_request_msg_payload(finish_block_response.getMsgPayloadSize());
            finish_block_response.serialize(control_request_msg_payload);

            // Send back FinishBlockResponse
            PropagationSimulator::propagateFromEdgeToNeighbor();
            edge_sendreq_tobeacon_socket_client_ptr_->send(control_request_msg_payload);
        }

        return is_finish;
    }

    void CooperationWrapperBase::locateTargetNode_(const DirectoryInfo& directory_info) const
    {
        // No need to acquire a write lock for cooperation metadata due to updating non-const individual variables

        // The current edge node must NOT be the target node
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(directory_info.getTargetEdgeIdx() != current_edge_idx);

        assert(edge_sendreq_totarget_socket_client_ptr_ != NULL);

        // Set remote address such that the current edge node can communicate with the target edge node
        uint32_t target_edge_idx = directory_info.getTargetEdgeIdx();
        std::string target_edge_ipstr = Config::getEdgeIpstr(target_edge_idx);
        uint16_t target_edge_cache_server_port = Util::getEdgeCacheServerRecvreqPort(target_edge_idx);
        NetworkAddr target_edge_cache_server_addr(target_edge_ipstr, target_edge_cache_server_port);
        edge_sendreq_totarget_socket_client_ptr_->setRemoteAddrForClient(target_edge_cache_server_addr);

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "target edge idx: " << target_edge_idx << "; current edge idx: " << current_edge_idx;
        //Util::dumpDebugMsg(base_instance_name_, oss.str());
        
        return;
    }

    void CooperationWrapperBase::verifyCurrentIsBeacon_(const Key& key) const
    {
        // No need to acquire a write lock for cooperation metadata due to updating const shared variables

        // The current edge node must be the beacon node for the key
        assert(dht_wrapper_ptr_ != NULL);
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(beacon_edge_idx == current_edge_idx);
        return;
    }

    void CooperationWrapperBase::verifyCurrentIsNotBeacon_(const Key& key) const
    {
        // No need to acquire a write lock for cooperation metadata due to updating const shared variables
        
        // The current edge node must be the beacon node for the key
        assert(dht_wrapper_ptr_ != NULL);
        uint32_t beacon_edge_idx = dht_wrapper_ptr_->getBeaconEdgeIdx(key);
        assert(edge_param_ptr_ != NULL);
        uint32_t current_edge_idx = edge_param_ptr_->getEdgeIdx();
        assert(beacon_edge_idx != current_edge_idx);
        return;
    }
}
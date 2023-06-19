#include "edge/basic_edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/key.h"
#include "common/param.h"
#include "common/util.h"
#include "message/data_message.h"
#include "message/control_message.h"
#include "message/message_base.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicEdgeWrapper::kClassName("BasicEdgeWrapper");

    BasicEdgeWrapper::BasicEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr) : EdgeWrapperBase(cache_name, hash_name, edge_param_ptr)
    {
        assert(cache_name != Param::COVERED_CACHE_NAME);

        // Differentiate BasicEdgeWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
    }

    BasicEdgeWrapper::~BasicEdgeWrapper() {}

    // (1) Data requests

    bool BasicEdgeWrapper::processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const
    {
        // Get key and value from redirected request if any
        assert(redirected_request_ptr != NULL && redirected_request_ptr->getMessageType() == MessageType::kRedirectedGetRequest);
        const RedirectedGetRequest* const redirected_get_request_ptr = static_cast<const RedirectedGetRequest*>(redirected_request_ptr);
        Key tmp_key = redirected_get_request_ptr->getKey();
        Value tmp_value;

        // Acquire a read lock for serializability before accessing any shared variable in the target edge node
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock_shared(tmp_key, "processRedirectedGetRequest_()"))
            {
                break;
            }
        }

        assert(edge_cache_ptr_ != NULL);

        bool is_finish = false;
        Hitflag hitflag = Hitflag::kGlobalMiss;

        // Access local edge cache for cooperative edge caching (current edge node is the target edge node)
        bool is_cooperative_cached_and_valid = edge_cache_ptr_->get(tmp_key, tmp_value);
        bool is_cooperaitve_cached = edge_cache_ptr_->isLocalCached(tmp_key);
        if (is_cooperative_cached_and_valid) // cached and valid
        {
            hitflag = Hitflag::kCooperativeHit;
        }
        else // not cached or invalid
        {
            if (is_cooperaitve_cached) // cached and invalid
            {
                hitflag = Hitflag::kCooperativeInvalid;
            }
        }

        // NOTE: no need to perform recursive cooperative edge caching (current edge node is already the target edge node for cooperative edge caching)
        // NOTE: no need to access cloud to get data, which will be performed by the closest edge node

        // TODO: reply redirected response message to an edge node

        // Prepare RedirectedGetResponse for the closest edge node
        RedirectedGetResponse redirected_get_response(tmp_key, tmp_value, hitflag);

        // Reply redirected response message to the closest edge node (the remote address set by the most recent recv)
        DynamicArray redirected_response_msg_payload(redirected_get_response.getMsgPayloadSize());
        redirected_get_response.serialize(redirected_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_recvreq_socket_server_ptr_->send(redirected_response_msg_payload);

        perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
        return is_finish;
    }

    void BasicEdgeWrapper::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processRedirectedGetRequest_() and processRedirectedWriteRequest_()

        assert(edge_cache_ptr_ != NULL);
        assert(cooperation_wrapper_ptr_ != NULL);

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "admit key: " << key.getKeystr() << "; value: " << (value.isDeleted()?"true":"false") << "; //valuesize: " << value.getValuesize();
        //Util::dumpDebugMsg(instance_name_, oss.str());

        // Independently admit the new key-value pair into local edge cache
        bool is_being_written = false;
        cooperation_wrapper_ptr_->updateDirectory(key, true, is_being_written);
        edge_cache_ptr_->admit(key, value, !is_being_written); // valid if not being written

        // Evict until cache size <= cache capacity (TODO: update after introducing entire capacity)
        uint32_t cache_capacity = edge_cache_ptr_->getCapacityBytes();
        while (true)
        {
            uint32_t cache_size = edge_cache_ptr_->getSizeForCapacity();
            if (cache_size <= cache_capacity) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                Key victim_key;
                Value victim_value;
                edge_cache_ptr_->evict(victim_key, victim_value);
                bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
                cooperation_wrapper_ptr_->updateDirectory(victim_key, false, _unused_is_being_written);

                // TMPDEBUG
                //oss.clear();
                //oss.str("");
                //oss << "evict key: " << victim_key.getKeystr() << "; value: " << (victim_value.isDeleted()?"true":"false") << "; valuesize: " << victim_value.getValuesize();
                //Util::dumpDebugMsg(instance_name_, oss.str());

                continue;
            }
        }

        return;
    }

    // (2) Control requests

    bool BasicEdgeWrapper::processDirectoryLookupRequest_(MessageBase* control_request_ptr, const NetworkAddr& closest_edge_addr) const
    {
        // Get key and value from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryLookupRequest);
        const DirectoryLookupRequest* const directory_lookup_request_ptr = static_cast<const DirectoryLookupRequest*>(control_request_ptr);
        Key tmp_key = directory_lookup_request_ptr->getKey();

        assert(closest_edge_addr.isValid());

        // Acquire a read lock for serializability before accessing any shared variable in the beacon edge node
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock_shared(tmp_key, "processDirectoryLookupRequest_()"))
            {
                break;
            }
        }

        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;

        // Lookup local directory information and randomly select a target edge index
        bool is_being_written = false;
        bool is_valid_directory_exist = false;
        DirectoryInfo directory_info;
        cooperation_wrapper_ptr_->lookupLocalDirectory(tmp_key, is_being_written, is_valid_directory_exist, directory_info);

        // Send back a directory lookup response
        DirectoryLookupResponse directory_lookup_response(tmp_key, is_being_written, is_valid_directory_exist, directory_info);
        DynamicArray control_response_msg_payload(directory_lookup_response.getMsgPayloadSize());
        directory_lookup_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        if (is_being_written)
        {
            // Add the closest edge node into the CooperationWrapperBase::perkey_edge_blocklist_
            cooperation_wrapper_ptr_->addEdgeIntoBlocklist(tmp_key, closest_edge_addr);

            // Try to notify blocked edge nodes if without writes, in case that writes have finished before adding the closest edge node
            cooperation_wrapper_ptr_->tryToNotifyEdgesFromBlocklist(tmp_key);
        }

        perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
        return is_finish;
    }

    bool BasicEdgeWrapper::processDirectoryUpdateRequest_(MessageBase* control_request_ptr)
    {
        // Get key and value from control request if any
        assert(control_request_ptr != NULL);
        assert(control_request_ptr->getMessageType() == MessageType::kDirectoryUpdateRequest);
        const DirectoryUpdateRequest* const directory_update_request_ptr = static_cast<const DirectoryUpdateRequest*>(control_request_ptr);
        Key tmp_key = directory_update_request_ptr->getKey();
        bool is_admit = directory_update_request_ptr->isValidDirectoryExist();
        DirectoryInfo directory_info = directory_update_request_ptr->getDirectoryInfo();

        // Acquire a write lock for serializability before accessing any shared variable in the beacon edge node
        assert(perkey_rwlock_for_serializability_ptr_ != NULL);
        while (true)
        {
            if (perkey_rwlock_for_serializability_ptr_->try_lock(tmp_key, "processDirectoryUpdateRequest_()"))
            {
                break;
            }
        }

        assert(cooperation_wrapper_ptr_ != NULL);
        assert(edge_recvreq_socket_server_ptr_ != NULL);

        bool is_finish = false;        

        // Update local directory information
        bool is_being_written = false;
        cooperation_wrapper_ptr_->updateLocalDirectory(tmp_key, is_admit, directory_info, is_being_written);

        // Send back a directory update response
        DirectoryUpdateResponse directory_update_response(tmp_key, is_being_written);
        DynamicArray control_response_msg_payload(directory_update_response.getMsgPayloadSize());
        directory_update_response.serialize(control_response_msg_payload);
        PropagationSimulator::propagateFromNeighborToEdge();
        edge_recvreq_socket_server_ptr_->send(control_response_msg_payload);

        perkey_rwlock_for_serializability_ptr_->unlock(tmp_key);
        return is_finish;
    }
    
    bool BasicEdgeWrapper::processOtherControlRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL);

        // TODO: acquire a read/write lock before accessing any shared variable in the beacon edge node

        bool is_finish = false;

        std::ostringstream oss;
        oss << "control request " << MessageBase::messageTypeToString(control_request_ptr->getMessageType()) << " is not supported!";
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return is_finish;
    }
}
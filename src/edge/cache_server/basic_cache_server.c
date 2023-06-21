#include "edge/cache_server/basic_cache_server.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "message/data_message.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCacheServer::kClassName("BasicCacheServer");

    BasicCacheServer::BasicCacheServer(EdgeWrapper* edge_wrapper_ptr) : CacheServerBase(edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cache_name_ != Param::COVERED_CACHE_NAME);
        uint32_t edge_idx = edge_wrapper_ptr_->edge_param_ptr->getEdgeIdx();

        // Differentiate BasicCacheServer in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicCacheServer::~BasicCacheServer() {}

    // (1) Data requests

    bool BasicCacheServer::processRedirectedGetRequest_(MessageBase* redirected_request_ptr) const
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

        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_cache_ptr_ != NULL);

        bool is_finish = false;
        Hitflag hitflag = Hitflag::kGlobalMiss;

        // Access local edge cache for cooperative edge caching (current edge node is the target edge node)
        bool is_cooperative_cached_and_valid = edge_wrapper_ptr_->edge_cache_ptr_->get(tmp_key, tmp_value);
        bool is_cooperaitve_cached = edge_wrapper_ptr_->edge_cache_ptr_->isLocalCached(tmp_key);
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
        edge_cache_server_recvreq_socket_server_ptr_->send(redirected_response_msg_payload);

        perkey_rwlock_for_serializability_ptr_->unlock_shared(tmp_key);
        return is_finish;
    }

    void BasicCacheServer::triggerIndependentAdmission_(const Key& key, const Value& value) const
    {
        // No need to acquire per-key rwlock for serializability, which has been done in processLocalGetRequest_() and processLocalWriteRequest_()

        assert(edge_wrapper_ptr_ != NULL);
        assert(edge_wrapper_ptr_->edge_cache_ptr_ != NULL);
        assert(edge_wrapper_ptr_->cooperation_wrapper_ptr_ != NULL);

        // TMPDEBUG
        //std::ostringstream oss;
        //oss << "admit key: " << key.getKeystr() << "; value: " << (value.isDeleted()?"true":"false") << "; //valuesize: " << value.getValuesize();
        //Util::dumpDebugMsg(instance_name_, oss.str());

        // Independently admit the new key-value pair into local edge cache
        bool is_being_written = false;
        edge_wrapper_ptr_->cooperation_wrapper_ptr_->updateDirectory(key, true, is_being_written);
        edge_wrapper_ptr_->edge_cache_ptr_->admit(key, value, !is_being_written); // valid if not being written

        // Evict until used bytes <= capacity bytes
        while (true)
        {
            // Data and metadata for local edge cache, and cooperation metadata
            uint32_t used_bytes = edge_wrapper_ptr_->edge_cache_ptr_->getSizeForCapacity() + edge_wrapper_ptr_->cooperation_wrapper_ptr_->getSizeForCapacity();
            if (used_bytes <= edge_wrapper_ptr_->capacity_bytes_) // Not exceed capacity limitation
            {
                break;
            }
            else // Exceed capacity limitation
            {
                Key victim_key;
                Value victim_value;
                edge_wrapper_ptr_->edge_cache_ptr_->evict(victim_key, victim_value);
                bool _unused_is_being_written = false; // NOTE: is_being_written does NOT affect cache eviction
                edge_wrapper_ptr_->cooperation_wrapper_ptr_->updateDirectory(victim_key, false, _unused_is_being_written);

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
}
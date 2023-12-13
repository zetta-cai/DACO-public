/*
 * SynchronizationServerParam: parameters to launch synchronization server in an edge node (thread safe).
 * 
 * By Siyuan Sheng (2023.12.13).
 */

#ifndef SYNCHRONIZATION_SERVER_PARAM_H
#define SYNCHRONIZATION_SERVER_PARAM_H

#include <string>

namespace covered
{
    class SynchronizationServerParam;
}

#include "concurrency/ring_buffer_impl.h"
#include "edge/edge_wrapper.h"
#include "edge/synchronization_server/covered_synchronization_server.h"
#include "edge/synchronization_server/synchronization_server_item.h"

namespace covered
{   
    class SynchronizationServerParam
    {
    public:
        SynchronizationServerParam();
        SynchronizationServerParam(EdgeWrapper* edge_wrapper_ptr, const uint32_t& data_request_buffer_size);
        ~SynchronizationServerParam();

        const SynchronizationServerParam& operator=(const SynchronizationServerParam& other);

        EdgeWrapper* getEdgeWrapperPtr() const;
        RingBuffer<SynchronizationServerItem>* getSynchronizationServerItemBufferPtr() const;
    private:
        static const std::string kClassName;

        EdgeWrapper* edge_wrapper_ptr_; // thread safe
        RingBuffer<SynchronizationServerItem>* synchronization_server_item_buffer_ptr_; // thread safe
    };
}

#endif
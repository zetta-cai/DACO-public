/*
 * InvalidationServerBase: listen to receive control requests issued by closest/beacon edge nodes; access local edge cache to invalidate cached objects for MSI protocol.
 * 
 * Involved messages triggered by local put/del requests
 * (1) Receive/issue invalidation requests/responses
 * 
 * By Siyuan Sheng (2023.06.24).
 */

#ifndef INVALIDATION_SERVER_BASE_H
#define INVALIDATION_SERVER_BASE_H

#include <string>

#include "edge/edge_wrapper.h"
#include "message/message_base.h"
#include "network/udp_socket_wrapper.h"

namespace covered
{
    class InvalidationServerBase
    {
    public:
        static InvalidationServerBase* getInvalidationServerByCacheName(EdgeWrapper* edge_wrapper_ptr);

        InvalidationServerBase(EdgeWrapper* edge_wrapper_ptr);
        virtual ~InvalidationServerBase();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        virtual bool processInvalidationRequest_(MessageBase* control_request_ptr) = 0;

        // Member varaibles

        // Const variable
        std::string base_instance_name_;
    protected:

        // Member variables

        // Const variable
        const EdgeWrapper* edge_wrapper_ptr_;

        // Non-const individual variable
        UdpSocketWrapper* edge_invalidation_server_recvreq_socket_server_ptr_;
    };
}

#endif
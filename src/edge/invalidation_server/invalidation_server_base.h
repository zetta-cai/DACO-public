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
        bool processInvalidationRequest_(MessageBase* control_request_ptr, const NetworkAddr& recvrsp_dst_addr);
        virtual void processReqForInvalidation_(MessageBase* control_request_ptr) = 0;
        virtual MessageBase* getRspForInvalidation_(MessageBase* control_request_ptr, const BandwidthUsage& total_bandwidth_usage, const EventList& event_list) = 0;

        // Member varaibles

        // Const variable
        std::string base_instance_name_;
    protected:
        void checkPointers_() const;

        // Member variables

        // Const variable
        const EdgeWrapper* edge_wrapper_ptr_;

        // For receiving invalidation requests
        NetworkAddr edge_invalidation_server_recvreq_source_addr_; // The same as cache server worker or beacon server to send invalidation requests (const individual variable)
        UdpMsgSocketServer* edge_invalidation_server_recvreq_socket_server_ptr_; // Used by invalidation server to receive invalidtion requests from cache server workers or beacon servers (non-const individual variable)
    };
}

#endif
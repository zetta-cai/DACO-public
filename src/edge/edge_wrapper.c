#include "edge_wrapper.h"

#include <assert.h>

#include "common/util.h"
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
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // NOTE: no need to delete local_edge_param_ptr, as it is maintained outside EdgeWrapper
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
        DynamicArray req_msg_payload;
        bool is_timeout = false;
        while (local_edge_param_ptr_->isEdgeRunning()) // local_edge_running_ is set as true by default
        {
            is_timeout = edge_recvreq_socket_server.recv(req_msg_payload);
            if (is_timeout == true)
            {
                if (!local_edge_param_ptr_->isEdgeRunning()) // Check local_edge_running_ to break
                {
                    break;
                }
                else
                {
                    // TODO: timeout-and-retry
                }
            } // End of (is_timeout == true)
            else
            {
                // TODO: Process received requests
            } // End of (is_timeout == false)
        } // End of while loop
    }
}
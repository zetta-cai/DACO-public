#include "benchmark/evaluator_wrapper.h"

#include <assert.h>
#include <sstream>
#include <unordered_map>

#include "common/config.h"
#include "common/dynamic_array.h"
#include "common/param.h"
#include "common/util.h"
#include "message/message_base.h"
#include "message/control_message.h"

namespace covered
{
    const std::string EvaluatorWrapper::kClassName("EvaluatorWrapper");

    void* EvaluatorWrapper::launchEvaluatorWrapper(void* is_evaluator_initialized_ptr)
    {
        bool& is_evaluator_initialized = *(static_cast<bool*>(is_evaluator_initialized_ptr));

        EvaluatorWrapper evaluator(Param::getClientcnt(), Param::getEdgecnt());
        is_evaluator_initialized = true; // Such that simulator or prototype will continue to launch cloud, edge, and client nodes

        evaluator.start();
        
        pthread_exit(NULL);
        return NULL;
    }

    EvaluatorWrapper::EvaluatorWrapper(const uint32_t& clientcnt, const uint32_t& edgecnt) : clientcnt_(clientcnt), edgecnt_(edgecnt)
    {
        is_warmup_phase_ = true;

        // (1) Manage evaluation phases

        // Prepare evaluator recvmsg source addr
        std::string evaluator_ipstr = Config::getEvaluatorIpstr();
        uint16_t evaluator_recvmsg_port = Config::getEvaluatorRecvmsgPort();
        evaluator_recvmsg_source_addr_ = NetworkAddr(evaluator_ipstr, evaluator_recvmsg_port);

        // Prepare client recvmsg dst addrs
        perclient_recvmsg_dst_addrs_ = new NetworkAddr[clientcnt];
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        for (uint32_t client_idx = 0; client_idx < clientcnt; client_idx++)
        {
            std::string tmp_client_ipstr = Config::getClientIpstr(client_idx, clientcnt);
            uint16_t tmp_client_recvmsg_port = Util::getClientRecvmsgPort(client_idx, clientcnt);
            perclient_recvmsg_dst_addrs_[client_idx] = NetworkAddr(tmp_client_ipstr, tmp_client_recvmsg_port);
        }

        // Prepare edge recvmsg dst addrs
        peredge_recvmsg_dst_addrs_ = new NetworkAddr[edgecnt];
        assert(peredge_recvmsg_dst_addrs_ != NULL);
        for (uint32_t edge_idx = 0; edge_idx < edgecnt; edge_idx++)
        {
            std::string tmp_edge_ipstr = Config::getEdgeIpstr(edge_idx, edgecnt);
            uint16_t tmp_edge_recvmsg_port = Util::getEdgeRecvmsgPort(edge_idx, edgecnt);
            peredge_recvmsg_dst_addrs_[edge_idx] = NetworkAddr(tmp_edge_ipstr, tmp_edge_recvmsg_port);
        }

        // Prepare cloud recvmsg dst addr
        std::string cloud_ipstr = Config::getCloudIpstr();
        uint16_t cloud_recvmsg_port = Util::getCloudRecvmsgPort(0);
        cloud_recvmsg_dst_addr_ = NetworkAddr(cloud_ipstr, cloud_recvmsg_port);

        // Prepare a socket server to receive benchmark control messages
        NetworkAddr host_addr(Util::ANY_IPSTR, evaluator_recvmsg_port);
        evaluator_recvmsg_socket_server_ptr_ = new UdpMsgSocketServer(host_addr);
        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);

        // Prepare a socket client to send benchmark control messages
        evaluator_sendmsg_socket_client_ptr_ = new UdpMsgSocketClient();
        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);

        // (2) Track total aggregated statistics

        // TODO
        //total_statistics_tracker_ptr_ = new TotalStatisticsTracker();
        //assert(total_statistics_tracker_ptr_ != NULL);
    }

    EvaluatorWrapper::~EvaluatorWrapper()
    {
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        delete[] perclient_recvmsg_dst_addrs_;
        perclient_recvmsg_dst_addrs_ = NULL;

        assert(peredge_recvmsg_dst_addrs_ != NULL);
        delete[] peredge_recvmsg_dst_addrs_;
        peredge_recvmsg_dst_addrs_ = NULL;

        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);
        delete evaluator_recvmsg_socket_server_ptr_;
        evaluator_recvmsg_socket_server_ptr_ = NULL;

        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);
        delete evaluator_sendmsg_socket_client_ptr_;
        evaluator_sendmsg_socket_client_ptr_ = NULL;

        assert(total_statistics_tracker_ptr_ != NULL);
        delete total_statistics_tracker_ptr_;
        total_statistics_tracker_ptr_ = NULL;
    }

    void EvaluatorWrapper::start()
    {
        // Wait all componenets finish initiailization
        blockForInitialization_();

        // TODO
    }

    void EvaluatorWrapper::blockForInitialization_()
    {
        checkPointers_();

        // Client/edge/cloud ack flags
        std::unordered_map<NetworkAddr, bool, NetworkAddrHasher> initialization_acked_flags;
        for (uint32_t client_idx = 0; client_idx < clientcnt_; client_idx++)
        {
            initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(perclient_recvmsg_dst_addrs_[client_idx], false));
        }
        for (uint32_t edge_idx = 0; edge_idx < edgecnt_; edge_idx++)
        {
            initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(peredge_recvmsg_dst_addrs_[edge_idx], false));
        }
        initialization_acked_flags.insert(std::pair<NetworkAddr, bool>(cloud_recvmsg_dst_addr_, false));

        Util::dumpNormalMsg(kClassName, "Wait for intialization of all clients, edges, and clouds...");

        uint32_t acked_cnt = 0;
        while (acked_cnt < initialization_acked_flags.size())
        {
            DynamicArray control_request_msg_payload;
            bool is_timeout = evaluator_recvmsg_socket_server_ptr_->recv(control_request_msg_payload);
            if (is_timeout)
            {
                continue; // Wait until all client/edge/cloud finish initialization
            }
            else
            {
                MessageBase* control_request_ptr = MessageBase::getRequestFromMsgPayload(control_request_msg_payload);
                assert(control_request_ptr != NULL);
                assert(control_request_ptr->getMessageType() == MessageType::kInitializationRequest);

                NetworkAddr tmp_dst_addr = control_request_ptr->getSourceAddr();
                std::unordered_map<NetworkAddr, bool, NetworkAddrHasher>::iterator iter = initialization_acked_flags.find(tmp_dst_addr);
                if (iter == initialization_acked_flags.end())
                {
                    std::ostringstream oss;
                    oss << "receive InitializationRequest from network addr " << tmp_dst_addr.toString() << ", which is NOT in the to-be-acked list!";
                    Util::dumpWarnMsg(kClassName, oss.str());
                }
                else
                {
                    if (iter->second == false)
                    {
                        // Send back InitializationResponse
                        InitializationResponse initialization_response(0, evaluator_recvmsg_source_addr_, EventList());
                        evaluator_sendmsg_socket_client_ptr_->send((MessageBase*)&initialization_response, tmp_dst_addr);

                        iter->second = true;
                        acked_cnt++;
                    }
                }

                delete control_request_ptr;
                control_request_ptr = NULL;
            }
        }

        Util::dumpNormalMsg(kClassName, "All clients, edges, and clouds finish initialization!");

        return;
    }

    void EvaluatorWrapper::checkPointers_() const
    {
        assert(perclient_recvmsg_dst_addrs_ != NULL);
        assert(peredge_recvmsg_dst_addrs_ != NULL);
        assert(evaluator_recvmsg_socket_server_ptr_ != NULL);
        assert(evaluator_sendmsg_socket_client_ptr_ != NULL);
        assert(total_statistics_tracker_ptr_ != NULL);
        return;
    }
}
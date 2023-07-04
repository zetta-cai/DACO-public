#include "network/propagation_simulator.h"

#include <sstream>
#include <unistd.h> // usleep

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    std::string PropagationSimulator::kClassName("PropagationSimulator");

    void* PropagationSimulator::launchPropagationSimulator(void* propagation_simulator_param_ptr)
    {
        assert(propagation_simulator_param_ptr != NULL);

        PropagationSimulator propagation_simulator((PropagationSimulatorParam*)propagation_simulator_param_ptr);
        propagation_simulator.start();

        pthread_exit(NULL);
        return NULL;
    }

    PropagationSimulator::PropagationSimulator(PropagationSimulatorParam* propagation_simulator_param_ptr)
    {
        assert(propagation_simulator_param_ptr != NULL);

        NodeParamBase* node_param_ptr = propagation_simulator_param_ptr->getNodeParamPtr();
        assert(node_param_ptr != NULL);
        uint32_t node_idx = node_param_ptr->getNodeIdx();
        std::string node_role = node_param_ptr->getNodeRole();

        // Differential propagation simulator of different nodes
        std::ostringstream oss;
        oss << kClassName << " " << node_role << node_idx;
        instance_name_ = oss.str();

        // Allocate socket client to issue message
        propagation_simulator_socket_client_ptr_ = new UdpMsgSocketClient();
        assert(propagation_simulator_socket_client_ptr_ != NULL);

        propagation_simulator_param_ptr_ = propagation_simulator_param_ptr;
    }

    PropagationSimulator::~PropagationSimulator()
    {
        // NOTE: no need to release propagation_simulator_param_ptr_, which will be released outside PropagationSimulator (e.g., in Client/Edge/CloudWrapper)

        // Release socket client
        assert(propagation_simulator_socket_client_ptr_ != NULL);
        delete propagation_simulator_socket_client_ptr_;
        propagation_simulator_socket_client_ptr_ = NULL;
    }

    void PropagationSimulator::start()
    {
        checkPointers_();

        // Loop until node finishes
        while (propagation_simulator_param_ptr_->getNodeParamPtr()->isNodeRunning())
        {
            PropagationItem tmp_propagation_item;
            bool is_successful = propagation_simulator_param_ptr_->pop(tmp_propagation_item);

            if (!is_successful)
            {
                continue; // Continue to check if node is finished
            }
            else
            {
                // Sleep to simulate propagation latency
                uint32_t sleep_us = tmp_propagation_item.getSleepUs();
                #ifdef DEBUG_PROPAGATION_SIMULATOR
                std::ostringstream oss;
                oss << "sleep " << sleep_us << " us to simulate a propagation latency of " << propagation_simulator_param_ptr_->getPropagationLatencyUs() << " us";
                Util::dumpDebugMsg(instance_name_, oss.str());
                #endif
                propagate_(sleep_us);

                // Prepare message payload
                MessageBase* message_ptr = tmp_propagation_item.getMessagePtr();
                assert(message_ptr != NULL);
                DynamicArray message_payload(message_ptr->getMsgPayloadSize());
                message_ptr->serialize(message_payload);

                // Issue the message to the given address
                NetworkAddr dst_addr = tmp_propagation_item.getNetworkAddr();
                assert(dst_addr.isValidAddr());
                propagation_simulator_socket_client_ptr_->send(message_payload, dst_addr);

                // Release the message
                delete message_ptr;
                message_ptr = NULL;
            }
        }

        return;
    }

    void PropagationSimulator::checkPointers_() const
    {
        assert(propagation_simulator_param_ptr_ != NULL);
        assert(propagation_simulator_param_ptr_->getNodeParamPtr() != NULL);

        return;
    }
        
    void PropagationSimulator::propagate_(const uint32_t& sleep_us)
    {
        usleep(sleep_us);
        return;
    }
}
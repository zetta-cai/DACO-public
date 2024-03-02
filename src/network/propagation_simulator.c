#include "network/propagation_simulator.h"

#include <sstream>
#include <unistd.h> // usleep

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

    PropagationSimulator::PropagationSimulator(PropagationSimulatorParam* propagation_simulator_param_ptr) : propagation_simulator_param_ptr_(propagation_simulator_param_ptr)
    {
        assert(propagation_simulator_param_ptr != NULL);

        // Differential propagation simulator of different nodes
        std::ostringstream oss;
        oss << kClassName << " " << propagation_simulator_param_ptr->getNodeWrapperPtr()->getNodeRoleIdxStr();
        instance_name_ = oss.str();

        // Allocate socket client to issue message
        propagation_simulator_socket_client_ptr_ = new UdpMsgSocketClient();
        assert(propagation_simulator_socket_client_ptr_ != NULL);
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

        // Notify node that the current propagation simulator has finished initialization
        propagation_simulator_param_ptr_->markFinishInitialization();

        // NOTE: block for clients which is NOT running at first
        while (!propagation_simulator_param_ptr_->getNodeWrapperPtr()->isNodeRunning()) {}

        // Loop until node finishes
        while (propagation_simulator_param_ptr_->getNodeWrapperPtr()->isNodeRunning())
        {
            PropagationItem tmp_propagation_item;
            bool is_successful = propagation_simulator_param_ptr_->pop(tmp_propagation_item);

            if (!is_successful)
            {        
                continue; // Continue to check if node is finished
            }
            else
            {
                // Prepare message payload
                MessageBase* message_ptr = tmp_propagation_item.getMessagePtr();
                assert(message_ptr != NULL);

                // Sleep to simulate propagation latency
                uint32_t sleep_us = tmp_propagation_item.getSleepUs();
                #ifdef DEBUG_PROPAGATION_SIMULATOR
                std::ostringstream oss;
                oss << "before sleep " << sleep_us << " us to simulate a propagation latency of " << propagation_simulator_param_ptr_->getPropagationLatencyUs() << " us; keystr: " << MessageBase::getKeyFromMessage(message_ptr).getKeystr() << "; dstadrr: " << tmp_propagation_item.getNetworkAddr().toString() << "; srcaddr: " << message_ptr->getSourceAddr().toString();
                Util::dumpDebugMsg(instance_name_, oss.str());
                #endif
                propagate_(sleep_us);
                #ifdef DEBUG_PROPAGATION_SIMULATOR
                oss.clear();
                oss.str("");
                oss << "after sleep " << sleep_us << " us to simulate a propagation latency of " << propagation_simulator_param_ptr_->getPropagationLatencyUs() << " us; keystr: " << MessageBase::getKeyFromMessage(message_ptr).getKeystr() << "; dstadrr: " << tmp_propagation_item.getNetworkAddr().toString() << "; srcaddr: " << message_ptr->getSourceAddr().toString();
                Util::dumpDebugMsg(instance_name_, oss.str());
                #endif

                // Issue the message to the given address
                NetworkAddr dst_addr = tmp_propagation_item.getNetworkAddr();
                assert(dst_addr.isValidAddr());
                propagation_simulator_socket_client_ptr_->send(message_ptr, dst_addr);

                // Release the message
                assert(message_ptr != NULL);
                delete message_ptr;
                message_ptr = NULL;
            }
        }

        return;
    }

    void PropagationSimulator::checkPointers_() const
    {
        assert(propagation_simulator_param_ptr_ != NULL);
        return;
    }
        
    void PropagationSimulator::propagate_(const uint32_t& sleep_us)
    {
        // NOTE: NOT simulator propagation latency for sleep_us of 0 (e.g., skip propagation latency under warmup phase if enable warmup speedup)
        if (sleep_us > 0)
        {
            usleep(sleep_us);
        }
        return;
    }
}
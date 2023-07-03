/*
 * PropagationItem: a message to be issued by PropagationSimulator towards a network address after a propagation latency.
 * 
 * By Siyuan Sheng (2023.07.03).
 */

#ifndef PROPAGATION_ITEM_H
#define PROPAGATION_ITEM_H

#include <string>

#include "message/message_base.h"
#include "network/network_addr.h"

namespace covered
{
    class PropagationItem
    {
    public:
        PropagationItem();
        PropagationItem(MessageBase* message_ptr, const NetworkAddr& network_addr, const uint32_t& sleep_us);
        ~PropagationItem();

        PropagationItem& operator=(const PropagationItem& other);
    private:
        static const std::string kClassName;

        MessageBase* message_ptr_;
        NetworkAddr network_addr_;
        uint32_t sleep_us_;
    };
}



#endif
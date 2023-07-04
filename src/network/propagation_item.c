#include "network/propagation_item.h"

#include <assert.h>

namespace covered
{
    const std::string PropagationItem::kClassName("PropagationItem");

    PropagationItem::PropagationItem() : network_addr_()
    {
        message_ptr_ = NULL;
        sleep_us_ = 0;
    }

    PropagationItem::PropagationItem(MessageBase* message_ptr, const NetworkAddr& network_addr, const uint32_t& sleep_us)
    {
        assert(message_ptr != NULL);
        assert(network_addr_.isValidAddr());

        message_ptr_ = message_ptr;
        network_addr_ = network_addr;
        sleep_us_ = sleep_us;
    }

    PropagationItem::~PropagationItem()
    {
        // NOTE: no need to release message_ptr_, which will be released by PropagationSimultor after issuing the message
    }

    MessageBase* PropagationItem::getMessagePtr() const
    {
        return message_ptr_;
    }

    NetworkAddr PropagationItem::getNetworkAddr() const
    {
        return network_addr_;
    }

    uint32_t PropagationItem::getSleepUs() const
    {
        return sleep_us_;
    }

    PropagationItem& PropagationItem::operator=(const PropagationItem& other)
    {
        message_ptr_ = other.message_ptr_; // shallow copy
        network_addr_ = other.network_addr_;
        sleep_us_ = other.sleep_us_;
    }
}
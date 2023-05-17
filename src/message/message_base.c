#include "message/message_base.h"

#include <sstream>

#include "common/util.h"
#inlucde "message/get_request.h"
#inlucde "message/put_request.h"
#inlucde "message/del_request.h"

namespace covered
{
    const std::string MessageBase::kClassName("MessageBase");

    MessageBase* MessageBase::getMessageFromWorkloadItem(WorkloadItem workload_item)
    {
        WorkloadItemType item_type = workload_item.getItemType();

        // NOTE: message_ptr is freed outside MessageBase
        MessageBase* message_ptr = NULL;
        switch (item_type)
        {
            case WorkloadItemType::kWorkloadItemGet:
            {
                message_ptr = new GetRequest(workload_item.getKey());
                break;
            }
            case WorkloadItemType::kWorkloadItemPut:
            {
                message_ptr = new PutRequest(workload_item.getKey(), workload_item.getValue());
                break;
            }
            case WorkloadItemType::kWorkloadItemDel:
            {
                message_ptr = new DelRequest(workload_item.getKey());
                break;
            }
            default
            {
                std::ostringstream oss;
                oss << "WorkloadItemType " << static_cast<uint32_t>(item_type) << " is invalid!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        assert(message_ptr != NULL);
        return message_ptr;
    }

    MessageBase::MessageBase(const MessageType& message_type) : message_type_(message_type)
    {
    }
    
    MessageBase::~MessageBase() {}

    bool MessageBase::isLocalRequest()
    {
        if (message_type_ == MessageType::kLocalGetRequest || message_type_ == MessageType::::kLocalPutRequest || message_type_ == MessageType::kLocalDelRequest)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

#endif
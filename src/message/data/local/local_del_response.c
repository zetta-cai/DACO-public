#include "message/data/local/local_del_response.h"

namespace covered
{
    const std::string LocalDelResponse::kClassName("LocalDelResponse");

    LocalDelResponse::LocalDelResponse(const Key& key, const Hitflag& hitflag) : KeyByteMessage(key, static_cast<uint8_t>(hitflag), MessageType::kLocalDelResponse)
    {
    }

    LocalDelResponse::LocalDelResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    LocalDelResponse::~LocalDelResponse() {}

    Hitflag LocalDelResponse::getHitflag() const
    {
        uint8_t byte = getByte_();
        Hitflag hitflag = static_cast<Hitflag>(byte);
        return hitflag;
    }
}
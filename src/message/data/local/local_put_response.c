#include "message/data/local/local_put_response.h"

namespace covered
{
    const std::string LocalPutResponse::kClassName("LocalPutResponse");

    LocalPutResponse::LocalPutResponse(const Key& key, const Hitflag& hitflag) : KeyByteMessage(key, static_cast<uint8_t>(hitflag), MessageType::kLocalPutResponse)
    {
    }

    LocalPutResponse::LocalPutResponse(const DynamicArray& msg_payload) : KeyByteMessage(msg_payload)
    {
    }

    LocalPutResponse::~LocalPutResponse() {}

    Hitflag LocalPutResponse::getHitflag() const
    {
        uint8_t byte = getByte_();
        Hitflag hitflag = static_cast<Hitflag>(byte);
        return hitflag;
    }
}
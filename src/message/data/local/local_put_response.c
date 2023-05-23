#include "message/data/local/local_put_response.h"

namespace covered
{
    const std::string LocalPutResponse::kClassName("LocalPutResponse");

    LocalPutResponse::LocalPutResponse(const Key& key, const Hitflag& hitflag) : KeyHitflagMessage(key, hitflag, MessageType::kLocalPutResponse)
    {
    }

    LocalPutResponse::LocalPutResponse(const DynamicArray& msg_payload) : KeyMessage(msg_payload)
    {
    }

    LocalPutResponse::~LocalPutResponse() {}
}
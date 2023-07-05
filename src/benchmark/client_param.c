#include "benchmark/client_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : NodeParamBase(NodeParamBase::CLIENT_NODE_ROLE, 0, false)
    {
    }

    ClientParam::ClientParam(const uint32_t& client_idx) : NodeParamBase(NodeParamBase::CLIENT_NODE_ROLE, client_idx, false)
    {
    }
        
    ClientParam::~ClientParam() {}

    const ClientParam& ClientParam::operator=(const ClientParam& other)
    {
        NodeParamBase::operator=(other);

        return *this;
    }
}
#include "cloud/cloud_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string CloudParam::kClassName("CloudParam");

    CloudParam::CloudParam() : NodeParamBase(NodeParamBase::CLOUD_NODE_ROLE, 0, true)
    {
    }

    CloudParam::~CloudParam() {}

    const CloudParam& CloudParam::operator=(const CloudParam& other)
    {
        NodeParamBase::operator=(other);
        
        assert(node_idx_ == 0); // TODO: only support 1 cloud node now!
        
        return *this;
    }
}
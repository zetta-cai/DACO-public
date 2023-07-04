#include "edge/edge_param.h"

#include "common/util.h"

namespace covered
{
    const std::string EdgeParam::kClassName("EdgeParam");

    EdgeParam::EdgeParam() : NodeParamBase(0, true)
    {
    }

    EdgeParam::EdgeParam(uint32_t edge_idx) : NodeParamBase(edge_idx, true)
    {
    }

    EdgeParam::~EdgeParam() {}

    const EdgeParam& EdgeParam::operator=(const EdgeParam& other)
    {
        NodeParamBase::operator=(other);

        return *this;
    }
}
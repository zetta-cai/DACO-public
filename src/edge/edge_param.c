#include "edge/edge_param.h"

namespace covered
{
    const std::string EdgeParam::kClassName("EdgeParam");

    EdgeParam::EdgeParam()
    {
        global_edge_idx_ = 0;
    }

    EdgeParam::EdgeParam(uint32_t global_edge_idx)
    {
        global_edge_idx_ = global_edge_idx;
    }

    EdgeParam::~EdgeParam() {}

    const EdgeParam& EdgeParam::operator=(const EdgeParam& other)
    {
        global_edge_idx_ = other.global_edge_idx_;
    }

    uint32_t getGlobalEdgeIdx()
    {
        return global_edge_idx_;
    }
}
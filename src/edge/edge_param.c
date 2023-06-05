#include "edge/edge_param.h"

#include "common/util.h"

namespace covered
{
    const std::string EdgeParam::kClassName("EdgeParam");

    EdgeParam::EdgeParam() : current_edge_running_(true)
    {
        edge_idx_ = 0;
    }

    EdgeParam::EdgeParam(uint32_t edge_idx) : current_edge_running_(true)
    {
        edge_idx_ = edge_idx;
    }

    EdgeParam::~EdgeParam() {}

    const EdgeParam& EdgeParam::operator=(const EdgeParam& other)
    {
        current_edge_running_.store(other.current_edge_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        edge_idx_ = other.edge_idx_;
        return *this;
    }

    bool EdgeParam::isEdgeRunning()
    {
        return current_edge_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void EdgeParam::setEdgeRunning()
    {
        return current_edge_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void EdgeParam::resetEdgeRunning()
    {
        return current_edge_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    uint32_t EdgeParam::getEdgeIdx()
    {
        return edge_idx_;
    }
}
#include "edge/edge_param.h"

#include "common/util.h"

namespace covered
{
    const std::string EdgeParam::kClassName("EdgeParam");

    EdgeParam::EdgeParam() : edge_running_(true)
    {
        edge_idx_ = 0;
    }

    EdgeParam::EdgeParam(uint32_t edge_idx) : edge_running_(true)
    {
        edge_idx_ = edge_idx;
    }

    EdgeParam::~EdgeParam() {}

    const EdgeParam& EdgeParam::operator=(const EdgeParam& other)
    {
        edge_running_.store(other.edge_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        edge_idx_ = other.edge_idx_;
        return *this;
    }

    bool EdgeParam::isEdgeRunning() const
    {
        return edge_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void EdgeParam::setEdgeRunning()
    {
        return edge_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void EdgeParam::resetEdgeRunning()
    {
        return edge_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    uint32_t EdgeParam::getEdgeIdx() const
    {
        return edge_idx_;
    }
}
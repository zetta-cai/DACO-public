#include "common/node_param_base.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string NodeParamBase::kClassName("NodeParamBase");

    NodeParamBase::NodeParamBase() : node_running_(false)
    {
        node_idx_ = 0;
    }

    NodeParamBase::NodeParamBase(const uint32_t& node_idx, const bool& is_running) : node_running_(is_running)
    {
        node_idx_ = node_idx;
    }

    NodeParamBase::~NodeParamBase() {}

    const NodeParamBase& NodeParamBase::operator=(const NodeParamBase& other)
    {
        node_running_.store(other.node_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        node_idx_ = other.node_idx_;
        return *this;
    }

    uint32_t NodeParamBase::getNodeIdx() const
    {
        return node_idx_;
    }

    bool NodeParamBase::isNodeRunning() const
    {
        return node_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void NodeParamBase::setNodeRunning()
    {
        return node_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void NodeParamBase::resetNodeRunning()
    {
        return node_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }
}
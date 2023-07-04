#include "common/node_param_base.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string NodeParamBase::CLIENT_NODE_ROLE("client");
    const std::string NodeParamBase::EDGE_NODE_ROLE("edge");
    const std::string NodeParamBase::CLOUD_NODE_ROLE("cloud");

    const std::string NodeParamBase::kClassName("NodeParamBase");

    NodeParamBase::NodeParamBase() : node_role_(""), node_running_(false)
    {
        node_idx_ = 0;
    }

    NodeParamBase::NodeParamBase(const std::string& node_role, const uint32_t& node_idx, const bool& is_running) : node_role_(node_role), node_running_(is_running)
    {
        node_idx_ = node_idx;
    }

    NodeParamBase::~NodeParamBase() {}

    std::string NodeParamBase::getNodeRole() const
    {
        return node_role_;
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

    uint32_t NodeParamBase::getNodeIdx() const
    {
        return node_idx_;
    }

    const NodeParamBase& NodeParamBase::operator=(const NodeParamBase& other)
    {
        node_role_ = other.node_role_;
        node_running_.store(other.node_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        node_idx_ = other.node_idx_;
        return *this;
    }
}
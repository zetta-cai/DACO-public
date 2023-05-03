#include "edge/edge_param.h"

namespace covered
{
    const std::string EdgeParam::kClassName("EdgeParam");

    EdgeParam::EdgeParam() : local_edge_running_(true)
    {
        global_edge_idx_ = 0;
    }

    EdgeParam::EdgeParam(uint32_t global_edge_idx) : local_edge_running_(true)
    {
        global_edge_idx_ = global_edge_idx;
    }

    EdgeParam::~EdgeParam() {}

    const EdgeParam& EdgeParam::operator=(const EdgeParam& other)
    {
        global_edge_idx_ = other.global_edge_idx_;
        return *this;
    }

    bool EdgeParam::isEdgeRunning()
    {
        return local_edge_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void EdgeParam::setEdgeRunning()
    {
        return local_edge_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void EdgeParam::resetEdgeRunning()
    {
        return local_edge_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    uint32_t EdgeParam::getGlobalEdgeIdx()
    {
        return global_edge_idx_;
    }
}
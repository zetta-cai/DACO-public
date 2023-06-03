#include "cloud/cloud_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string CloudParam::kClassName("CloudParam");

    CloudParam::CloudParam() : local_cloud_running_(true)
    {
        global_cloud_idx_ = 0;
    }

    CloudParam::~CloudParam() {}

    const CloudParam& CloudParam::operator=(const CloudParam& other)
    {
        local_cloud_running_.store(other.local_cloud_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        assert(other.global_cloud_idx_ == 0); // TODO: only support 1 cloud node now!
        global_cloud_idx_ = other.global_cloud_idx_;
        return *this;
    }

    uint32_t CloudParam::getGlobalCloudIdx() const
    {
        return global_cloud_idx_;
    }

    bool CloudParam::isCloudRunning()
    {
        return local_cloud_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void CloudParam::setCloudRunning()
    {
        return local_cloud_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void CloudParam::resetCloudRunning()
    {
        return local_cloud_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }
}
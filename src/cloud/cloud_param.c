#include "cloud/cloud_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string CloudParam::kClassName("CloudParam");

    CloudParam::CloudParam() : cloud_running_(true)
    {
        cloud_idx_ = 0;
    }

    CloudParam::~CloudParam() {}

    const CloudParam& CloudParam::operator=(const CloudParam& other)
    {
        cloud_running_.store(other.cloud_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        assert(other.cloud_idx_ == 0); // TODO: only support 1 cloud node now!
        cloud_idx_ = other.cloud_idx_;
        return *this;
    }

    uint32_t CloudParam::getCloudIdx() const
    {
        return cloud_idx_;
    }

    bool CloudParam::isCloudRunning()
    {
        return cloud_running_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void CloudParam::setCloudRunning()
    {
        return cloud_running_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }

    void CloudParam::resetCloudRunning()
    {
        return cloud_running_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }
}
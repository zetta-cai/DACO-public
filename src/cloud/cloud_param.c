#include "cloud/cloud_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CloudParam::kClassName("CloudParam");

    CloudParam::CloudParam() : local_cloud_running_(true)
    {
    }

    CloudParam::~CloudParam() {}

    const CloudParam& CloudParam::operator=(const CloudParam& other)
    {
        local_cloud_running_.store(other.local_cloud_running_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        return *this;
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
#include "common/subthread_param_base.h"

#include "common/util.h"

namespace covered
{
    const uint32_t SubthreadParamBase::INITIALIZATION_WAIT_INTERVAL_US = 1000; // 1ms

    const std::string SubthreadParamBase::kClassName("SubthreadParamBase");

    SubthreadParamBase::SubthreadParamBase() : is_finish_initialization_(false)
    {
    }

    SubthreadParamBase::~SubthreadParamBase()
    {
    }

    const SubthreadParamBase& SubthreadParamBase::operator=(const SubthreadParamBase& other)
    {
        is_finish_initialization_.store(other.is_finish_initialization_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        return *this;
    }

    bool SubthreadParamBase::isFinishInitialization() const
    {
        return is_finish_initialization_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void SubthreadParamBase::markFinishInitialization()
    {
        is_finish_initialization_.store(true, Util::STORE_CONCURRENCY_ORDER);
    }
}
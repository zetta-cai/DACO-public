#include "cloud/cloud_component_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CloudComponentParam::kClassName("CloudComponentParam");

    CloudComponentParam::CloudComponentParam() : SubthreadParamBase()
    {
        cloud_wrapper_ptr_ = NULL;
    }

    CloudComponentParam::CloudComponentParam(CloudWrapper* cloud_wrapper_ptr) : SubthreadParamBase()
    {
        if (cloud_wrapper_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "cloud_wrapper_ptr is NULL!");
            exit(1);
        }
        cloud_wrapper_ptr_ = cloud_wrapper_ptr;
    }

    CloudComponentParam::~CloudComponentParam()
    {
        // NOTE: no need to delete cloud_wrapper_ptr_, as it is maintained outside CloudComponentParam
    }

    const CloudComponentParam& CloudComponentParam::operator=(const CloudComponentParam& other)
    {
        SubthreadParamBase::operator=(other);

        if (other.cloud_wrapper_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.cloud_wrapper_ptr_ is NULL!");
            exit(1);
        }
        cloud_wrapper_ptr_ = other.cloud_wrapper_ptr_;
        return *this;
    }

    CloudWrapper* CloudComponentParam::getCloudWrapperPtr() const
    {
        assert(cloud_wrapper_ptr_ != NULL);
        return cloud_wrapper_ptr_;
    }
}
/*
 * CloudComponentParam: parameters to launch a cloud component (e.g., cloud data server).
 * 
 * By Siyuan Sheng (2024.03.02).
 */

#ifndef CLOUD_COMPONENT_PARAM_H
#define CLOUD_COMPONENT_PARAM_H

#include <atomic>
#include <string>

namespace covered
{
    class CloudComponentParam;
}

#include "cloud/cloud_wrapper.h"
#include "common/subthread_param_base.h"

namespace covered
{
    class CloudComponentParam : public SubthreadParamBase
    {
    public:
        CloudComponentParam();
        CloudComponentParam(CloudWrapper* cloud_wrapper_ptr);
        ~CloudComponentParam();

        const CloudComponentParam& operator=(const CloudComponentParam& other);

        CloudWrapper* getCloudWrapperPtr() const;
    private:
        static const std::string kClassName;

        CloudWrapper* cloud_wrapper_ptr_;
    };
}

#endif
/*
 * WorkloadExtraParamBase: extra parameters of workloads.
 * 
 * By Siyuan Sheng (2024.02.15).
 */

#ifndef WORKLOAD_EXTRA_PARAM_BASE_H
#define WORKLOAD_EXTRA_PARAM_BASE_H

#include <string>

namespace covered
{
    class WorkloadExtraParamBase
    {
    public:
        WorkloadExtraParamBase();
        virtual ~WorkloadExtraParamBase();

        virtual const WorkloadExtraParamBase& operator=(const WorkloadExtraParamBase& other);
    private:
        static const std::string kClassName;
    };
}

#endif
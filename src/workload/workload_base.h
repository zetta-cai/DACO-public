/*
 * WorkloadBase: the base class for workload generator.
 *
 * Access global covered::Config and covered::Param for static and dynamic configurations.
 * Overwrite some workload parameters (e.g., numOps and numKeys) based on static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef WORKLOAD_BASE_H
#define WORKLOAD_BASE_H

#include <string>

#include "common/request.h"

namespace covered
{
    class WorkloadBase
    {
    public:
        WorkloadBase();
        virtual ~WorkloadBase();

        void validate();
        Request generateReq();
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters() = 0; // initialize workload parameters (e.g., by default or by loading config file)
        virtual void overwriteWorkloadParameters() = 0; // overwrite some workload patermers based on covered::Config and covered::Param
        virtual void createWorkloadGenerator() = 0; // create workload generator based on overwritten workload parameters

        virtual Request generateReqInternal() = 0;

        bool is_valid_;
        void checkIsValid();
    };
}

#endif
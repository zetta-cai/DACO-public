/*
 * WorkloadWrapperBase: the base class for workload generator to generate key-value pairs and requests.
 *
 * Access global covered::Config and covered::Param for static and dynamic configurations.
 * Overwrite some workload parameters (e.g., numOps and numKeys) based on static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef WORKLOAD_WRAPPER_BASE_H
#define WORKLOAD_WRAPPER_BASE_H

#include <random> // std::mt19937_64
#include <string>

#include "workload/workload_item.h"

namespace covered
{
    class WorkloadWrapperBase
    {
    public:
        // Workload generator type (only used in WorkloadWrapperBase)
        static const std::string FACEBOOK_WORKLOAD_NAME;

        static WorkloadWrapperBase* getWorkloadGenerator(const std::string& workload_name, const uint32_t& client_idx);

        WorkloadWrapperBase();
        virtual ~WorkloadWrapperBase();

        void validate(const uint32_t& client_idx);
        WorkloadItem generateItem(std::mt19937_64& request_randgen);
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters_() = 0; // initialize workload parameters (e.g., by default or by loading config file)
        virtual void overwriteWorkloadParameters_() = 0; // overwrite some workload patermers based on covered::Config and covered::Param
        virtual void createWorkloadGenerator_(const uint32_t& client_idx) = 0; // create workload generator based on overwritten workload parameters

        virtual WorkloadItem generateItemInternal_(std::mt19937_64& request_randgen) = 0;

        bool is_valid_;
        void checkIsValid_();
    };
}

#endif
/*
 * WorkloadBase: the base class for workload generator to generate key-value pairs and requests.
 *
 * Access global covered::Config and covered::Param for static and dynamic configurations.
 * Overwrite some workload parameters (e.g., numOps and numKeys) based on static/dynamic configurations.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef WORKLOAD_BASE_H
#define WORKLOAD_BASE_H

#include <string>
#include <random> // std::mt19937_64

#include "common/request.h"

namespace covered
{
    class WorkloadBase
    {
    public:
        static const std::string FACEBOOK_WORKLOAD_NAME;

        static WorkloadBase* getWorkloadGenerator(std::string workload_name, const uint32_t& global_client_idx);

        WorkloadBase();
        virtual ~WorkloadBase();

        void validate(const uint32_t& global_client_idx);
        Request generateReq(std::mt19937_64& request_randgen);
    private:
        static const std::string kClassName;

        virtual void initWorkloadParameters() = 0; // initialize workload parameters (e.g., by default or by loading config file)
        virtual void overwriteWorkloadParameters() = 0; // overwrite some workload patermers based on covered::Config and covered::Param
        virtual void createWorkloadGenerator(const uint32_t& global_client_idx) = 0; // create workload generator based on overwritten workload parameters

        virtual Request generateReqInternal(std::mt19937_64& request_randgen) = 0;

        bool is_valid_;
        void checkIsValid();
    };
}

#endif
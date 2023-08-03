/*
 * WorkloadParam: store CLI parameters for workload dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef WORKLOAD_PARAM_H
#define WORKLOAD_PARAM_H

#include <string>

namespace covered
{
    class WorkloadParam
    {
    public:
        // For workload name
        static const std::string FACEBOOK_WORKLOAD_NAME; // Workload generator type

        static void setParameters(const uint32_t& keycnt, const std::string& workload_name);

        static uint32_t getKeycnt();
        static std::string getWorkloadName();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkWorkloadName_();
        static void verifyIntegrity_();

        static void checkIsValid_();

        static bool is_valid_;

        static uint32_t keycnt_;
        static std::string workload_name_;
    };
}

#endif
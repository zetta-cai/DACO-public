/*
 * WorkloadParam: store CLI parameters for workload dynamic configurations.
 *
 * NOTE: different Params should NOT have overlapped fields, and each Param will be set at most once.
 *
 * NOTE: all parameters should be passed into each instance when launching threads, except those with no effect on results (see comments of "// NO effect on results").
 * 
 * By Siyuan Sheng (2023.08.02).
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

        static void setParameters(const std::string& workload_name);

        static std::string getWorkloadName();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkWorkloadName_();

        static void checkIsValid_();

        static bool is_valid_;

        static std::string workload_name_;
    };
}

#endif
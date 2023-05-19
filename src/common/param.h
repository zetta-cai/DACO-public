/*
 * Param: store CLI parameters for dynamic configurations.
 * 
 * By Siyuan Sheng (2023.04.10).
 */

#ifndef PARAM_H
#define PARAM_H

#include <string>

namespace covered
{
    class Param
    {
    public:
        static void setParameters(const bool& is_simulation, const uint32_t& clientcnt, const std::string& cloud_storage, const std::string& config_filepath, const bool& is_debug, const double& duration, const uint32_t& edgecnt, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const std::string& workload_name);

        static bool isSimulation();
        static std::string getCacheName();
        static uint32_t getCapacity();
        static uint32_t getClientcnt();
        static std::string getCloudStorage();
        static std::string getConfigFilepath();
        static bool isDebug();
        static double getDuration();
        static uint32_t getEdgecnt();
        static uint32_t getKeycnt();
        static uint32_t getOpcnt();
        static uint32_t getPerclientWorkercnt();
        static std::string getWorkloadName();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid_();

        static bool is_valid_;
        static bool is_simulation_;
        static std::string cache_name_;
        static uint32_t capacity_;
        static uint32_t clientcnt_;
        static std::string cloud_storage_;
        static std::string config_filepath_;
        static bool is_debug_;
        static double duration_;
        static uint32_t edgecnt_;
        static uint32_t keycnt_;
        static uint32_t opcnt_;
        static uint32_t perclient_workercnt_;
        static std::string workload_name_;
    };
}

#endif
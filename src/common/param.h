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
        static void setParameters(const std::string& config_filepath, const bool& is_debug, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& clientcnt, const uint32_t& perclient_workercnt);

        static std::string getConfigFilepath();
        static bool isDebug();
        static uint32_t getKeycnt();
        static uint32_t getOpcnt();
        static uint32_t getClientcnt();
        static uint32_t getPerclientWorkercnt();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid();

        static bool is_valid_;
        static std::string config_filepath_;
        static bool is_debug_;
        static uint32_t keycnt_;
        static uint32_t opcnt_;
        static uint32_t clientcnt_;
        static uint32_t perclient_workercnt_;
    };
}

#endif
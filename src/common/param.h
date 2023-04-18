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
        static void setParameters(const bool& is_debug);

        static bool isDebug();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid();

        static bool is_valid_;
        static bool is_debug_;
    };
}

#endif
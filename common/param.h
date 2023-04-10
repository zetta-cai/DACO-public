/*
 * Param: store CLI parameters.
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
        static bool isDebug();

        static void setValid(const bool& is_valid);
        static void setDebug(const bool& is_debug);

        static std::string toString();
    private:
        static const std::string class_name_;

        static void checkIsValid();

        static bool is_valid_;
        static bool is_debug_;
    };
}

#endif
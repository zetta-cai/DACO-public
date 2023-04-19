/*
 * Value: a general value with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef VALUE_H
#define VALUE_H

#include <string>

namespace covered
{
    class Value
    {
    public:
        Value();
        Value(const uint32_t& valuesize);
        ~Value();
    private:
        static const std::string kClassName;

        uint32_t valuesize_;
    };
}

#endif
/*
 * Value: a general value with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef VALUE_H
#define VALUE_H

namespace covered
{
    class Value
    {
    public:
        Value(const std::string& valuestr);
        ~Value();
    private:
        static const std::string kClassName;

        std::string valuestr_;
    }
}

#endif
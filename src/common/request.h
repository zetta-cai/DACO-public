/*
 * Request: a general request to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef REQUEST_H
#define REQUEST_H

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class Request
    {
    public:
        Request(const covered::Key& key, const covered::Value& value);
        ~Request();
    private:
        static const std::string kClassName;

        covered::Key key_;
        covered::Value value_;
    }
}

#endif
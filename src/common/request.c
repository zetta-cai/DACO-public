#include "common/request.h"

namespace covered
{
    const std::string Request::kClassName("Request");

    Request::Request(const Key& key, const Value& value)
    {
        key_ = key;
        value_ = value;
    }

    Request::~Request() {}

    Key Request::getKey()
    {
        return key_;
    }

    Value Request::getValue()
    {
        return value_;
    }
}
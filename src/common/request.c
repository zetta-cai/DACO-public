#include "common/request.h"

const std::string covered::Request::kClassName = "Request";

covered::Request::Request(const covered::Key& key, const covered::Value& value)
{
    key_ = key;
    value_ = value;
}

covered::Request::~Request() {}
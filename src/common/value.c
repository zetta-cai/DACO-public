#include "common/value.h"

const std::string covered::Value::kClassName = "Value";

covered::Value::Value(const std::string& valuestr)
{
    valuestr_ = valuestr;
}

covered::Value::~Value() {}
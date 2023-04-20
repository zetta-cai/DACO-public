#include "common/value.h"

namespace covered
{
    const std::string Value::kClassName("Value");

    Value::Value()
    {
        valuesize_ = 0;
    }

    Value::Value(const uint32_t& valuesize)
    {
        valuesize_ = valuesize;
    }

    Value::~Value() {}
}
#include "common/key.h"

namespace covered
{
    const std::string Key::kClassName = "Key";

    Key::Key()
    {
        keystr_ = "";
    }

    Key::Key(const std::string& keystr)
    {
        keystr_ = keystr;
    }

    Key::~Key() {}
}
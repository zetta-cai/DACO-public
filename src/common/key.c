#include "common/key.h"

const std::string covered::Key::kClassName = "Key";

covered::Key::Key(const std::string& keystr)
{
    keystr_ = keystr;
}

covered::Key::~Key() {}
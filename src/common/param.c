#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"
#include "common/param.h"

const std::string covered::Param::kClassName = "Param";

bool covered::Param::is_valid_ = false;
bool covered::Param::is_debug_ = false;

bool covered::Param::isDebug()
{
    checkIsValid();
    return is_debug_;
}

void covered::Param::setValid(const bool& is_valid)
{
    is_valid_ = is_valid;
    return;
}

void covered::Param::setDebug(const bool& is_debug)
{
    is_debug_ = is_debug;
    return;
}

std::string covered::Param::toString()
{
    checkIsValid();

    std::ostringstream oss;
    oss << "[Stored CLI parameters]" << std::endl;
    oss << "Debug: " << (is_debug_?"true":"false");
    return oss.str();
    
}

void covered::Param::checkIsValid()
{
    if (!is_valid_)
    {
        covered::Util::dumpErrorMsg(kClassName, "invalid Param (CLI parameters have not been set)!");
        exit(-1);
    }
    return;
}
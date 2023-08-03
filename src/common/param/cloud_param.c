#include "common/param/cloud_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string CloudParam::HDD_NAME = "hdd";

    const std::string CloudParam::kClassName("CloudParam");

    bool CloudParam::is_valid_ = false;

    std::string CloudParam::cloud_storage_ = "";

    void CloudParam::setParameters(const std::string& cloud_storage)
    {
        // NOTE: NOT rely on any other module
        if (is_valid_)
        {
            return; // NO need to set parameters once again
        }
        else
        {
            Util::dumpNormalMsg(kClassName, "invoke setParameters()!");
        }

        cloud_storage_ = cloud_storage;
        checkCloudStorage_();

        is_valid_ = true;
        return;
    }

    std::string CloudParam::getCloudStorage()
    {
        checkIsValid_();
        return cloud_storage_;
    }

    std::string CloudParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Cloud storage: " << cloud_storage_;

        return oss.str();  
    }

    void CloudParam::checkCloudStorage_()
    {
        if (cloud_storage_ != HDD_NAME)
        {
            std::ostringstream oss;
            oss << "cloud storage " << cloud_storage_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void CloudParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid CloudParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
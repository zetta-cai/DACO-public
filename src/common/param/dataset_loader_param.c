#include "common/param/dataset_loader_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string DatasetLoaderParam::kClassName("DatasetLoaderParam");

    bool DatasetLoaderParam::is_valid_ = false;

    uint32_t DatasetLoaderParam::dataset_loadercnt_ = 0;
    uint32_t DatasetLoaderParam::cloud_idx_ = 0;

    void DatasetLoaderParam::setParameters(const uint32_t& dataset_loadercnt, const uint32_t& cloud_idx)
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

        dataset_loadercnt_ = dataset_loadercnt;
        cloud_idx_ = cloud_idx;
        
        verifyIntegrity_();

        is_valid_ = true;
        return;
    }

    uint32_t DatasetLoaderParam::getDatasetLoadercnt()
    {
        checkIsValid_();
        return dataset_loadercnt_;
    }

    uint32_t DatasetLoaderParam::getCloudIdx()
    {
        checkIsValid_();
        return cloud_idx_;
    }

    std::string DatasetLoaderParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Dataset loader count: " << dataset_loadercnt_ << std::endl;
        oss << "Cloud index: " << cloud_idx_;

        return oss.str();  
    }

    void DatasetLoaderParam::verifyIntegrity_()
    {
        assert(dataset_loadercnt_ > 0);

        return;
    }

    void DatasetLoaderParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid DatasetLoaderParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
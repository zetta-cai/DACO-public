/*
 * DatasetLoaderParam: store CLI parameters for dataset loader dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef DATASET_LOADER_PARAM_H
#define DATASET_LOADER_PARAM_H

#include <string>

namespace covered
{
    class DatasetLoaderParam
    {
    public:
        static void setParameters(const uint32_t& dataset_loadercnt, const uint32_t& cloud_idx);

        static uint32_t getDatasetLoadercnt();
        static uint32_t getCloudIdx();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void verifyIntegrity_();

        static void checkIsValid_();

        static bool is_valid_;

        static uint32_t dataset_loadercnt_;
        static uint32_t cloud_idx_;
    };
}

#endif
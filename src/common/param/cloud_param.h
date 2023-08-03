/*
 * CloudParam: store CLI parameters for cloud dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef CLOUD_PARAM_H
#define CLOUD_PARAM_H

#include <string>

namespace covered
{
    class CloudParam
    {
    public:
        // For cloud storage
        static const std::string HDD_NAME; // NOTE: a single RocksDB size on HDD should NOT exceed 500 GiB

        static void setParameters(const std::string& cloud_storage);

        static std::string getCloudStorage();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkCloudStorage_();

        static void checkIsValid_();

        static bool is_valid_;

        static std::string cloud_storage_;
    };
}

#endif
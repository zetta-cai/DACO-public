/*
 * EdgeParam: store CLI parameters for edge dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGE_PARAM_H
#define EDGE_PARAM_H

#include <string>

namespace covered
{
    class EdgeParam
    {
    public:
        // For cache name
        static const std::string LRU_CACHE_NAME;
        static const std::string COVERED_CACHE_NAME;
        // For hash name
        static const std::string MMH3_HASH_NAME;
        
        static void setParameters(const std::string& cache_name, const uint64_t& capacity_bytes, const std::string& hash_name, const uint32_t& percacheserver_workercnt);

        static std::string getCacheName();
        static uint64_t getCapacityBytes();
        static std::string getHashName();
        static uint32_t getPercacheserverWorkercnt();

        static bool isValid();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid_();

        static void checkCacheName_();
        static void checkHashName_();
        static void verifyIntegrity_();

        static bool is_valid_;

        static std::string cache_name_;
        static uint64_t capacity_bytes_;
        static std::string hash_name_;
        static uint32_t percacheserver_workercnt_;
    };
}

#endif
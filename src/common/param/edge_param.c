#include "common/param/edge_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/param/client_param.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgeParam::LRU_CACHE_NAME("lru");
    const std::string EdgeParam::COVERED_CACHE_NAME("covered");
    const std::string EdgeParam::MMH3_HASH_NAME("mmh3");

    const std::string EdgeParam::kClassName("EdgeParam");

    bool EdgeParam::is_valid_ = false;

    std::string EdgeParam::cache_name_ = "";
    uint64_t EdgeParam::capacity_bytes_ = 0;
    std::string EdgeParam::hash_name_ = "";
    uint32_t EdgeParam::percacheserver_workercnt_ = 0;

    void EdgeParam::setParameters(const std::string& cache_name, const uint64_t& capacity_bytes, const std::string& hash_name, const uint32_t& percacheserver_workercnt)
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

        cache_name_ = cache_name;
        checkCacheName_();
        capacity_bytes_ = capacity_bytes;
        hash_name_ = hash_name;
        checkHashName_();
        percacheserver_workercnt_ = percacheserver_workercnt;

        is_valid_ = true;
        return;
    }

    std::string EdgeParam::getCacheName()
    {
        checkIsValid_();
        return cache_name_;
    }

    uint64_t EdgeParam::getCapacityBytes()
    {
        checkIsValid_();
        return capacity_bytes_;
    }

    std::string EdgeParam::getHashName()
    {
        checkIsValid_();
        return hash_name_;
    }

    uint32_t EdgeParam::getPercacheserverWorkercnt()
    {
        checkIsValid_();
        return percacheserver_workercnt_;
    }
    
    bool EdgeParam::isValid()
    {
        return is_valid_;
    }

    std::string EdgeParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Cache name: " << cache_name_ << std::endl;
        oss << "Capacity (bytes): " << capacity_bytes_ << std::endl;
        oss << "Hash name: " << hash_name_ << std::endl;
        oss << "Per-cache-server worker count:" << percacheserver_workercnt_;

        return oss.str();  
    }

    void EdgeParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid EdgeParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }

    void EdgeParam::checkCacheName_()
    {
        if (cache_name_ != LRU_CACHE_NAME && cache_name_ != COVERED_CACHE_NAME)
        {
            std::ostringstream oss;
            oss << "cache name " << cache_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeParam::checkHashName_()
    {
        if (hash_name_ != MMH3_HASH_NAME)
        {
            std::ostringstream oss;
            oss << "hash name " << hash_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void EdgeParam::verifyIntegrity_()
    {
        assert(capacity_bytes_ > 0);
        assert(percacheserver_workercnt_ > 0);
        
        return;
    }
}
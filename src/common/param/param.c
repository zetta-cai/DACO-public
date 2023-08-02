#include "common/param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string Param::LRU_CACHE_NAME("lru");
    const std::string Param::COVERED_CACHE_NAME("covered");
    const std::string Param::HDD_NAME = "hdd";
    const std::string Param::MMH3_HASH_NAME("mmh3");

    const std::string Param::kClassName("Param");

    bool Param::is_valid_ = false;
    std::string Param::cache_name_ = "";
    uint64_t Param::capacity_bytes_ = 0;
    std::string Param::cloud_storage_ = "";
    uint32_t Param::edgecnt_ = 0;
    std::string Param::hash_name_ = "";
    uint32_t Param::max_warmup_duration_sec_ = 0;
    uint32_t Param::percacheserver_workercnt_ = 0;
    uint32_t Param::propagation_latency_clientedge_us_ = 0;
    uint32_t Param::propagation_latency_crossedge_us_ = 0;
    uint32_t Param::propagation_latency_edgecloud_us_ = 0;
    uint32_t Param::stresstest_duration_sec_ = 0;

    void Param::setParameters(const std::string& cache_name, const uint64_t& capacity_bytes, const std::string& cloud_storage, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& max_warmup_duration_sec, const uint32_t& percacheserver_workercnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const uint32_t& stresstest_duration_sec)
    {
        // NOTE: Param::setParameters() does NOT rely on any other module
        if (is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "Param::setParameters cannot be invoked more than once!");
            exit(1);
        }

        cache_name_ = cache_name;
        checkCacheName_();
        capacity_bytes_ = capacity_bytes;
        cloud_storage_ = cloud_storage;
        checkCloudStorage_();
        edgecnt_ = edgecnt;
        hash_name_ = hash_name;
        checkHashName_();
        max_warmup_duration_sec_ = max_warmup_duration_sec;
        percacheserver_workercnt_ = percacheserver_workercnt;
        propagation_latency_clientedge_us_ = propagation_latency_clientedge_us;
        propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
        propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;
        stresstest_duration_sec_ = stresstest_duration_sec;

        verifyIntegrity_();

        is_valid_ = true;
        return;
    }

    std::string Param::getCacheName()
    {
        checkIsValid_();
        return cache_name_;
    }

    uint64_t Param::getCapacityBytes()
    {
        checkIsValid_();
        return capacity_bytes_;
    }

    std::string Param::getCloudStorage()
    {
        checkIsValid_();
        return cloud_storage_;
    }

    uint32_t Param::getEdgecnt()
    {
        checkIsValid_();
        return edgecnt_;
    }

    std::string Param::getHashName()
    {
        checkIsValid_();
        return hash_name_;
    }

    uint32_t Param::getMaxWarmupDurationSec()
    {
        checkIsValid_();
        return max_warmup_duration_sec_;
    }

    uint32_t Param::getPercacheserverWorkercnt()
    {
        checkIsValid_();
        return percacheserver_workercnt_;
    }

    uint32_t Param::getPropagationLatencyClientedgeUs()
    {
        checkIsValid_();
        return propagation_latency_clientedge_us_;
    }

    uint32_t Param::getPropagationLatencyCrossedgeUs()
    {
        checkIsValid_();
        return propagation_latency_crossedge_us_;
    }

    uint32_t Param::getPropagationLatencyEdgecloudUs()
    {
        checkIsValid_();
        return propagation_latency_edgecloud_us_;
    }

    uint32_t Param::getStresstestDurationSec()
    {
        checkIsValid_();
        return stresstest_duration_sec_;
    }

    std::string Param::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters]" << std::endl;
        oss << "Cache name: " << cache_name_ << std::endl;
        oss << "Capacity (bytes): " << capacity_bytes_ << std::endl;
        oss << "Cloud storage: " << cloud_storage_ << std::endl;
        oss << "Edge count: " << edgecnt_ << std::endl;
        oss << "Hash name: " << hash_name_ << std::endl;
        oss << "Max warmup duration seconds: " << max_warmup_duration_sec_ << std::endl;
        oss << "Per-cache-server worker count:" << percacheserver_workercnt_ << std::endl;
        oss << "One-way propagation latency between client and edge: " << propagation_latency_clientedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and edge: " << propagation_latency_crossedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and cloud: " << propagation_latency_edgecloud_us_ << "us" << std::endl;
        oss << "Stresstest duration seconds: " << stresstest_duration_sec_ << std::endl;
        return oss.str();  
    }

    void Param::checkCacheName_()
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

    void Param::checkCloudStorage_()
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

    void Param::checkHashName_()
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

    void Param::verifyIntegrity_()
    {
        assert(capacity_bytes_ > 0);
        assert(edgecnt_ > 0);
        
        return;
    }

    void Param::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid Param (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}
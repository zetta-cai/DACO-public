#include "common/param.h"

#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string Param::SIMULATOR_MAIN_NAME("simulator");
    const std::string Param::STATISTICS_AGGREGATOR_MAIN_NAME("statistics_aggregator");
    const std::string Param::CLIENT_MAIN_NAME("client");
    const std::string Param::EDGE_MAIN_NAME("edge");
    const std::string Param::CLOUD_MAIN_NAME("cloud");
    const std::string Param::LRU_CACHE_NAME("lru");
    const std::string Param::COVERED_CACHE_NAME("covered");
    const std::string Param::HDD_NAME = "hdd";
    const std::string Param::MMH3_HASH_NAME("mmh3");
    const std::string Param::FACEBOOK_WORKLOAD_NAME("facebook");

    const std::string Param::kClassName("Param");

    std::string Param::main_class_name_ = "";
    bool Param::is_valid_ = false;
    bool Param::is_simulation_ = true;
    std::string Param::cache_name_ = "";
    uint32_t Param::capacity_bytes_ = 0;
    uint32_t Param::clientcnt_ = 0;
    std::string Param::cloud_storage_ = "";
    std::string Param::config_filepath_ = "";
    bool Param::is_debug_ = false;
    double Param::duration_ = 0.0;
    uint32_t Param::edgecnt_ = 0;
    std::string Param::hash_name_ = "";
    uint32_t Param::keycnt_ = 0;
    uint32_t Param::opcnt_ = 0;
    uint32_t Param::perclient_workercnt_ = 0;
    uint32_t Param::propagation_latency_clientedge_ = 0;
    uint32_t Param::propagation_latency_crossedge_ = 0;
    uint32_t Param::propagation_latency_edgecloud_ = 0;
    std::string Param::workload_name_ = "";

    void Param::setParameters(const std::string& main_class_name, const bool& is_simulation, const std::string& cache_name, const uint32_t& capacity_bytes, const uint32_t& clientcnt, const std::string& cloud_storage, const std::string& config_filepath, const bool& is_debug, const double& duration, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt, const uint32_t& propagation_latency_clientedge, const uint32_t& propagation_latency_crossedge, const uint32_t& propagation_latency_edgecloud, const std::string& workload_name)
    {
        // NOTE: Param::setParameters() does NOT rely on any other module
        if (is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "Param::setParameters cannot be invoked more than once!");
            exit(1);
        }

        main_class_name_ = main_class_name;
        checkMainClassName_();
        is_simulation_ = is_simulation;
        cache_name_ = cache_name;
        checkCacheName_();
        capacity_bytes_ = capacity_bytes;
        clientcnt_ = clientcnt;
        cloud_storage_ = cloud_storage;
        checkCloudStorage_();
        config_filepath_ = config_filepath;
        is_debug_ = is_debug;
        duration_ = duration;
        edgecnt_ = edgecnt;
        hash_name_ = hash_name;
        checkHashName_();
        keycnt_ = keycnt;
        opcnt_ = opcnt;
        perclient_workercnt_ = perclient_workercnt;
        propagation_latency_clientedge_ = propagation_latency_clientedge;
        propagation_latency_crossedge_ = propagation_latency_crossedge;
        propagation_latency_edgecloud_ = propagation_latency_edgecloud;
        workload_name_ = workload_name;
        checkWorkloadName_();

        is_valid_ = true;
        return;
    }

    std::string Param::getMainClassName()
    {
        checkIsValid_();
        return main_class_name_;
    }

    bool Param::isSimulation()
    {
        checkIsValid_();
        return is_simulation_;
    }

    std::string Param::getCacheName()
    {
        checkIsValid_();
        return cache_name_;
    }

    uint32_t Param::getCapacityBytes()
    {
        checkIsValid_();
        return capacity_bytes_;
    }

    uint32_t Param::getClientcnt()
    {
        checkIsValid_();
        return clientcnt_;
    }

    std::string Param::getCloudStorage()
    {
        checkIsValid_();
        return cloud_storage_;
    }

    std::string Param::getConfigFilepath()
    {
        checkIsValid_();
        return config_filepath_;
    }

    bool Param::isDebug()
    {
        checkIsValid_();
        return is_debug_;
    }

    double Param::getDuration()
    {
        checkIsValid_();
        return duration_;
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

    uint32_t Param::getKeycnt()
    {
        checkIsValid_();
        return keycnt_;
    }

    uint32_t Param::getOpcnt()
    {
        checkIsValid_();
        return opcnt_;
    }

    uint32_t Param::getPerclientWorkercnt()
    {
        checkIsValid_();
        return perclient_workercnt_;
    }

    uint32_t Param::getPropagationLatencyClientedge()
    {
        checkIsValid_();
        return propagation_latency_clientedge_;
    }

    uint32_t Param::getPropagationLatencyCrossedge()
    {
        checkIsValid_();
        return propagation_latency_crossedge_;
    }

    uint32_t Param::getPropagationLatencyEdgecloud()
    {
        checkIsValid_();
        return propagation_latency_edgecloud_;
    }

    std::string Param::getWorkloadName()
    {
        checkIsValid_();
        return workload_name_;
    }

    std::string Param::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters]" << std::endl;
        oss << "Cache name: " << cache_name_ << std::endl;
        oss << "Capacity (bytes): " << capacity_bytes_ << std::endl;
        oss << "Client count: " << clientcnt_ << std::endl;
        oss << "Cloud storage: " << cloud_storage_ << std::endl;
        oss << "Config filepath: " << config_filepath_ << std::endl;
        oss << "Debug flag: " << (is_debug_?"true":"false") << std::endl;
        oss << "Duration: " << duration_ << std::endl;
        oss << "Edge count: " << edgecnt_ << std::endl;
        oss << "Hash name: " << hash_name_ << std::endl;
        oss << "Key count (dataset size): " << keycnt_ << std::endl;
        oss << "Operation count (workload size): " << opcnt_ << std::endl;
        oss << "Per-client worker count: " << perclient_workercnt_ << std::endl;
        oss << "One-way propagation latency between client and edge: " << propagation_latency_clientedge_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and edge: " << propagation_latency_crossedge_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and cloud: " << propagation_latency_edgecloud_ << "us" << std::endl;
        oss << "Workload name: " << workload_name_;
        return oss.str();
        
    }

    void Param::checkMainClassName_()
    {
        if (main_class_name_ != SIMULATOR_MAIN_NAME && main_class_name_ != STATISTICS_AGGREGATOR_MAIN_NAME && main_class_name_ != CLIENT_MAIN_NAME && main_class_name_ != EDGE_MAIN_NAME && main_class_name_ != CLOUD_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "main class name " << main_class_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, main_class_name_);
            exit(1);
        }
        return;
    }

    void Param::checkCacheName_()
    {
        if (cache_name_ != LRU_CACHE_NAME && cache_name_ != COVERED_CACHE_NAME)
        {
            std::ostringstream oss;
            oss << "cache name " << cache_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, cache_name_);
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
            Util::dumpErrorMsg(kClassName, cloud_storage_);
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
            Util::dumpErrorMsg(kClassName, hash_name_);
            exit(1);
        }
        return;
    }

    void Param::checkWorkloadName_()
    {
        if (workload_name_ != FACEBOOK_WORKLOAD_NAME)
        {
            std::ostringstream oss;
            oss << "workload name " << workload_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, workload_name_);
            exit(1);
        }
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
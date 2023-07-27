#include "common/param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/util.h"

namespace covered
{
    const std::string Param::SIMULATOR_MAIN_NAME("simulator");
    //const std::string Param::STATISTICS_AGGREGATOR_MAIN_NAME("statistics_aggregator");
    const std::string Param::TOTAL_STATISTICS_LOADER_MAIN_NAME("total_statistics_loader");
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
    bool Param::is_single_node_ = true;
    std::string Param::cache_name_ = "";
    uint64_t Param::capacity_bytes_ = 0;
    uint32_t Param::clientcnt_ = 0;
    std::string Param::cloud_storage_ = "";
    std::string Param::config_filepath_ = "";
    bool Param::is_debug_ = false;
    uint32_t Param::duration_sec_ = 0;
    uint32_t Param::edgecnt_ = 0;
    std::string Param::hash_name_ = "";
    uint32_t Param::keycnt_ = 0;
    uint32_t Param::opcnt_ = 0;
    uint32_t Param::percacheserver_workercnt_ = 0;
    uint32_t Param::perclient_workercnt_ = 0;
    uint32_t Param::propagation_latency_clientedge_us_ = 0;
    uint32_t Param::propagation_latency_crossedge_us_ = 0;
    uint32_t Param::propagation_latency_edgecloud_us_ = 0;
    bool Param::track_event_ = false;
    std::string Param::workload_name_ = "";

    void Param::setParameters(const std::string& main_class_name, const bool& is_single_node, const std::string& cache_name, const uint64_t& capacity_bytes, const uint32_t& clientcnt, const std::string& cloud_storage, const std::string& config_filepath, const bool& is_debug, const uint32_t& duration_sec, const uint32_t& edgecnt, const std::string& hash_name, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& percacheserver_workercnt, const uint32_t& perclient_workercnt, const uint32_t& propagation_latency_clientedge_us, const uint32_t& propagation_latency_crossedge_us, const uint32_t& propagation_latency_edgecloud_us, const bool& track_event, const std::string& workload_name)
    {
        // NOTE: Param::setParameters() does NOT rely on any other module
        if (is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "Param::setParameters cannot be invoked more than once!");
            exit(1);
        }

        main_class_name_ = main_class_name;
        checkMainClassName_();
        is_single_node_ = is_single_node;
        cache_name_ = cache_name;
        checkCacheName_();
        capacity_bytes_ = capacity_bytes;
        clientcnt_ = clientcnt;
        cloud_storage_ = cloud_storage;
        checkCloudStorage_();
        config_filepath_ = config_filepath;
        is_debug_ = is_debug;
        duration_sec_ = duration_sec;
        edgecnt_ = edgecnt;
        hash_name_ = hash_name;
        checkHashName_();
        keycnt_ = keycnt;
        opcnt_ = opcnt;
        percacheserver_workercnt_ = percacheserver_workercnt;
        perclient_workercnt_ = perclient_workercnt;
        propagation_latency_clientedge_us_ = propagation_latency_clientedge_us;
        propagation_latency_crossedge_us_ = propagation_latency_crossedge_us;
        propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us;
        track_event_ = track_event;
        workload_name_ = workload_name;
        checkWorkloadName_();

        verifyIntegrity_();

        is_valid_ = true;
        return;
    }

    std::string Param::getMainClassName()
    {
        checkIsValid_();
        return main_class_name_;
    }

    bool Param::isSingleNode()
    {
        checkIsValid_();
        return is_single_node_;
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

    uint32_t Param::getDurationSec()
    {
        checkIsValid_();
        return duration_sec_;
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

    uint32_t Param::getPercacheserverWorkercnt()
    {
        checkIsValid_();
        return percacheserver_workercnt_;
    }

    uint32_t Param::getPerclientWorkercnt()
    {
        checkIsValid_();
        return perclient_workercnt_;
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

    bool Param::isTrackEvent()
    {
        checkIsValid_();
        return track_event_;
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
        oss << "Duration seconds: " << duration_sec_ << std::endl;
        oss << "Edge count: " << edgecnt_ << std::endl;
        oss << "Hash name: " << hash_name_ << std::endl;
        oss << "Key count (dataset size): " << keycnt_ << std::endl;
        oss << "Operation count (workload size): " << opcnt_ << std::endl;
        oss << "Per-cache-server worker count:" << percacheserver_workercnt_ << std::endl;
        oss << "Per-client worker count: " << perclient_workercnt_ << std::endl;
        oss << "One-way propagation latency between client and edge: " << propagation_latency_clientedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and edge: " << propagation_latency_crossedge_us_ << "us" << std::endl;
        oss << "One-way propagation latency between edge and cloud: " << propagation_latency_edgecloud_us_ << "us" << std::endl;
        oss << "Track event flag: " << (track_event_?"true":"false") << std::endl;
        oss << "Workload name: " << workload_name_;
        return oss.str();
        
    }

    void Param::checkMainClassName_()
    {
        if (main_class_name_ != SIMULATOR_MAIN_NAME && main_class_name_ != STATISTICS_AGGREGATOR_MAIN_NAME && main_class_name_ != CLIENT_MAIN_NAME && main_class_name_ != EDGE_MAIN_NAME && main_class_name_ != CLOUD_MAIN_NAME)
        {
            std::ostringstream oss;
            oss << "main class name " << main_class_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
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

    void Param::checkWorkloadName_()
    {
        if (workload_name_ != FACEBOOK_WORKLOAD_NAME)
        {
            std::ostringstream oss;
            oss << "workload name " << workload_name_ << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    void Param::verifyIntegrity_()
    {
        assert(capacity_bytes_ > 0);
        assert(clientcnt_ > 0);
        assert(edgecnt_ > 0);
        assert(keycnt_ > 0);
        assert(opcnt_ > 0);
        assert(perclient_workercnt_ > 0);

        if (clientcnt_ < edgecnt_)
        {
            std::ostringstream oss;
            oss << "clientcnt " << clientcnt_ << " should >= edgecnt " << edgecnt_ << " for edge-client mapping!";
            Util::dumpErrorMsg(kClassName, oss.str());
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
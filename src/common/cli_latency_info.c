#include "common/cli_latency_info.h"

namespace covered
{
    const std::string CLILatencyInfo::kClassName("CLILatencyInfo");

    CLILatencyInfo::CLILatencyInfo()
    {
        propagation_latency_distname_ = "";
        propagation_latency_clientedge_lbound_us_ = 0;
        propagation_latency_clientedge_avg_us_ = 0;
        propagation_latency_clientedge_rbound_us_ = 0;
        propagation_latency_crossedge_lbound_us_ = 0;
        propagation_latency_crossedge_avg_us_ = 0;
        propagation_latency_crossedge_rbound_us_ = 0;
        propagation_latency_edgecloud_lbound_us_ = 0;
        propagation_latency_edgecloud_avg_us_ = 0;
        propagation_latency_edgecloud_rbound_us_ = 0;
    }

    CLILatencyInfo::CLILatencyInfo(const std::string& propagation_latency_distname, const uint32_t& propagation_latency_clientedge_lbound_us, const uint32_t& propagation_latency_clientedge_avg_us, const uint32_t& propagation_latency_clientedge_rbound_us, const uint32_t& propagation_latency_crossedge_lbound_us, const uint32_t& propagation_latency_crossedge_avg_us, const uint32_t& propagation_latency_crossedge_rbound_us, const uint32_t& propagation_latency_edgecloud_lbound_us, const uint32_t& propagation_latency_edgecloud_avg_us, const uint32_t& propagation_latency_edgecloud_rbound_us, const std::string& p2p_latency_mat_path)
    {
        propagation_latency_distname_ = propagation_latency_distname;
        propagation_latency_clientedge_lbound_us_ = propagation_latency_clientedge_lbound_us;
        propagation_latency_clientedge_avg_us_ = propagation_latency_clientedge_avg_us;
        propagation_latency_clientedge_rbound_us_ = propagation_latency_clientedge_rbound_us;
        propagation_latency_crossedge_lbound_us_ = propagation_latency_crossedge_lbound_us;
        propagation_latency_crossedge_avg_us_ = propagation_latency_crossedge_avg_us;
        propagation_latency_crossedge_rbound_us_ = propagation_latency_crossedge_rbound_us;
        propagation_latency_edgecloud_lbound_us_ = propagation_latency_edgecloud_lbound_us;
        propagation_latency_edgecloud_avg_us_ = propagation_latency_edgecloud_avg_us;
        propagation_latency_edgecloud_rbound_us_ = propagation_latency_edgecloud_rbound_us;

        if(!p2p_latency_mat_path.empty()){
            DelayConfig config = DelayConfigReader::readFromFile(p2p_latency_mat_path);
            p2p_latency_mat_path_ = p2p_latency_mat_path;
            latency_distribution = config.distribution;
            p2p_latency_mat = config.delay_matrix;

        }
    }

    CLILatencyInfo::~CLILatencyInfo() {}

    std::string CLILatencyInfo::getPropagationLatencyDistname() const
    {
        return propagation_latency_distname_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyClientedgeLboundUs() const
    {
        return propagation_latency_clientedge_lbound_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyClientedgeAvgUs() const
    {
        return propagation_latency_clientedge_avg_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyClientedgeRboundUs() const
    {
        return propagation_latency_clientedge_rbound_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyCrossedgeLboundUs() const
    {
        return propagation_latency_crossedge_lbound_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyCrossedgeAvgUs() const
    {
        return propagation_latency_crossedge_avg_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyCrossedgeRboundUs() const
    {
        return propagation_latency_crossedge_rbound_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyEdgecloudLboundUs() const
    {
        return propagation_latency_edgecloud_lbound_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyEdgecloudAvgUs() const
    {
        return propagation_latency_edgecloud_avg_us_;
    }

    uint32_t CLILatencyInfo::getPropagationLatencyEdgecloudRboundUs() const
    {
        return propagation_latency_edgecloud_rbound_us_;
    }

    std::string CLILatencyInfo::getPropagationP2PLatencyMatPath() const
    {
        return p2p_latency_mat_path_;
    }
    std::vector<std::vector<uint32_t>>  CLILatencyInfo::getP2PLatencyMatrix() const
    {
        return p2p_latency_mat;
    }
    CLILatencyInfo& CLILatencyInfo::operator=(const CLILatencyInfo& other)
    {
        if (this != &other)
        {
            propagation_latency_distname_ = other.propagation_latency_distname_;
            propagation_latency_clientedge_lbound_us_ = other.propagation_latency_clientedge_lbound_us_;
            propagation_latency_clientedge_avg_us_ = other.propagation_latency_clientedge_avg_us_;
            propagation_latency_clientedge_rbound_us_ = other.propagation_latency_clientedge_rbound_us_;
            propagation_latency_crossedge_lbound_us_ = other.propagation_latency_crossedge_lbound_us_;
            propagation_latency_crossedge_avg_us_ = other.propagation_latency_crossedge_avg_us_;
            propagation_latency_crossedge_rbound_us_ = other.propagation_latency_crossedge_rbound_us_;
            propagation_latency_edgecloud_lbound_us_ = other.propagation_latency_edgecloud_lbound_us_;
            propagation_latency_edgecloud_avg_us_ = other.propagation_latency_edgecloud_avg_us_;
            propagation_latency_edgecloud_rbound_us_ = other.propagation_latency_edgecloud_rbound_us_;
            p2p_latency_mat_path_ = other.p2p_latency_mat_path_;
            p2p_latency_mat = other.p2p_latency_mat;
            latency_distribution = other.latency_distribution;

        }
        return *this;
    }
}
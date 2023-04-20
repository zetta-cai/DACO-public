#include <pthread>

#include "common/util.h"
#include "common/param.h"
#include "common/config.h"
#include "benchmark/physical_client_util.h"

namespace covered
{
    const std::string PhysicalClientUtil::kClassName = "PhysicalClientUtil";

    uint16_t PhysicalClientUtil::getLocalClientWorkloadStartport(uint32_t global_client_idx)
    {
        int64_t local_client_workload_startport = 0;
        int64_t global_client_workload_startport = static_cast<int64_t>(covered::Config::getGlobalClientWorkloadStartport());
        if (covered::Param::isSimulation())
        {
            int64_t perclient_workercnt = static_cast<int64_t>(covered::Param::getPerclientWorkercnt());
            local_client_workload_startport = global_client_workload_startport + static_cast<int64_t>(global_client_idx) * perclient_workercnt;
        }
        else
        {
            local_client_workload_startport = global_client_workload_startport;
        }
        return covered::Util::toUint16(local_client_workload_startport);
    }

    std::string PhysicalClientUtil::getLocalEdgeNodeIpstr(uint32_t global_client_idx)
    {
        std::string local_edge_node_ipstr = "";
        if (covered::Param::isSimulation())
        {
            local_edge_node_ipstr = covered::Util::LOCALHOST_IPSTR;
        }
        else
        {
            // TODO: set local_edge_node_ipstr based on covered::Config
            covered::Util::dumpErrorMsg(kClassName, "NOT support getLocalEdgeNodeIpstr for prototype now!");
            exit(1);
        }
        return local_edge_node_ipstr;
    }
    
    void* PhysicalClientUtil::launchPhysicalClient(void* physical_client_param)
    {
        // TODO: END HERE
        
        pthread_exit(NULL);
        return NULL;
    }
}
#include "cooperation/covered_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string CoveredCooperationWrapper::kClassName("CoveredCooperationWrapper");

    CoveredCooperationWrapper::CoveredCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name) : CooperationWrapperBase(edgecnt, edge_idx, hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    CoveredCooperationWrapper::~CoveredCooperationWrapper() {}
}
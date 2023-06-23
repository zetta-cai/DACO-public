#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr) : CooperationWrapperBase(hash_name, edge_param_ptr)
    {
        // Differentiate CooperationWrapper in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}
}
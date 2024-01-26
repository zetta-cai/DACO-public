#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name) : CooperationWrapperBase(edgecnt, edge_idx, hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}

    // (0) Cache-method-specific custom functions

    void BasicCooperationWrapper::constCustomFunc(const std::string& funcname, CooperationCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "Unknown const custom function name: " << funcname;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }
}
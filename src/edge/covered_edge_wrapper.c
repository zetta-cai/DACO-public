#include "edge/covered_edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string CoveredEdgeWrapper::kClassName("CoveredEdgeWrapper");

    CoveredEdgeWrapper::CoveredEdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr) : EdgeWrapperBase(cache_name, hash_name, edge_param_ptr)
    {
        assert(cache_name == Param::COVERED_CACHE_NAME);
    }

    CoveredEdgeWrapper::~CoveredEdgeWrapper() {}

    // (1) Data requests

    bool CoveredEdgeWrapper::processRedirectedGetRequest_(MessageBase* redirected_request_ptr)
    {
        // TODO

        return false;
    }

    void CoveredEdgeWrapper::triggerIndependentAdmission_(const Key& key, const Value& value)
    {
        // NOTE: COVERED will NOT trigger any independent cache admission/eviction decision
        std::ostringstream oss;
        Util::dumpErrorMsg(kClassName, "triggerIndependentAdmission_() should NOT be invoked in CoveredEdgeWrapper!");
        exit(1);
        return;
    }

    // (2) Control requests

    bool CoveredEdgeWrapper::processDirectoryLookupRequest_(MessageBase* control_request_ptr)
    {
        // TODO

        // NOTE: For COVERED, beacon node will tell the closest edge node if to admit, w/o independent decision

        return false;
    }

    bool CoveredEdgeWrapper::processDirectoryUpdateRequest_(MessageBase* control_request_ptr)
    {
        // TODO

        // NOTE: For COVERED, beacon node will tell the closest edge node if to admit, w/o independent decision

        return false;
    }

    bool CoveredEdgeWrapper::processOtherControlRequest_(MessageBase* control_request_ptr)
    {
        // TODO

        return false;
    }
}
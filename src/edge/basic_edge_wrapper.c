#include "edge/basic_edge_wrapper.h"

#include <assert.h>

namespace covered
{
    BasicEdgeWrapper::BasicEdgeWrapper(const std::string& cache_name, EdgeParam* edge_param_ptr) : EdgeWrapperBase(cache_name, edge_param_ptr)
    {
    }

    BasicEdgeWrapper::~BasicEdgeWrapper() {}

    bool BasicEdgeWrapper::processControlRequest_(MessageBase* control_request_ptr)
    {
        assert(control_request_ptr != NULL && control_request_ptr->isControlRequest());
        assert(edge_cache_ptr_ != NULL);

        bool is_finish = false; // Mark if local edge node is finished

        // TODO: invalidation and cache admission/eviction requests for control message
        // TODO: reply control response message to a beacon node
        // assert(control_response_ptr != NULL && control_response_ptr->isControlResponse());
        return is_finish;
    }
}

#endif
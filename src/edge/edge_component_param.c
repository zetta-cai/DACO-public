#include "edge/edge_component_param.h"

#include "common/util.h"

namespace covered
{
    const std::string EdgeComponentParam::kClassName("EdgeComponentParam");

    EdgeComponentParam::EdgeComponentParam() : SubthreadParamBase()
    {
        edge_wrapper_ptr_ = NULL;
    }

    EdgeComponentParam::EdgeComponentParam(EdgeWrapperBase* edge_wrapper_ptr) : SubthreadParamBase()
    {
        if (edge_wrapper_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "edge_wrapper_ptr is NULL!");
            exit(1);
        }
        edge_wrapper_ptr_ = edge_wrapper_ptr;
    }

    EdgeComponentParam::~EdgeComponentParam()
    {
        // NOTE: no need to delete edge_wrapper_ptr_, as it is maintained outside EdgeComponentParam
    }

    const EdgeComponentParam& EdgeComponentParam::operator=(const EdgeComponentParam& other)
    {
        SubthreadParamBase::operator=(other);

        if (other.edge_wrapper_ptr_ == NULL)
        {
            Util::dumpErrorMsg(kClassName, "other.edge_wrapper_ptr_ is NULL!");
            exit(1);
        }
        edge_wrapper_ptr_ = other.edge_wrapper_ptr_;
        return *this;
    }

    EdgeWrapperBase* EdgeComponentParam::getEdgeWrapperPtr() const
    {
        assert(edge_wrapper_ptr_ != NULL);
        return edge_wrapper_ptr_;
    }
}
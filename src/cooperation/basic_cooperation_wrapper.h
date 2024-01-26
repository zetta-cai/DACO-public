/*
 * BasicCooperationWrapper: basic cooperative caching framework for baselines.
 *
 * NOTE: all non-const shared variables in BasicCooperationWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef BASIC_COOPERATION_WRAPPER_H
#define BASIC_COOPERATION_WRAPPER_H

#include <string>

#include "cooperation/cooperation_wrapper_base.h"

namespace covered
{
    class BasicCooperationWrapper : public CooperationWrapperBase
    {
    public:
        BasicCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~BasicCooperationWrapper();

        // (0) Cache-method-specific custom functions

        virtual void constCustomFunc(const std::string& funcname, CooperationCustomFuncParamBase* func_param_ptr) const override;
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
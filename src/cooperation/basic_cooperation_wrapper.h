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

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"
#include "cooperation/directory_info.h"

namespace covered
{
    class BasicCooperationWrapper : public CooperationWrapperBase
    {
    public:
        BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~BasicCooperationWrapper();
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
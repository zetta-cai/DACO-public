/*
 * CoveredCooperationWrapper: basic cooperative caching framework for baselines.
 *
 * NOTE: all non-const shared variables in CoveredCooperationWrapper should be thread safe.
 * 
 * By Siyuan Sheng (2023.10.12).
 */

#ifndef COVERED_COOPERATION_WRAPPER_H
#define COVERED_COOPERATION_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"

namespace covered
{
    class CoveredCooperationWrapper : public CooperationWrapperBase
    {
    public:
        CoveredCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name);
        virtual ~CoveredCooperationWrapper();
    private:
        static const std::string kClassName;

        std::string instance_name_;
    };
}

#endif
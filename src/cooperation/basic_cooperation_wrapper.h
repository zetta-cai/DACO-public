/*
 * BasicCooperationWrapper: basic cooperative caching framework for baselines.
 * 
 * By Siyuan Sheng (2023.06.06).
 */

#ifndef BASIC_COOPERATION_WRAPPER_H
#define BASIC_COOPERATION_WRAPPER_H

#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_wrapper_base.h"

namespace covered
{
    class BasicCooperationWrapper : public CooperationWrapperBase
    {
    public:
        BasicCooperationWrapper(const std::string& hash_name, EdgeParam* edge_param_ptr);
        virtual ~BasicCooperationWrapper();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        virtual bool lookupBeaconDirectory_(const Key& key, bool& is_directory_exist, uint32_t& target_edge_idx) override;
        virtual bool redirectGetToTarget_(const Key& key, Value& value, bool& is_cooperative_cached) override;
        virtual bool updateBeaconDirectory_(const Key& key, const bool& is_admit, const uint32_t& target_edge_idx) override;
    };
}

#endif
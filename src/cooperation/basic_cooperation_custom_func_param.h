/*
 * Different custom function parameters of baselines for cooperation module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef BASIC_COOPERATION_CUSTOM_FUNC_PARAM_H
#define BASIC_COOPERATION_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "common/key.h"
#include "cooperation/cooperation_custom_func_param_base.h"
#include "cooperation/directory/directory_info.h"

namespace covered
{
    // PreserveDirectoryTableIfGlobalUncachedFuncParam

    class PreserveDirectoryTableIfGlobalUncachedFuncParam : public CooperationCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // get local-beaconed victim dirinfosets for neighbor-synced victims in victim syncset

        PreserveDirectoryTableIfGlobalUncachedFuncParam(const Key& key, const DirectoryInfo& directory_info);
        virtual ~PreserveDirectoryTableIfGlobalUncachedFuncParam();

        Key getKey() const;
        DirectoryInfo getDirectoryInfo() const;

        bool& isSuccessfulPreservationRef();
        const bool& isSuccessfulPreservationConstRef() const;
    private:
        static const std::string kClassName;

        const Key key_;
        const DirectoryInfo directory_info_;

        bool is_successful_preservation_;
    };
}

#endif
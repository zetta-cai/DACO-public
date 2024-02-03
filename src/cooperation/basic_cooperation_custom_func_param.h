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

    // ValidateDirectoryTableForPreservedDirinfoFuncParam

    class ValidateDirectoryTableForPreservedDirinfoFuncParam : public CooperationCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // validate dirinfo in directory table if exists

        ValidateDirectoryTableForPreservedDirinfoFuncParam(const Key& key, const uint32_t& source_edge_idx, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached);
        virtual ~ValidateDirectoryTableForPreservedDirinfoFuncParam();

        Key getKey() const;
        uint32_t getSourceEdgeIdx() const;
        DirectoryInfo getDirectoryInfo() const;
        bool& isBeingWrittenRef() const;
        bool& isNeighborCachedRef() const;

        bool& isSuccessfulValidationRef();
        const bool& isSuccessfulValidationConstRef() const;
    private:
        static const std::string kClassName;

        const Key key_;
        const uint32_t source_edge_idx_;
        const DirectoryInfo directory_info_;
        bool& is_being_written_ref_;
        bool& is_neighbor_cached_ref_;

        bool is_successful_validation_;
    };
}

#endif
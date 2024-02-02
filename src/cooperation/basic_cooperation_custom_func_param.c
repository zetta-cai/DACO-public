#include "cooperation/basic_cooperation_custom_func_param.h"

namespace covered
{
    // PreserveDirectoryTableIfGlobalUncachedFuncParam

    const std::string PreserveDirectoryTableIfGlobalUncachedFuncParam::kClassName("PreserveDirectoryTableIfGlobalUncachedFuncParam");

    const std::string PreserveDirectoryTableIfGlobalUncachedFuncParam::FUNCNAME("preserve_directory_table_if_global_uncached");

    PreserveDirectoryTableIfGlobalUncachedFuncParam::PreserveDirectoryTableIfGlobalUncachedFuncParam(const Key& key, const DirectoryInfo& directory_info) : CooperationCustomFuncParamBase(), key_(key), directory_info_(directory_info)
    {
        is_successful_preservation_ = false;
    }

    PreserveDirectoryTableIfGlobalUncachedFuncParam::~PreserveDirectoryTableIfGlobalUncachedFuncParam()
    {
    }

    Key PreserveDirectoryTableIfGlobalUncachedFuncParam::getKey() const
    {
        return key_;
    }

    DirectoryInfo PreserveDirectoryTableIfGlobalUncachedFuncParam::getDirectoryInfo() const
    {
        return directory_info_;
    }

    bool& PreserveDirectoryTableIfGlobalUncachedFuncParam::isSuccessfulPreservationRef()
    {
        return is_successful_preservation_;
    }

    const bool& PreserveDirectoryTableIfGlobalUncachedFuncParam::isSuccessfulPreservationConstRef() const
    {
        return is_successful_preservation_;
    }

    // ValidateDirectoryTableForPreservedDirinfoFuncParam

    const std::string ValidateDirectoryTableForPreservedDirinfoFuncParam::kClassName("ValidateDirectoryTableForPreservedDirinfoFuncParam");

    const std::string ValidateDirectoryTableForPreservedDirinfoFuncParam::FUNCNAME("validate_directory_table_for_preserved_dirinfo");

    ValidateDirectoryTableForPreservedDirinfoFuncParam::ValidateDirectoryTableForPreservedDirinfoFuncParam(const Key& key, const DirectoryInfo& directory_info) : CooperationCustomFuncParamBase(), key_(key), directory_info_(directory_info)
    {
        is_successful_validation_ = false;
    }

    ValidateDirectoryTableForPreservedDirinfoFuncParam::~ValidateDirectoryTableForPreservedDirinfoFuncParam()
    {
    }

    Key ValidateDirectoryTableForPreservedDirinfoFuncParam::getKey() const
    {
        return key_;
    }

    DirectoryInfo ValidateDirectoryTableForPreservedDirinfoFuncParam::getDirectoryInfo() const
    {
        return directory_info_;
    }

    bool& ValidateDirectoryTableForPreservedDirinfoFuncParam::isSuccessfulValidationRef()
    {
        return is_successful_validation_;
    }

    const bool& ValidateDirectoryTableForPreservedDirinfoFuncParam::isSuccessfulValidationConstRef() const
    {
        return is_successful_validation_;
    }
}
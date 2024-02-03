#include "cooperation/basic_cooperation_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/dynamic_array.h"
#include "common/util.h"
#include "cooperation/basic_cooperation_custom_func_param.h"
#include "network/propagation_simulator.h"

namespace covered
{
    const std::string BasicCooperationWrapper::kClassName("BasicCooperationWrapper");

    BasicCooperationWrapper::BasicCooperationWrapper(const uint32_t& edgecnt, const uint32_t& edge_idx, const std::string& hash_name) : CooperationWrapperBase(edgecnt, edge_idx, hash_name)
    {
        // Differentiate CooperationWrapper in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    BasicCooperationWrapper::~BasicCooperationWrapper() {}

    // (0) Cache-method-specific custom functions

    void BasicCooperationWrapper::customFunc(const std::string& funcname, CooperationCustomFuncParamBase* func_param_ptr)
    {
        if (funcname == PreserveDirectoryTableIfGlobalUncachedFuncParam::FUNCNAME)
        {
            PreserveDirectoryTableIfGlobalUncachedFuncParam* tmp_param_ptr = dynamic_cast<PreserveDirectoryTableIfGlobalUncachedFuncParam*>(func_param_ptr);
            assert(tmp_param_ptr != NULL);

            bool& is_successful_preservation_ref = tmp_param_ptr->isSuccessfulPreservationRef();
            is_successful_preservation_ref = preserveDirectoryTableIfGlobalUncachedInternal_(tmp_param_ptr->getKey(), tmp_param_ptr->getDirectoryInfo());
        }
        else if (funcname == ValidateDirectoryTableForPreservedDirinfoFuncParam::FUNCNAME)
        {
            ValidateDirectoryTableForPreservedDirinfoFuncParam* tmp_param_ptr = dynamic_cast<ValidateDirectoryTableForPreservedDirinfoFuncParam*>(func_param_ptr);
            assert(tmp_param_ptr != NULL);

            bool& is_successful_validation_ref = tmp_param_ptr->isSuccessfulValidationRef();
            is_successful_validation_ref = validateDirectoryTableForPreservedDirinfoInternal_(tmp_param_ptr->getKey(), tmp_param_ptr->getSourceEdgeIdx(), tmp_param_ptr->getDirectoryInfo(), tmp_param_ptr->isBeingWrittenRef(), tmp_param_ptr->isNeighborCachedRef());
        }
        else
        {
            std::ostringstream oss;
            oss << "Unknown custom function name: " << funcname;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void BasicCooperationWrapper::constCustomFunc(const std::string& funcname, CooperationCustomFuncParamBase* func_param_ptr) const
    {
        std::ostringstream oss;
        oss << "Unknown const custom function name: " << funcname;
        Util::dumpErrorMsg(instance_name_, oss.str());
        exit(1);

        return;
    }

    bool BasicCooperationWrapper::preserveDirectoryTableIfGlobalUncachedInternal_(const Key& key, const DirectoryInfo& directory_info)
    {
        checkPointers_();

        // NOTE: sender may preserve a dirinfo for another placement edge node (e.g., see BestGuess)!!!
        //assert(source_edge_idx == directory_info.getTargetEdgeIdx());

        // Acquire a write lock
        std::string context_name = "BasicCooperationWrapper::preserveDirectoryTableIfGlobalUncachedInternal_()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        MYASSERT(dht_wrapper_ptr_->getBeaconEdgeIdx(key) == edge_idx_); // Current edge node MUST be beacon for the given key

        bool is_successful_preservation = false;
        if (!directory_table_ptr_->isGlobalCached(key)) // NOTE: ONLY preserve dirinfo for global uncached object
        {
            DirectoryMetadata directory_metadata(false); // Must be invalid dirinfo for preservation
            MetadataUpdateRequirement unused_metadata_update_requirement;
            bool is_global_cached = directory_table_ptr_->update(key, true, directory_info, directory_metadata, unused_metadata_update_requirement);
            UNUSED(unused_metadata_update_requirement);

            assert(is_global_cached);
            is_successful_preservation = true;
        }

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return is_successful_preservation;
    }

    bool BasicCooperationWrapper::validateDirectoryTableForPreservedDirinfoInternal_(const Key& key, const uint32_t& source_edge_idx, const DirectoryInfo& directory_info, bool& is_being_written, bool& is_neighbor_cached)
    {
        checkPointers_();

        // NOTE: sender MUST be placement edge node during validation of BestGuess
        assert(source_edge_idx == directory_info.getTargetEdgeIdx());

        // Acquire a write lock
        std::string context_name = "BasicCooperationWrapper::validateDirectoryTableForPreservedDirinfoInternal_()";
        cooperation_wrapper_perkey_rwlock_ptr_->acquire_lock(key, context_name);

        MYASSERT(dht_wrapper_ptr_->getBeaconEdgeIdx(key) == edge_idx_); // Current edge node MUST be beacon for the given key

        // Directory validation MUST be admission for BestGuess
        is_being_written = block_tracker_ptr_->isBeingWrittenForKey(key);

        bool is_successful_validation = directory_table_ptr_->validateDirinfoForKeyIfExist(key, directory_info);

        // Return if key is cached by any other edge node except the source edge node
        is_neighbor_cached = directory_table_ptr_->isNeighborCached(key, source_edge_idx);

        // Release a write lock
        cooperation_wrapper_perkey_rwlock_ptr_->unlock(key, context_name);

        return is_successful_validation;
    }
}
#include "cache/validity_map.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    // ValidityFlag

    const std::string ValidityFlag::IS_VALID_FLAG_FUNCNAME("isValidFlag");

    const std::string ValidityFlag::kClassName("ValidityFlag");

    ValidityFlag::ValidityFlag()
    {
        is_valid_ = false;
    }

    ValidityFlag::ValidityFlag(const bool& is_valid)
    {
        is_valid_ = is_valid;
    }

    ValidityFlag::~ValidityFlag() {}

    // (1) Access valid flag

    bool ValidityFlag::isValidFlag() const
    {
        return is_valid_;
    }

    // (2) For ConcurrentHashtable

    uint64_t ValidityFlag::getSizeForCapacity() const
    {
        return sizeof(bool);
    }

    bool ValidityFlag::call(const std::string& function_name, void* param_ptr)
    {
        bool is_erase = false;

        Util::dumpErrorMsg(kClassName, "NOT need call() for ConcurrentHashtable::insertOrCall()/callIfExist()!");
        exit(1);

        return is_erase;
    }

    void ValidityFlag::constCall(const std::string& function_name, void* param_ptr) const
    {
        assert(param_ptr != NULL);

        if (function_name == IS_VALID_FLAG_FUNCNAME)
        {
            IsValidFlagParam* tmp_param_ptr = static_cast<IsValidFlagParam*>(param_ptr);
            tmp_param_ptr->is_valid = isValidFlag();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for constCall()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return;
    }

    const ValidityFlag& ValidityFlag::operator=(const ValidityFlag& other)
    {
        is_valid_ = other.is_valid_;
        return *this;
    }

    // ValidityMap

    const std::string ValidityMap::kClassName("ValidityMap");

    ValidityMap::ValidityMap(const uint32_t& edge_idx, const PerkeyRwlock* perkey_rwlock_ptr) : perkey_validity_("perkey_validity_", ValidityFlag(), perkey_rwlock_ptr)
    {
        // Differentiate local edge cache in different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();
    }

    ValidityMap::~ValidityMap() {}

    bool ValidityMap::isKeyExist(const Key& key) const
    {
        bool is_key_exist = perkey_validity_.isExist(key);
        
        return is_key_exist;
    }

    bool ValidityMap::isValidFlagForKey(const Key& key, bool& is_exist) const
    {
        // Prepare IsValidFlagParam
        ValidityFlag::IsValidFlagParam tmp_param = {false};

        perkey_validity_.constCallIfExist(key, is_exist, ValidityFlag::IS_VALID_FLAG_FUNCNAME, &tmp_param);
        return tmp_param.is_valid;
    }

    void ValidityMap::invalidateFlagForKey(const Key& key, bool& is_exist)
    {
        ValidityFlag validity_flag(false);
        perkey_validity_.insertOrUpdate(key, validity_flag, is_exist);
        return;
    }

    void ValidityMap::validateFlagForKey(const Key& key, bool& is_exist)
    {
        ValidityFlag validity_flag(true);
        perkey_validity_.insertOrUpdate(key, validity_flag, is_exist);
        return;
    }

    void ValidityMap::eraseFlagForKey(const Key& key, bool& is_exist)
    {
        perkey_validity_.eraseIfExist(key, is_exist);
        return;
    }

    uint64_t ValidityMap::getSizeForCapacity() const
    {
        // NOTE: we do NOT count key size of validity_map_, as the size of keys cached by closest edge node has been counted by the corresponding local edge cache (e.g., LruCacheWrapper::getInternal_())
        uint64_t size = perkey_validity_.getTotalValueSizeForCapcity();
        return size;
    }

    void ValidityMap::getAllKeyValidityPairs(std::unordered_map<Key, ValidityFlag, KeyHasher>& key_validity_map) const
    {
        return perkey_validity_.getAllKeyValuePairs(key_validity_map);
    }

    void ValidityMap::putKeyValidityPair(const Key& tmp_key, const ValidityFlag& tmp_validity_flag, bool& is_exist)
    {
        is_exist = false;
        perkey_validity_.putKeyValuePair(tmp_key, tmp_validity_flag, is_exist);
        return;
    }
}
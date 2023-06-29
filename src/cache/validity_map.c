#include "cache/validity_map.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    // ValidityFlag

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

    uint32_t ValidityFlag::getSizeForCapacity() const
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

        if (function_name == "isValidFlag")
        {
            IsValidFlagParam* tmp_param_ptr = static_cast<IsValidFlagParam*>(param_ptr);
            tmp_param_ptr->is_valid = isValidFlag();
        }
        else
        {
            std::ostringstream oss;
            oss << "invalid function name " << function_name << " for constCall()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);s
        }

        return;
    }

    ValidityFlag& ValidityFlag::operator=(const ValidityFlag& other)
    {
        is_valid_ = other.is_valid_;
        return *this;
    }

    // ValidityMap

    const std::string ValidityMap::kClassName("ValidityMap");

    ValidityMap::ValidityMap(EdgeParam* edge_param_ptr) : perkey_validity_("perkey_validity_", ValidityFlag())
    {
        // Differentiate local edge cache in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
    }

    ValidityMap::~ValidityMap() {}

    bool ValidityMap::isValidFlagForKey(const Key& key, bool& is_exist) const
    {
        // Prepare IsValidFlagForParam
        ValidityFlag::IsValidFlagParam tmp_param = {false};

        perkey_validity_.constCallIfExist(key, is_exist, "isValidFlag", &tmp_param);
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

    uint32_t ValidityMap::getSizeForCapacity() const
    {
        // NOTE: we do NOT count key size of validity_map_, as the size of keys cached by closest edge node has been counted by the corresponding local edge cache (e.g., LruCacheWrapper::getInternal_())
        uint32_t size = perkey_validity_.getTotalValueSizeForCapcity();
        return size;
    }
}
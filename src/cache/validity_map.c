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

    bool ValidityFlag::isValid() const
    {
        return is_valid_;
    }

    uint32_t ValidityFlag::getSizeForCapacity() const
    {
        return sizeof(bool);
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

    bool ValidityMap::isValidObject(const Key& key, bool& is_found) const
    {
        ValidityFlag validity_flag = perkey_validity_.get(key, is_found);
        bool is_valid = validity_flag.isValid();
        return is_valid;
    }

    void ValidityMap::invalidateObject(const Key& key, bool& is_found)
    {
        ValidityFlag validity_flag(false);
        perkey_validity_.update(key, validity_flag, is_found);
        return;
    }

    void ValidityMap::validateObject(const Key& key, bool& is_found)
    {
        ValidityFlag validity_flag(true);
        perkey_validity_.update(key, validity_flag, is_found);
        return;
    }

    void ValidityMap::erase(const Key& key, bool& is_found)
    {
        perkey_validity_.erase(key, is_found);
        return;
    }

    uint32_t ValidityMap::getSizeForCapacity() const
    {
        // NOTE: we do NOT count key size of validity_map_, as the size of keys cached by closest edge node has been counted by the corresponding local edge cache (e.g., LruCacheWrapper::getInternal_())
        uint32_t size = perkey_validity_.getTotalValueSizeForCapcity();
        return size;
    }
}
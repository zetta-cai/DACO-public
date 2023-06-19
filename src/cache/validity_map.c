#include "cache/validity_map.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string ValidityMap::kClassName("ValidityMap");

    ValidityMap::ValidityMap(EdgeParam* edge_param_ptr)
    {
        // Differentiate local edge cache in different edge nodes
        assert(edge_param_ptr != NULL);
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_validity_ptr_";
        rwlock_for_validity_ptr_ = new Rwlock(oss.str());
        assert(rwlock_for_validity_ptr_ != NULL);

        perkey_validity_.clear();
    }

    ValidityMap::~ValidityMap()
    {
        assert(rwlock_for_validity_ptr_ != NULL);
        delete rwlock_for_validity_ptr_;
        rwlock_for_validity_ptr_ = NULL;
    }

    bool ValidityMap::isValid(const Key& key, bool& is_found) const
    {
        // Acquire a read lock for perkey_validity_ before accessing any shared variable
        assert(rwlock_for_validity_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_validity_ptr_->try_lock_shared("isValid()"))
            {
                break;
            }
        }

        perkey_validity_t::const_iterator iter = perkey_validity_.find(key);
        bool is_valid = false;
        if (iter != perkey_validity_.end())
        {
            is_valid = iter->second;
            is_found = true;
        }
        else
        {
            is_valid = false;
            is_found = false;
        }

        rwlock_for_validity_ptr_->unlock_shared();
        return is_valid;
    }

    void ValidityMap::invalidate(const Key& key, bool& is_found)
    {
        // Acquire a write lock for validity_map_ before accessing any shared variable
        assert(rwlock_for_validity_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_validity_ptr_->try_lock("invalidate()"))
            {
                break;
            }
        }

        if (perkey_validity_.find(key) != perkey_validity_.end())
        {
            perkey_validity_[key] = false;
            is_found = true;
        }
        else
        {
            perkey_validity_.insert(std::pair<Key, bool>(key, false));
            is_found = false;
        }

        rwlock_for_validity_ptr_->unlock();
        return;
    }

    void ValidityMap::validate(const Key& key, bool& is_found)
    {
        // Acquire a write lock for validity_map_ before accessing any shared variable
        assert(rwlock_for_validity_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_validity_ptr_->try_lock("validate()"))
            {
                break;
            }
        }

        if (perkey_validity_.find(key) != perkey_validity_.end())
        {
            perkey_validity_[key] = true;
            is_found = true;
        }
        else
        {
            perkey_validity_.insert(std::pair<Key, bool>(key, true));
            is_found = false;
        }

        rwlock_for_validity_ptr_->unlock();
        return;
    }

    void ValidityMap::erase(const Key& key, bool& is_found)
    {
        // Acquire a write lock for validity_map_ before accessing any shared variable
        assert(rwlock_for_validity_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_validity_ptr_->try_lock("invalidate()"))
            {
                break;
            }
        }

        if (perkey_validity_.find(key) != perkey_validity_.end())
        {
            perkey_validity_.erase(key);
            is_found = true;
        }
        else
        {
            is_found = false;
        }

        rwlock_for_validity_ptr_->unlock();
        return;
    }

    uint32_t ValidityMap::getSizeForCapacity() const
    {
        // Acquire a read lock for validity_map_ before accessing any shared variable
        assert(rwlock_for_validity_ptr_ != NULL);
        while (true)
        {
            if (rwlock_for_validity_ptr_->try_lock_shared("getSizeForCapacity()"))
            {
                break;
            }
        }

        // NOTE: we do NOT count key size of validity_map_, as the size of keys cached by closest edge node has been counted by the corresponding local edge cache (e.g., LruCacheWrapper::getInternal_())
        uint32_t size = perkey_validity_.size() * sizeof(bool);

        rwlock_for_validity_ptr_->unlock_shared();
        return size;
    }
}
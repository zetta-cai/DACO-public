#include "common/perkey_rwlock.h"

#include <assert.h>

#include "common/param.h"

namespace covered
{
    const std::string PerkeyRwlock::kClassName("PerkeyRwlock");
    const uint32_t PerkeyRwlock::RWLOCK_HASHTABLE_CAPCITY = 1000;
    
    PerkeyRwlock::PerkeyRwlock()
    {
        rwlock_hashtable_.resize(RWLOCK_HASHTABLE_CAPCITY, NULL);
        for (uint32_t i = 0; i < rwlock_hashtable_.size(); i++)
        {
            rwlock_hashtable_[i] = new boost::shared_mutex;
            assert(rwlock_hashtable_[i] != NULL);
        }

        // NOTE: hashing for rwlock index is orthogonal with hash partition for DHT-based content discovery
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapper(Param::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);
    }

    PerkeyRwlock::~PerkeyRwlock()
    {
        for (uint32_t i = 0; i < rwlock_hashtable_.size(); i++)
        {
            assert(rwlock_hashtable_[i] != NULL);
            delete rwlock_hashtable_[i];
            rwlock_hashtable_[i] = NULL;
        }

        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    bool PerkeyRwlock::try_lock_shared(const Key& key)
    {
        uint32_t rwlock_index = getRwlockIndex_(key);
        assert(rwlock_hashtable_[rwlock_index] != NULL);
        return rwlock_hashtable_[rwlock_index]->try_lock_shared();
    }

    void PerkeyRwlock::unlock_shared(const Key& key)
    {
        uint32_t rwlock_index = getRwlockIndex_(key);
        assert(rwlock_hashtable_[rwlock_index] != NULL);
        rwlock_hashtable_[rwlock_index]->unlock_shared();
        return;
    }

    bool PerkeyRwlock::try_lock(const Key& key)
    {
        uint32_t rwlock_index = getRwlockIndex_(key);
        assert(rwlock_hashtable_[rwlock_index] != NULL);
        return rwlock_hashtable_[rwlock_index]->try_lock();
    }

    void PerkeyRwlock::unlock(const Key& key)
    {
        uint32_t rwlock_index = getRwlockIndex_(key);
        assert(rwlock_hashtable_[rwlock_index] != NULL);
        rwlock_hashtable_[rwlock_index]->unlock();
        return;
    }

    uint32_t PerkeyRwlock::getRwlockIndex_(const Key& key)
    {
        // Hash the key
        assert(hash_wrapper_ptr_ != NULL);
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t rwlock_index = hash_value % rwlock_hashtable_.size();
        return rwlock_index;
    }
}
#ifndef CONCURRENT_HASHTABLE_IMPL_H
#define CONCURRENT_HASHTABLE_IMPL_H

#include "concurrency/concurrent_hashtable.h"

#include <sstream>

#include "common/kv_list_helper_impl.h"
#include "common/util.h"

namespace covered
{
    template<class V>
    const std::string ConcurrentHashtable<V>::kClassName("ConcurrentHashtable");

    template<class V>
    ConcurrentHashtable<V>::ConcurrentHashtable(const std::string& table_name, const V& default_value, const PerkeyRwlock* perkey_rwlock_ptr) : perkey_rwlock_ptr_(perkey_rwlock_ptr), total_key_size_(0), total_value_size_(0)
    {
        std::ostringstream oss;
        oss << kClassName << " of " << table_name;
        instance_name_ = oss.str();

        default_value_ = default_value;

        assert(perkey_rwlock_ptr_ != NULL);

        uint32_t bucket_count = perkey_rwlock_ptr_->getFineGrainedLockingSize();
        hashtables_.resize(bucket_count);
        for (uint32_t bucket_idx = 0; bucket_idx < bucket_count; bucket_idx++)
        {
            hashtables_[bucket_idx].clear();
        }
    }

    template<class V>
    ConcurrentHashtable<V>::~ConcurrentHashtable()
    {
        // NOTE: no need to release perkey_rwlock_ptr_, which is maintained outside ConcurrentHashtable (e.g., in CacheWrapperBase and CooperationWrapperBase)
    }

    template<class V>
    bool ConcurrentHashtable<V>::isExist(const Key& key) const
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a read/write/write lock
        assert(perkey_rwlock_ptr_->isReadOrWriteLocked(key));
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        bool is_exist = false;

        const std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::const_iterator iter = tmp_hashtable.find(key);
        is_exist = (iter != tmp_hashtable.end());

        // Must be protected by a read/write lock
        assert(perkey_rwlock_ptr_->isReadOrWriteLocked(key));

        return is_exist;
    }

    /*template<class V>
    V ConcurrentHashtable<V>::getIfExist(const Key& key, bool& is_exist) const
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a read/write lock
        assert(perkey_rwlock_ptr_->isReadOrWriteLocked(key));
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        V value;
        const std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::const_iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            value = iter->second;
            is_exist = true;
        }
        else
        {
            value = default_value_;
            is_exist = false;
        }

        // Must be protected by a read/write lock
        assert(perkey_rwlock_ptr_->isReadOrWriteLocked(key));

        return value;
    }*/

    template<class V>
    void ConcurrentHashtable<V>::insertOrUpdate(const Key& key, const V& value, bool& is_exist)
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a write lock
        if (!perkey_rwlock_ptr_->isWriteLocked(key))
        {
            std::ostringstream oss;
            oss << "perkey_rwlock_ptr_ is NOT write locked for key " << key.getKeystr();
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, value));
            is_exist = false;

            Util::uint64AddForAtomic(total_key_size_, static_cast<uint64_t>(key.getKeyLength()));
            Util::uint64AddForAtomic(total_value_size_, static_cast<uint64_t>(value.getSizeForCapacity()));
        }
        else // key exists
        {
            uint64_t original_value_size = iter->second.getSizeForCapacity();

            // Update value
            iter->second = value;
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);
        }

        // Must be protected by a write lock
        assert(perkey_rwlock_ptr_->isWriteLocked(key));

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::insertOrCall(const Key& key, const V& value, bool& is_exist, const std::string& function_name, void* param_ptr)
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a write lock
        if (!perkey_rwlock_ptr_->isWriteLocked(key))
        {
            std::ostringstream oss;
            oss << "perkey_rwlock_ptr_ is NOT write locked for key " << key.getKeystr();
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, value));
            is_exist = false;

            Util::uint64AddForAtomic(total_key_size_, static_cast<uint64_t>(key.getKeyLength()));
            Util::uint64AddForAtomic(total_value_size_, static_cast<uint64_t>(value.getSizeForCapacity()));
        }
        else // key exists
        {
            uint64_t original_value_size = iter->second.getSizeForCapacity();

            // Call value's function
            bool is_erase = iter->second.call(function_name, param_ptr);
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);

            assert(is_erase == false); // NOT erase key-value pair in insertOrCall()
        }


        // Must be protected by a write lock
        assert(perkey_rwlock_ptr_->isWriteLocked(key));

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::callIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr)
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a write lock
        if (!perkey_rwlock_ptr_->isWriteLocked(key))
        {
            std::ostringstream oss;
            oss << "perkey_rwlock_ptr_ is NOT write locked for key " << key.getKeystr();
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::iterator iter = tmp_hashtable.find(key);

        if (iter != tmp_hashtable.end()) // key exists
        {
            uint64_t original_value_size = iter->second.getSizeForCapacity();

            // Call value's function
            bool is_erase = iter->second.call(function_name, param_ptr);
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);
            original_value_size = iter->second.getSizeForCapacity();

            if (is_erase) // Erase if necessary
            {
                tmp_hashtable.erase(iter);

                Util::uint64MinusForAtomic(total_key_size_, static_cast<uint64_t>(key.getKeyLength()));
                Util::uint64MinusForAtomic(total_value_size_, original_value_size);
            }
        }
        else // key NOT exist
        {
            is_exist = false;
        }

        // Must be protected by a write lock
        assert(perkey_rwlock_ptr_->isWriteLocked(key));

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::constCallIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr) const
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a read/write lock
        MYASSERT(perkey_rwlock_ptr_->isReadOrWriteLocked(key));
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        const std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::const_iterator iter = tmp_hashtable.find(key);

        if (iter != tmp_hashtable.end()) // key exists
        {
            // Call value's function
            iter->second.constCall(function_name, param_ptr);
            is_exist = true;
        }
        else // key NOT exist
        {
            is_exist = false;
        }

        // Must be protected by a read/write lock
        MYASSERT(perkey_rwlock_ptr_->isReadOrWriteLocked(key));

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::eraseIfExist(const Key& key, bool& is_exist)
    {
        assert(perkey_rwlock_ptr_ != NULL);

        // Must be protected by a write lock
        if (!perkey_rwlock_ptr_->isWriteLocked(key))
        {
            std::ostringstream oss;
            oss << "perkey_rwlock_ptr_ is NOT write locked for key " << key.getKeystr();
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        uint32_t hashidx = perkey_rwlock_ptr_->getRwlockIndex(key);

        std::unordered_map<Key, V, KeyHasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, KeyHasher>::iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            uint64_t original_value_size = iter->second.getSizeForCapacity();

            tmp_hashtable.erase(iter);
            is_exist = true;

            Util::uint64MinusForAtomic(total_key_size_, static_cast<uint64_t>(key.getKeyLength()));
            Util::uint64MinusForAtomic(total_value_size_, original_value_size);
        }
        else // key NOT exist
        {
            is_exist = false;
        }

        // Must be protected by a write lock
        assert(perkey_rwlock_ptr_->isWriteLocked(key));

        return;
    }

    template<class V>
    uint64_t ConcurrentHashtable<V>::getTotalKeySizeForCapcity() const
    {
        return total_key_size_.load(Util::LOAD_CONCURRENCY_ORDER);
    }
    
    template<class V>
    uint64_t ConcurrentHashtable<V>::getTotalValueSizeForCapcity() const
    {
        return total_value_size_;
    }

    template<class V>
    void ConcurrentHashtable<V>::updateTotalValueSize_(uint64_t current_value_size, uint64_t original_value_size)
    {
        if (current_value_size >= original_value_size)
        {
            Util::uint64AddForAtomic(total_value_size_, current_value_size - original_value_size);
        }
        else
        {
            Util::uint64MinusForAtomic(total_value_size_, original_value_size - current_value_size);
        }
        return;
    }
}

#endif
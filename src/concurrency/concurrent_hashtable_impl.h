#ifndef CONCURRENT_HASHTABLE_IMPL_H
#define CONCURRENT_HASHTABLE_IMPL_H

#include "concurrency/concurrent_hashtable.h"

#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    template<class V>
    const uint32_t ConcurrentHashtable<V>::CONCURRENT_HASHTABLE_BUCKET_COUNT = 1000;

    template<class V>
    const std::string ConcurrentHashtable<V>::kClassName("ConcurrentHashtable");

    template<class V>
    ConcurrentHashtable<V>::ConcurrentHashtable(const std::string& table_name, const V& default_value, const uint32_t& bucket_count)
    {
        std::ostringstream oss;
        oss << kClassName << " of " << table_name;
        instance_name_ = oss.str();

        default_value_ = default_value;

        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapper(Param::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);
        
        rwlocks_ = new boost::shared_mutex[bucket_count];
        assert(rwlocks_ != NULL);

        hashtables_.resize(bucket_count);
        for (uint32_t bucket_idx = 0; bucket_idx < bucket_count; bucket_idx++)
        {
            hashtables_[bucket_idx].clear();
        }
    }

    template<class V>
    ConcurrentHashtable<V>::~ConcurrentHashtable()
    {
        assert(rwlocks_ != NULL);
        delete[] rwlocks_;
        rwlocks_ = NULL;

        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    template<class V>
    bool ConcurrentHashtable<V>::isExist(const Key& key) const
    {
        assert(rwlocks_ != NULL);

        // Acquire a read lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock_shared())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a read lock for key " << key.getKeystr() << " in isExist()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        bool is_exist = false;

        const std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::const_iterator iter = tmp_hashtable.find(key);
        is_exist = (iter != tmp_hashtable.end());

        // Release the read lock
        rwlocks_[hashidx].unlock_shared();

        return is_exist;
    }

    /*template<class V>
    V ConcurrentHashtable<V>::getIfExist(const Key& key, bool& is_exist) const
    {
        assert(rwlocks_ != NULL);

        // Acquire a read lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock_shared())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a read lock for key " << key.getKeystr() << " in getIfExist()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        V value;
        const std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::const_iterator iter = tmp_hashtable.find(key);
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

        // Release the read lock
        rwlocks_[hashidx].unlock_shared();

        return value;
    }*/

    template<class V>
    void ConcurrentHashtable<V>::insertOrUpdate(const Key& key, const V& value, bool& is_exist)
    {
        assert(rwlocks_ != NULL);

        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a write lock for key " << key.getKeystr() << " in insertOrUpdate()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, value));
            is_exist = false;

            total_key_size_.fetch_add(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
            total_value_size_.fetch_add(value.getSizeForCapacity(), Util::RMW_CONCURRENCY_ORDER);
        }
        else // key exists
        {
            uint32_t original_value_size = iter->second.getSizeForCapacity();

            // Update value
            iter->second = value;
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::insertOrCall(const Key& key, const V& value, bool& is_exist, const std::string& function_name, void* param_ptr)
    {
        assert(rwlocks_ != NULL);

        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a write lock for key " << key.getKeystr() << " in insertOrCall()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, value));
            is_exist = false;

            total_key_size_.fetch_add(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
            total_value_size_.fetch_add(value.getSizeForCapacity(), Util::RMW_CONCURRENCY_ORDER);
        }
        else // key exists
        {
            uint32_t original_value_size = iter->second.getSizeForCapacity();

            // Call value's function
            bool is_erase = iter->second.call(function_name, param_ptr);
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);

            assert(is_erase == false); // NOT erase key-value pair in insertOrCall()
        }


        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::callIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr)
    {
        assert(rwlocks_ != NULL);

        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a write lock for key " << key.getKeystr() << " in callIfExist()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);

        if (iter != tmp_hashtable.end()) // key exists
        {
            uint32_t original_value_size = iter->second.getSizeForCapacity();

            // Call value's function
            bool is_erase = iter->second.call(function_name, param_ptr);
            is_exist = true;

            updateTotalValueSize_(iter->second.getSizeForCapacity(), original_value_size);
            original_value_size = iter->second.getSizeForCapacity();

            if (is_erase) // Erase if necessary
            {
                tmp_hashtable.erase(iter);

                total_key_size_.fetch_sub(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
                total_value_size_.fetch_sub(original_value_size, Util::RMW_CONCURRENCY_ORDER);
            }
        }
        else // key NOT exist
        {
            is_exist = false;
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::constCallIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr) const
    {
        assert(rwlocks_ != NULL);

        // Acquire a read lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_shared_lock())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a read lock for key " << key.getKeystr() << " in constCallIfExist()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::const_iterator iter = tmp_hashtable.find(key);

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

        // Release the read lock
        rwlocks_[hashidx].unlock_shared();

        return;
    }

    template<class V>
    void ConcurrentHashtable<V>::eraseIfExist(const Key& key, bool& is_exist)
    {
        assert(rwlocks_ != NULL);

        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                // TMPDEBUG
                std::ostringstream oss;
                oss << "acquire a write lock for key " << key.getKeystr() << " in eraseIfExist()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            uint32_t original_value_size = iter->second.getSizeForCapacity();

            tmp_hashtable.erase(iter);
            is_exist = true;

            total_key_size_.fetch_sub(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
            total_value_size_.fetch_sub(original_value_size, Util::RMW_CONCURRENCY_ORDER);
        }
        else // key NOT exist
        {
            is_exist = false;
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V>
    uint32_t ConcurrentHashtable<V>::getTotalKeySizeForCapcity() const
    {
        return total_key_size_.load(Util::LOAD_CONCURRENCY_ORDER);
    }
    
    template<class V>
    uint32_t ConcurrentHashtable<V>::getTotalValueSizeForCapcity() const
    {
        return total_value_size_;
    }

    template<class V>
    uint32_t ConcurrentHashtable<V>::getHashIndex_(const Key& key) const
    {
        // No need to acquire any lock, which has been done in get(), insert(), and erase()

        assert(hash_wrapper_ptr_ != NULL);
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t hashidx = hash_value % hashtables_.size();
        return hashidx;
    }

    template<class V>
    void ConcurrentHashtable<V>::updateTotalValueSize_(uint32_t current_value_size, uint32_t original_value_size)
    {
        if (current_value_size >= original_value_size)
        {
            total_value_size_.fetch_add(current_value_size - original_value_size, Util::RMW_CONCURRENCY_ORDER);
        }
        else
        {
            total_value_size_.fetch_sub(original_value_size - current_value_size, Util::RMW_CONCURRENCY_ORDER);
        }
        return;
    }
}

#endif
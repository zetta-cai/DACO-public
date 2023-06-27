#ifndef CONCURRENT_HASHTABLE_IMPL_H
#define CONCURRENT_HASHTABLE_IMPL_H

#include "concurrency/concurrent_hashtable.h"

#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    template<class V, class Hasher>
    const uint32_t ConcurrentHashtable<V, Hasher>::CONCURRENT_HASHTABLE_BUCKET_COUNT = 1000;

    template<class V, class Hasher>
    const std::string ConcurrentHashtable<V, Hasher>::kClassName("ConcurrentHashtable");

    template<class V, class Hasher>
    ConcurrentHashtable<V, Hasher>::ConcurrentHashtable(const std::string& table_name, const V& default_value, const uint32_t& bucket_count)
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

    template<class V, class Hasher>
    ConcurrentHashtable<V, Hasher>::~ConcurrentHashtable()
    {
        assert(rwlocks_ != NULL);
        delete[] rwlocks_;
        rwlocks_ = NULL;

        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    template<class V, class Hasher>
    bool ConcurrentHashtable<V, Hasher>::exists(const Key& key) const
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
                oss << "acquire a read lock for key " << key.getKeystr() << " in exists()";
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

    template<class V, class Hasher>
    V ConcurrentHashtable<V, Hasher>::get(const Key& key, bool& is_found) const
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
                oss << "acquire a read lock for key " << key.getKeystr() << " in get()";
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
            is_found = true;
        }
        else
        {
            value = default_value_;
            is_found = false;
        }

        // Release the read lock
        rwlocks_[hashidx].unlock_shared();

        return value;
    }

    template<class V, class Hasher>
    void ConcurrentHashtable<V, Hasher>::update(const Key& key, const V& value, bool& is_found)
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
                oss << "acquire a write lock for key " << key.getKeystr() << " in update()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, value));
            is_found = false;

            total_key_size_.fetch_add(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
            total_value_size_.fetch_add(value.getSizeForCapacity(), Util::RMW_CONCURRENCY_ORDER);
        }
        else // key exists
        {
            V original_value = iter->second;
            iter->second = value;
            is_found = true;

            total_value_size_ += (value.getSizeForCapacity() - original_value.getSizeForCapacity());
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V, class Hasher>
    void ConcurrentHashtable<V, Hasher>::erase(const Key& key, bool& is_found)
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
                oss << "acquire a write lock for key " << key.getKeystr() << " in erase()";
                Util::dumpDebugMsg(instance_name_, oss.str());

                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        typename std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            V original_value = iter->second;
            tmp_hashtable.erase(iter);
            is_found = true;

            total_key_size_.fetch_sub(key.getKeystr().length(), Util::RMW_CONCURRENCY_ORDER);
            total_value_size_.fetch_sub(original_value.getSizeForCapacity(), Util::RMW_CONCURRENCY_ORDER);
        }
        else // key NOT exist
        {
            is_found = false;
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V, class Hasher>
    uint32_t ConcurrentHashtable<V, Hasher>::getTotalKeySizeForCapcity() const
    {
        return total_key_size_.load(Util::LOAD_CONCURRENCY_ORDER);
    }
    
    template<class V, class Hasher>
    uint32_t ConcurrentHashtable<V, Hasher>::getTotalValueSizeForCapcity() const
    {
        return total_value_size_;
    }

    template<class V, class Hasher>
    uint32_t ConcurrentHashtable<V, Hasher>::getHashIndex_(const Key& key) const
    {
        // No need to acquire any lock, which has been done in get(), insert(), and erase()

        assert(hash_wrapper_ptr_ != NULL);
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t hashidx = hash_value % hashtables_.size();
        return hashidx;
    }
}

#endif
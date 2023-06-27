#ifndef CONCURRENT_HASHTABLE_IMPL_H
#define CONCURRENT_HASHTABLE_IMPL_H

#include "common/param.h"
#include "concurrency/concurrent_hashtable.h"

namespace covered
{
    template<class V, class Hasher>
    const uint32_t ConcurrentHashtable<V, Hasher>::CONCURRENT_HASHTABLE_BUCKET_COUNT = 1000;

    template<class V, class Hasher>
    const std::string ConcurrentHashtable<V, Hasher>::kClassName("ConcurrentHashtable");

    template<class V, class Hasher>
    ConcurrentHashtable<V, Hasher>::ConcurrentHashtable(const uint32_t& bucket_count)
    {
        rwlocks_.resize(bucket_count);

        hashtables_.resize(bucket_count);
        for (uint32_t bucket_idx = 0; bucket_idx < bucket_count; bucket_idx++)
        {
            hashtables_[bucket_idx].resize(0);
        }

        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapper(Param::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);
    }

    template<class V, class Hasher>
    ConcurrentHashtable<V, Hasher>::~ConcurrentHashtable()
    {
        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    template<class V, class Hasher>
    V ConcurrentHashtable<V, Hasher>::get(const Key& key) const
    {
        // Acquire a read lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock_shared())
            {
                break;
            }
        }

        V v;
        const std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        std::unordered_map<Key, V, Hasher>::const_iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            v = iter->second;
        }

        // Release the read lock
        rwlocks_[hashidx].unlock_shared();

        return v;
    }

    template<class V, class Hasher>
    void ConcurrentHashtable<V, Hasher>::update(const Key& key, const V& v)
    {
        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter == tmp_hashtable.end()) // key NOT exist
        {
            tmp_hashtable.insert(std::pair<Key, V>(key, v));
        }
        else // key exists
        {
            iter->second = v;
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V, class Hasher>
    void ConcurrentHashtable<V, Hasher>::erase(const Key& key)
    {
        // Acquire a write lock
        uint32_t hashidx = getHashIndex_(key);
        while (true)
        {
            if (rwlocks_[hashidx].try_lock())
            {
                break;
            }
        }

        std::unordered_map<Key, V, Hasher>& tmp_hashtable = hashtables_[hashidx];
        std::unordered_map<Key, V, Hasher>::iterator iter = tmp_hashtable.find(key);
        if (iter != tmp_hashtable.end()) // key exists
        {
            tmp_hashtable.erase(iter);
        }

        // Release the write lock
        rwlocks_[hashidx].unlock();

        return;
    }

    template<class V, class Hasher>
    uint32_t ConcurrentHashtable<V, Hasher>::getHashIndex_(const Key& key)
    {
        // No need to acquire any lock, which has been done in get(), insert(), and erase()

        assert(hash_wrapper_ptr_ != NULL);
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t hashidx = hash_value % rwlocks_.size();
        return hashidx;
    }
}

#endif
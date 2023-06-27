/*
 * ConcurrentHashtable: a hash table supports concurrent accesses by fine-grained locking (thread safe).
 * 
 * By Siyuan Sheng (2023.06.27).
 */

#ifndef CONCURRENT_HASHTABLE_H
#define CONCURRENT_HASHTABLE_H

#include <string>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    template<class V, class Hasher>
    class ConcurrentHashtable
    {
    public:
        static const uint32_t CONCURRENT_HASHTABLE_BUCKET_COUNT;

        ConcurrentHashtable(const uint32_t& bucket_count = CONCURRENT_HASHTABLE_BUCKET_COUNT);
        ~ConcurrentHashtable();

        V get(const Key& key) const;
        void update(const Key& key, const V& v); // Insert if key does not exist
        void erase(const Key& key); // Erase if key exists
    private:
        static const std::string kClassName;

        uint32_t getHashIndex_(const Key& key);

        std::vector<boost::shared_mutex> rwlocks_; // NOTE: boost::shared_mutex does NOT have copy constructor
        std::vector<std::unordered_map<Key, V, Hasher>> hashtables_;
        HashWrapperBase* hash_wrapper_ptr_;
    };
}

#endif
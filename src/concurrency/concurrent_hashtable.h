/*
 * ConcurrentHashtable: a hash table supports concurrent accesses by fine-grained locking (thread safe).
 * 
 * By Siyuan Sheng (2023.06.27).
 */

#ifndef CONCURRENT_HASHTABLE_H
#define CONCURRENT_HASHTABLE_H

#include <atomic>
#include <string>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    // NOTE: class V must support default constructor, operator=, getSizeForCapacity(), and call/constCall(const std::string& function_name, void* param_ptr)
    // NOTE: V::call() returns a boolean indicating whether to erase the key-value pair or not
    template<class V>
    class ConcurrentHashtable
    {
    public:
        static const uint32_t CONCURRENT_HASHTABLE_BUCKET_COUNT;

        ConcurrentHashtable(const std::string& table_name, const V& default_value, const uint32_t& bucket_count = CONCURRENT_HASHTABLE_BUCKET_COUNT);
        ~ConcurrentHashtable();

        // NOTE: thread-safe structure cannot return a reference, which may violate atomicity
        bool isExist(const Key& key) const;
        //V getIfExist(const Key& key, bool& is_exist) const; // Get if key exists
        void insertOrUpdate(const Key& key, const V& value, bool& is_exist); // Insert a new value if key does not exist, or update the value if key exists
        void insertOrCall(const Key& key, const V& value, bool& is_exist, const std::string& function_name, void* param_ptr); // Insert a new value if key does not exist, or call value.function_name if key exists
        void callIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr); // Call value.function_name if key exists
        void constCallIfExist(const Key& key, bool& is_exist, const std::string& function_name, void* param_ptr) const; // Const call value.function_name if key exists
        void eraseIfExist(const Key& key, bool& is_exist); // Erase if key exists

        uint32_t getTotalKeySizeForCapcity() const;
        uint32_t getTotalValueSizeForCapcity() const;
    private:
        static const std::string kClassName;

        uint32_t getHashIndex_(const Key& key) const;
        void updateTotalValueSize_(uint32_t current_value_size, uint32_t original_value_size);

        // Const shared varaibles
        std::string instance_name_;
        V default_value_;
        HashWrapperBase* hash_wrapper_ptr_;

        // Fine-grained locking for atomicity of non-const shared variables
        // NOTE: we have to use dynamic array for boost::shared_mutex, as it does NOT have copy constructor and operator= for std::vector (e.g., resize() and push_back())
        mutable boost::shared_mutex* rwlocks_;

        // Non-const shared variables
        std::vector<std::unordered_map<Key, V, KeyHasher>> hashtables_;

        // NOn-const shared variables (thread safe)
        std::atomic<uint32_t> total_key_size_;
        std::atomic<uint32_t> total_value_size_;
    };
}

#endif
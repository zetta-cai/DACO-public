/*
 * ConcurrentHashtable: a hash table supports concurrent accesses by PerkeyRwlock (thread safe).
 *
 * NOTE: ConcurrentHashtable guarantees thread-safety for writes of different keys, and atomicity for writes of the same key.
 * NOTE: To guarantee the atomicity among multiple ConcurrentHashtables for writes of the same key, you should pass the same PerkeyRwlock.
 * 
 * By Siyuan Sheng (2023.06.27).
 */

#ifndef CONCURRENT_HASHTABLE_H
#define CONCURRENT_HASHTABLE_H

#include <atomic>
#include <string>
#include <vector>

#include "common/key.h"
#include "concurrency/perkey_rwlock.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    // NOTE: class V must support default constructor, operator=, getSizeForCapacity(), and call/constCall(const std::string& function_name, void* param_ptr)
    // NOTE: V::call() returns a boolean indicating whether to erase the key-value pair or not
    template<class V>
    class ConcurrentHashtable
    {
    public:
        ConcurrentHashtable(const std::string& table_name, const V& default_value, const PerkeyRwlock* perkey_rwlock_ptr);
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

        void updateTotalValueSize_(uint32_t current_value_size, uint32_t original_value_size);

        // Const shared varaibles
        std::string instance_name_;
        V default_value_;
        const PerkeyRwlock* perkey_rwlock_ptr_;

        // Non-const shared variables
        std::vector<std::unordered_map<Key, V, KeyHasher>> hashtables_;

        // NOn-const shared variables (thread safe)
        std::atomic<uint32_t> total_key_size_;
        std::atomic<uint32_t> total_value_size_;
    };
}

#endif
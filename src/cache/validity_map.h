/*
 * ValidityMap: maintain per-key validity flag for each object cached in local edge cache of closest edge node (thread safe).
 *
 * NOTE: all non-const shared variables in ValidityMap should be thread safe.
 * 
 * By Siyuan Sheng (2023.06.19).
 */

#ifndef VALIDITY_MAP_H
#define VALIDITY_MAP_H

#include <string>

#include "common/key.h"
#include "concurrency/concurrent_hashtable_impl.h"
#include "concurrency/perkey_rwlock.h"

namespace covered
{
    class ValidityFlag
    {
    public:
        struct IsValidFlagParam
        {
            bool is_valid;
        };

        static const std::string IS_VALID_FLAG_FUNCNAME;

        ValidityFlag();
        ValidityFlag(const bool& is_valid);
        ~ValidityFlag();

        // (1) Access valid flag

        bool isValidFlag() const;

        // (2) For ConcurrentHashtable

        uint64_t getSizeForCapacity() const;
        bool call(const std::string& function_name, void* param_ptr);
        void constCall(const std::string& function_name, void* param_ptr) const;

        const ValidityFlag& operator=(const ValidityFlag& other);
    private:
        static const std::string kClassName;

        bool is_valid_;
    };

    class ValidityMap
    {
    public:
        ValidityMap(const uint32_t& edge_idx, const PerkeyRwlock* perkey_rwlock_ptr);
        ~ValidityMap();

        bool isKeyExist(const Key& key) const;
        bool isValidFlagForKey(const Key& key, bool& is_exist) const;
        void invalidateFlagForKey(const Key& key, bool& is_exist); // Add an invalid flag if key NOT exist
        void validateFlagForKey(const Key& key, bool& is_exist); // Add a valid flag if key NOT exist
        void eraseFlagForKey(const Key& key, bool& is_exist);

        uint64_t getSizeForCapacity() const;

        // ONLY used by edge snapshot
        void getAllKeyValidityPairs(std::unordered_map<Key, ValidityFlag, KeyHasher>& key_validity_map) const;
        void putKeyValidityPair(const Key& tmp_key, const ValidityFlag& tmp_validity_flag, bool& is_exist);
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Non-const shared variables
        // NOTE: as the flag of validity can be integrated into cache metadata, we ONLY count the flag instead of key into the total size for capacity limitation (validity_map_ is just an implementation trick to avoid hacking each individual cache)
        ConcurrentHashtable<ValidityFlag> perkey_validity_; // Validity metadata (thread safe)
    };
}

#endif
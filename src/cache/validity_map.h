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
#include "edge/edge_param.h"

namespace covered
{
    class ValidityFlag
    {
    public:
        ValidityFlag();
        ValidityFlag(const bool& is_valid);
        ~ValidityFlag();

        bool isValid() const;

        bool call(const std::string& function_name, void* param_ptr);
        uint32_t getSizeForCapacity() const;

        ValidityFlag& operator=(const ValidityFlag& other);
    private:
        static const std::string kClassName;

        bool is_valid_;
    };

    class ValidityMap
    {
    public:
        ValidityMap(EdgeParam* edge_param_ptr);
        ~ValidityMap();

        bool isValidObject(const Key& key, bool& is_found) const;
        void invalidateObject(const Key& key, bool& is_found);
        void validateObject(const Key& key, bool& is_found);
        void erase(const Key& key, bool& is_found);

        uint32_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Non-const shared variables
        // NOTE: as the flag of validity can be integrated into cache metadata, we ONLY count the flag instead of key into the total size for capacity limitation (validity_map_ is just an implementation trick to avoid hacking each individual cache)
        ConcurrentHashtable<ValidityFlag, KeyHasher> perkey_validity_; // Validity metadata (thread safe)
    };
}

#endif
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
#include <unordered_map>

#include "common/key.h"
#include "concurrency/rwlock.h"
#include "edge/edge_param.h"

namespace covered
{
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
        typedef std::unordered_map<Key, bool, KeyHasher> perkey_validity_t;

        static const std::string kClassName;

        // Const shared variables
        std::string instance_name_;

        // Guarantee the atomicity of validity_map_ (e.g., admit different keys)
        mutable Rwlock* rwlock_for_validity_ptr_;

        // NOTE: as the flag of validity can be integrated into cache metadata, we ONLY count the flag instead of key into the total size for capacity limitation (validity_map_ is just an implementation trick to avoid hacking each individual cache)
        perkey_validity_t perkey_validity_; // Metadata for local edge cache
    };
}

#endif
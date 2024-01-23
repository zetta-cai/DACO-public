/*
 * CBF: counting bloom filter (refer to lib/s3fifo/libCacheSim/libCacheSim/dataStructure/minimalIncrementCBF.*) used by some cache policies for cache management.
 *
 * By Siyuan Sheng (2024.01.23).
 */

#ifndef CBF_H
#define CBF_H

#include <string>
#include <vector>

#include "common/key.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    class CBF
    {
    public:
        CBF(const uint64_t& entries, const double& error); // Refer to minimalIncrementCBF_init() in lib/s3fifo/libCacheSim/libCacheSim/dataStructure/minimalIncrementCBF.c
        ~CBF();

        uint32_t estimate(const Key& key);
        uint32_t update(const Key& key);
        void decay();

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        uint32_t access_(const Key& key, const bool& is_update); // Refer to minimalIncrementCBF_check_add() in lib/s3fifo/libCacheSim/libCacheSim/dataStructure/minimalIncrementCBF.c

        std::vector<HashWrapperBase*> hash_wrappers_;
        std::vector<uint32_t> cbf_;
    };
}

#endif
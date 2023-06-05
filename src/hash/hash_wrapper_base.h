/*
 * HashWrapperBase: the base class for consistent hashing.
 * 
 * By Siyuan Sheng (2023.06.05).
 */

#ifndef HASH_WRAPPER_BASE_H
#define HASH_WRAPPER_BASE_H

#include <string>

#include "common/key.h"

namespace covered
{
    enum HashType
    {
        kMmh3Hash = 1
    };

    class HashWrapperBase
    {
    public:
        static std::string hashTypeToString(const HashType& hash_type);
        static HashWrapperBase* getHashWrapper(const HashType& hash_type);

        HashWrapperBase();
        virtual ~HashWrapperBase();

        // Hash functions for different types of input
        uint32_t hash(const Key& key) const;
    private:
        static const std::string kClassName;

        virtual uint32_t hashInternal_(const char *input, uint32_t length) const = 0;
    };
}

#endif
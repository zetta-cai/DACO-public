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
    class HashWrapperBase
    {
    public:
        static HashWrapperBase* getHashWrapperByHashName(const std::string& hash_name);

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
/*
 * Mmh3HashWrapper: mmh3-based consistent hashing.
 * 
 * By Siyuan Sheng (2023.06.05).
 */

#ifndef MMH3_HASH_WRAPPER_H
#define MMH3_HASH_WRAPPER_H

#include <string>

#include "hash_wrapper_base.h"

namespace covered
{
    class Mmh3HashWrapper : public HashWrapperBase
    {
    public:
        Mmh3HashWrapper(const uint32_t& seed = 0);
        virtual ~Mmh3HashWrapper();
    private:
        static const std::string kClassName;

        virtual uint32_t hashInternal_(const char *input, uint32_t length) const override;

        const uint32_t seed_;
    };
}

#endif
#include "hash/mmh3_hash_wrapper.h"

#include <assert.h>

#include "MurmurHash3.h"

namespace covered
{
    const std::string Mmh3HashWrapper::kClassName("Mmh3HashWrapper");

    Mmh3HashWrapper::Mmh3HashWrapper(const uint32_t& seed) : seed_(seed)
    {
    }

    Mmh3HashWrapper::~Mmh3HashWrapper() {}

    uint32_t Mmh3HashWrapper::hashInternal_(const char *input, uint32_t length) const
    {
        assert(input != NULL);

        uint32_t hash_value = 0;
        MurmurHash3_x86_32((const void*)input, static_cast<int>(length), seed_, &hash_value);
        return hash_value;
    }
}
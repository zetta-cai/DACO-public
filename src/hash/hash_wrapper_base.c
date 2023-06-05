#include "hash/hash_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "hash/mmh3_hash_wrapper.h"

namespace covered
{
    const std::string HashWrapperBase::kClassName("HashWrapperBase");

    std::string HashWrapperBase::hashTypeToString(const HashType& hash_type)
    {
        std::string hash_type_str = "";
        switch (hash_type)
        {
            case HashType::kMmh3Hash:
            {
                hash_type_str = "kMmh3Hash";
                break;
            }
            default:
            {
                hash_type_str = std::to_string(static_cast<uint32_t>(hash_type));
                break;
            }
        }
        return hash_type_str;
    }

    HashWrapperBase* HashWrapperBase::getHashWrapper(const HashType& hash_type)
    {
        HashWrapperBase* hash_wrapper_base_ptr = NULL;
        switch (hash_type)
        {
            case HashType::kMmh3Hash:
            {
                hash_wrapper_base_ptr = new Mmh3HashWrapper();
                break;
            }
            default:
            {
                std::ostringstream oss;
                oss << "hash type " << HashWrapperBase::hashTypeToString(hash_type) << " is not supported!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
                break;
            }
        }
        assert(hash_wrapper_base_ptr != NULL);
        return hash_wrapper_base_ptr;
    }

    HashWrapperBase::HashWrapperBase() {}

    HashWrapperBase::~HashWrapperBase() {}

    uint32_t HashWrapperBase::hash(const Key& key) const
    {
        return hashInternal_(key.getKeystr().data(), key.getKeystr().length());
    }
}
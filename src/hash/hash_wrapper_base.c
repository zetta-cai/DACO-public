#include "hash/hash_wrapper_base.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"
#include "hash/mmh3_hash_wrapper.h"

namespace covered
{
    const std::string HashWrapperBase::kClassName("HashWrapperBase");

    HashWrapperBase* HashWrapperBase::getHashWrapperByHashName(const std::string& hash_name)
    {
        HashWrapperBase* hash_wrapper_base_ptr = NULL;
        if (hash_name == Param::MMH3_HASH_NAME)
        {
            hash_wrapper_base_ptr = new Mmh3HashWrapper();
        }
        else
        {
            std::ostringstream oss;
            oss << "hash name " << hash_name << " is not supported!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
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
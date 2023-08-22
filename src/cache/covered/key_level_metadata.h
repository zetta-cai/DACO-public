/*
 * KeyLevelMetadata: key-level metadata for each object in local edge cache.
 * 
 * By Siyuan Sheng (2023.08.16).
 */

#ifndef KEY_LEVEL_METADATA_H
#define KEY_LEVEL_METADATA_H

#include <string>

#include "cache/covered/common_header.h"

namespace covered
{
    class KeyLevelMetadata
    {
    public:
        KeyLevelMetadata(const GroupId& group_id);
        ~KeyLevelMetadata();

        void updateDynamicMetadata();

        GroupId getGroupId() const;
        uint32_t getFrequency() const;
    private:
        static const std::string kClassName;

        // TODO: Tune per-variable size later

        // Const metadata
        const GroupId group_id_;

        // Non-const dynamic metadata
        uint32_t frequency_;
    };
}

#endif
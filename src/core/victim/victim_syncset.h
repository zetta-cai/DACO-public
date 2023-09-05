/*
 * VictimSyncset: maintain VictimCacheinfos of local synced victims and DirectoryInfos of synced victims beaconed by the current edge node for vicitm syncrhonizatino across edge nodes by piggybacking.
 *
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef VICTIM_SYNCSET_H
#define VICTIM_SYNCSET_H

#include <list>
#include <string>

#include "cooperation/directory/directory_info.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class VictimSyncset
    {
    public:
        VictimSyncset();
        VictimSyncset(const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_victims);
        ~VictimSyncset();

        const std::list<VictimCacheinfo>& getLocalSyncedVictimsRef() const;

        uint32_t getVictimSyncsetPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const VictimSyncset& operator=(const VictimSyncset& other);
    private:
        static const std::string kClassName;

        std::list<VictimCacheinfo> local_synced_victims_;
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_victims_;
    };
}

#endif
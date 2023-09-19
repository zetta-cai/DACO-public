/*
 * VictimSyncset: maintain VictimCacheinfos of local synced victims and DirectoryInfos of synced victims beaconed by the current edge node for vicitm syncrhonizatino across edge nodes by piggybacking.
 *
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef VICTIM_SYNCSET_H
#define VICTIM_SYNCSET_H

#include <list>
#include <string>
#include <unordered_map>

#include "cooperation/directory/directory_info.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class VictimSyncset
    {
    public:
        VictimSyncset();
        VictimSyncset(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& local_beaconed_victims);
        ~VictimSyncset();

        uint64_t getCacheMarginBytes() const;
        const std::list<VictimCacheinfo>& getLocalSyncedVictimsRef() const;
        const std::unordered_map<Key, dirinfo_set_t, KeyHasher>& getLocalBeaconedVictimsRef() const;

        uint32_t getVictimSyncsetPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        const VictimSyncset& operator=(const VictimSyncset& other);
    private:
        static const std::string kClassName;

        uint64_t cache_margin_bytes_; // Remaining bytes of local edge cache in sender
        std::list<VictimCacheinfo> local_synced_victims_;
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_victims_;
    };
}

#endif
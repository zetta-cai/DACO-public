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

        static const uint8_t COMPLETE_BITMAP; // All fields are NOT deduped/delta-compressed
        static const uint8_t CACHE_MARGIN_BYTES_DELTA_MASK; // Whether cache margin bytes is delta compressed
        static const uint8_t LOCAL_SYNCED_VICTIMS_DEDUP_MASK; // Whether at least one victim cacheinfo of a local synced victim is deduped
        static const uint8_t LOCAL_BEACONED_VICTIMS_DEDUP_MASK; // Whether at least one dirinfo set of a local beaconed victim is deduped

        uint8_t compressed_bitmap_; // Whether the syncset is a compressed victim syncset (1st lowest bit for cache margin bytes; 2nd lowest bit for local synced victims; 3rd lowest bit for local beaconed victims)

        // Delta compression (32-bit delta is enough for limited changes of cache margin bytes)
        uint64_t cache_margin_bytes_; // For complete victim syncset: remaining bytes of local edge cache in sender
        uint32_t cache_margin_delta_bytes_; // For compressed victim syncset: delta of cache margin bytes will NOT exceed max object size (e.g., several MiBs in Facebook CDN traces)

        // Deduplication (relatively stable victim cacheinfos of unpopular cached objects; most-eviction-few-admission victim dirinfo set of unpopular cached objects)
        std::list<VictimCacheinfo> local_synced_victims_; // For complete victim syncset: all complete victim cacheinfos; for compressed victim syncset: new victim cacheinfos (complete newly-synced victims or compressed existing victims w/ cacheinfo changes)
        // TODO: END HERE
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> local_beaconed_victims_;
        std::unordered_map<Key, dirinfo_set_t, KeyHasher> new_local_beaconed_victims_; // New dirinfo sets for newly-beaconed victims or existing beaconed victims -> dedup is enough for relatively stable dirinfo sets of unpopular cached objects
    };
}

#endif
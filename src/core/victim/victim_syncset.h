/*
 * VictimSyncset: maintain VictimCacheinfos of local synced victims and DirinfoSets of synced victims beaconed by the current edge node for vicitm syncrhonizatino across edge nodes by piggybacking.
 *
 * NOTE: transmited victim cacheinfos and dirinfo sets could be either completed or compressed.
 * 
 * By Siyuan Sheng (2023.08.31).
 */

#ifndef VICTIM_SYNCSET_H
#define VICTIM_SYNCSET_H

//#define DEBUG_VICTIM_SYNCSET

#include <list>
#include <string>
#include <unordered_map>

#include "cooperation/directory/dirinfo_set.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class VictimSyncset
    {
    public:
        static VictimSyncset compress(const VictimSyncset& current_victim_syncset, const VictimSyncset& prev_victim_syncset); // Compress current victim syncset w.r.t. previous victim syncset
        static VictimSyncset recover(const VictimSyncset& compressed_victim_syncset, const VictimSyncset& existing_victim_syncset); // Recover existing victim syncset w.r.t. compressed victim syncset

        VictimSyncset();
        VictimSyncset(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims);
        ~VictimSyncset();

        bool isComplete() const;
        bool isCompressed() const;

        // For both complete and compressed victim syncsets
        bool getCacheMarginBytesOrDelta(uint64_t& cache_margin_bytes, int& cache_margin_delta_bytes) const; // Return if with complete cache margin bytes
        bool getLocalSyncedVictims(std::list<VictimCacheinfo>& local_synced_vitims) const; // Return if with complete local synced vitim cacheinfos
        bool getLocalSyncedVictimsAsMap(std::unordered_map<Key, VictimCacheinfo, KeyHasher>& local_synced_vitims_map) const; // Return if with complete local synced vitim cacheinfos
        bool getLocalBeaconedVictims(std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims) const; // Return if with complete local beaconed vitim dirinfo sets

        // For complete victim syncset
        void setCacheMarginBytes(const uint64_t& cache_margin_bytes);
        void setLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victims);
        void setLocalBeaconedVictims(const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims);

        // For compressed victim syncset
        void setCacheMarginDeltaBytes(const int& cache_margin_delta_bytes);
        void setLocalSyncedVictimsForCompress(const std::list<VictimCacheinfo>& local_synced_victims);
        void setLocalBeaconedVictimsForCompress(const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims);

        uint32_t getVictimSyncsetPayloadSize() const;
        uint32_t serialize(DynamicArray& msg_payload, const uint32_t& position) const;
        uint32_t deserialize(const DynamicArray& msg_payload, const uint32_t& position);

        uint64_t getSizeForCapacity() const;

        const VictimSyncset& operator=(const VictimSyncset& other);
    private:
        static const std::string kClassName;

        static const uint8_t INVALID_BITMAP; // Invalid bitmap
        static const uint8_t COMPLETE_BITMAP; // All fields are NOT deduped/delta-compressed
        static const uint8_t COMPRESS_MASK; // At least one filed is deduped/delta-compressed
        static const uint8_t CACHE_MARGIN_BYTES_DELTA_MASK; // Whether cache margin bytes is delta compressed
        static const uint8_t LOCAL_SYNCED_VICTIMS_DEDUP_MASK; // Whether at least one victim cacheinfo of a local synced victim is deduped
        static const uint8_t LOCAL_SYNCED_VICTIMS_EMPTY_MASK; // Whether local synced victims is empty (e.g., no local synced victim for complete victim syncset, or all local synced victims are fully-deduped and hence NOT tracked for compressed victim syncset)
        static const uint8_t LOCAL_BEACONED_VICTIMS_DEDUP_MASK; // Whether at least one dirinfo set of a local beaconed victim is deduped
        static const uint8_t LOCAL_BEACONED_VICTIMS_EMPTY_MASK; // Whether local beaconed victims is empty (e.g., no local beaconed victim for complete victim syncset, or all local beaconed victims are fully-compressed and hence NOT tracked for compressed victim syncset)

        void assertAtLeastOneCacheinfoDeduped_() const;
        void assertAllCacheinfosComplete_() const;
        void assertAtLeastOneDirinfoSetCompressed_() const;
        void assertAllDirinfoSetsComplete_() const;

        uint8_t compressed_bitmap_; // 1st lowest bit indicates if the syncset is a compressed victim syncset (2nd lowest bit for cache margin bytes; 3rd/4th lowest bit for local synced victims; 5th/6th lowest bit for local beaconed victims)

        // Delta compression (32-bit delta is enough for limited changes of cache margin bytes)
        uint64_t cache_margin_bytes_; // ONLY for complete victim syncset: remaining bytes of local edge cache in sender
        int cache_margin_delta_bytes_; // ONLY for compressed victim syncset: delta of cache margin bytes will NOT exceed max object size (e.g., several MiBs in Facebook CDN traces)

        // Deduplication (relatively stable victim cacheinfos of unpopular cached objects)
        // (1) For complete victim syncset: all complete victim cacheinfos with ascending order of local rewards; (2) for compressed victim syncset: new victim cacheinfos (complete newly-synced victims or compressed existing victims w/ cacheinfo changes) w/o order
        // NOTE: we need to track stale victim cacheinfos of local-unsynced keys to remove them from victim tracker at neighbor edge node
        std::list<VictimCacheinfo> local_synced_victims_; // For both complete and compressed victim syncset

        // Delta compression (most-eviction-few-admission victim dirinfo set of unpopular cached objects)
        // (1) For complete victim syncset: all complete victim dirinfo sets; (2) for compressed victim syncset: new victim dirinfo sets (complete newly-beaconed victims or compressed existing victims w/ dirinfo set changes)
        // NOTE: we do NOT need to track stale dirinfo sets of local-/neighbor-unsynced keys, as they will be removed after refcnt changes to zero (triggered by stale victim cacheinfo removal)
        std::unordered_map<Key, DirinfoSet, KeyHasher> local_beaconed_victims_; // For both complete and compressed victim syncset
    };
}

#endif
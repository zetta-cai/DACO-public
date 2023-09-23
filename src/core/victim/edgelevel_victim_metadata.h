/*
 * EdgelevelVictimMetadata: track victim cacheinfos and cache margin bytes in an edge node.
 *
 * By Siyuan Sheng (2023.09.20).
 */

#ifndef EDGELEVLE_VICTIM_METADATA_H
#define EDGELEVLE_VICTIM_METADATA_H

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class EdgelevelVictimMetadata
    {
    public:
        EdgelevelVictimMetadata();
        EdgelevelVictimMetadata(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos);
        ~EdgelevelVictimMetadata();

        uint64_t getCacheMarginBytes() const;
        std::list<VictimCacheinfo> getVictimCacheinfos() const;
        const std::list<VictimCacheinfo>& getVictimCacheinfosRef() const;

        // Return if need to proactively fetch more victims (cur_edge_idx is the edge node corresponding to this EdgelevelVictimMetadata)
        bool findVictimsForObjectSize(const uint32_t& cur_edge_idx, const ObjectSize& object_size, std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos) const;

        // NOTE: removed victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node
        void removeVictimsForPlacement(const std::unordered_set<Key, KeyHasher>& victim_keyset);

        const EdgelevelVictimMetadata& operator=(const EdgelevelVictimMetadata& other);
    private:
        static const std::string kClassName;

        uint64_t cache_margin_bytes_;
        std::list<VictimCacheinfo> victim_cacheinfos_; // Victims are sorted in an ascending order of local rewards
    };
}

#endif
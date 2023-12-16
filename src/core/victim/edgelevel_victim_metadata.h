/*
 * EdgelevelVictimMetadata: track victim cacheinfos and cache margin bytes in an edge node.
 *
 * NOTE: all victim cacheinfos stored in each edge node (including local edge cache, edge-level victim metadata, extra fetched victims, and victim metadata cache of victim tracker) MUST be complete (ONLY transmitted victim cacheinfos could be compressed).
 *
 * By Siyuan Sheng (2023.09.20).
 */

#ifndef EDGELEVLE_VICTIM_METADATA_H
#define EDGELEVLE_VICTIM_METADATA_H

#include <list>
#include <string>

#include "core/popularity/edgeset.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    class EdgelevelVictimMetadata
    {
    public:
        EdgelevelVictimMetadata();
        EdgelevelVictimMetadata(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos);
        ~EdgelevelVictimMetadata();

        bool isValid() const;
        void validate(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos);

        void updateCacheMarginBytes(const uint64_t& cache_margin_bytes);

        uint64_t getCacheMarginBytes() const;
        std::list<VictimCacheinfo> getVictimCacheinfos() const;
        const std::list<VictimCacheinfo>& getVictimCacheinfosRef() const;

        // NOTE: cur_edge_idx is the edge node corresponding to this EdgelevelVictimMetadata
        // NOTE: pervictim_edgeset and pervictim_cacheinfos are used for eviction cost in placement calculation, peredge_synced_victimset and peredge_fetched_victimset are used for victim removal in non-blocking placement deployment, and victim_fetch_edgeset is used for lazy victim fetching
        void findVictimsForObjectSize(const uint32_t& cur_edge_idx, const ObjectSize& object_size, std::list<std::pair<Key, Edgeset>>& pervictim_edgeset, std::list<std::pair<Key, std::list<VictimCacheinfo>>>& pervictim_cacheinfos, std::list<std::pair<uint32_t, std::list<Key>>>& peredge_synced_victimset, std::list<std::pair<uint32_t, std::list<Key>>>& peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::list<VictimCacheinfo>& extra_victim_cacheinfos) const;

        // NOTE: removed victims should NOT be reused <- if synced victims in the edge node do NOT change, removed victims will NOT be reported to the beacon node due to dedup/delta-compression in victim synchronization; if need more victims, victim fetching request MUST be later than placement notification request, which has changed the synced victims in the edge node
        bool removeVictimsForPlacement(const std::list<Key>& victim_keyset, uint64_t& removed_cacheinfos_size); // Return if victim_cacheinfos_ is empty; removed_size indicates the size of all removed victim cacheinfos

        const EdgelevelVictimMetadata& operator=(const EdgelevelVictimMetadata& other);
    private:
        static const std::string kClassName;

        // Utils

        void updatePervictimEdgeset_(std::list<std::pair<Key, Edgeset>>& pervictim_edgeset, const Key& victim_key, const uint32_t edge_idx) const;
        void updatePervictimCacheinfos_(std::list<std::pair<Key, std::list<VictimCacheinfo>>>& pervictim_cacheinfos, const Key& victim_key, const VictimCacheinfo& victim_cacheinfo) const;
        void updatePeredgeVictimset_(std::list<std::pair<uint32_t, std::list<Key>>>& peredge_victimset, const uint32_t& edge_idx, const Key& victim_key) const;

        bool is_valid_; // Whether the edge-level victim metadata is initialized
        void checkValidity_() const;

        uint64_t cache_margin_bytes_; // Maintain per-edge-node margin bytes to decide whether to find victims or not
        std::list<VictimCacheinfo> victim_cacheinfos_; // Victims are sorted in an ascending order of local rewards
    };
}

#endif
#include "core/victim/edgelevel_victim_metadata.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string EdgelevelVictimMetadata::kClassName = "EdgelevelVictimMetadata";

    EdgelevelVictimMetadata::EdgelevelVictimMetadata() : cache_margin_bytes_(0)
    {
        victim_cacheinfos_.clear();
    }

    EdgelevelVictimMetadata::EdgelevelVictimMetadata(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        cache_margin_bytes_ = cache_margin_bytes;
        victim_cacheinfos_ = victim_cacheinfos;
    }

    EdgelevelVictimMetadata::~EdgelevelVictimMetadata() {}

    uint64_t EdgelevelVictimMetadata::getCacheMarginBytes() const
    {
        return cache_margin_bytes_;
    }

    std::list<VictimCacheinfo> EdgelevelVictimMetadata::getVictimCacheinfos() const
    {
        return victim_cacheinfos_;
    }

    const std::list<VictimCacheinfo>& EdgelevelVictimMetadata::getVictimCacheinfosRef() const
    {
        return victim_cacheinfos_;
    }

    void EdgelevelVictimMetadata::findVictimsForObjectSize(const uint32_t& cur_edge_idx, const ObjectSize& object_size, std::unordered_map<Key, Edgeset, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_synced_victimset, std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::list<VictimCacheinfo>& extra_victim_cacheinfos) const
    {
        // NOTE: NO need to clear pervictim_edgeset, pervictim_cacheinfos, peredge_synced_victimset, and peredge_fetched_victimset, which has been done by VictimTracker::findVictimsForPlacement_()

        bool need_more_victims = false;
        const bool with_extra_victims = (extra_victim_cacheinfos.size() > 0);

        // NOTE: If object size of admitted object <= cache margin bytes, the edge node does NOT need to evict any victim and hence contribute zero to eviction cost; otherwise, find victims based on required bytes and trigger lazy victim fetching if necessary (i.e., set need_more_victims = true)
        uint64_t tmp_required_bytes = 0;
        uint64_t tmp_saved_bytes = 0;
        if (object_size > cache_margin_bytes_) // Without sufficient cache space
        {
            tmp_required_bytes = object_size - cache_margin_bytes_;

            for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = victim_cacheinfos_.begin(); cacheinfo_list_const_iter != victim_cacheinfos_.end(); cacheinfo_list_const_iter++) // Note that victim_cacheinfos_ follows the ascending order of local rewards
            {
                const VictimCacheinfo& tmp_victim_cacheinfo = *cacheinfo_list_const_iter;
                const Key& tmp_victim_key = tmp_victim_cacheinfo.getKey();

                // Update per-victim edgeset
                updatePervictimEdgeset_(pervictim_edgeset, tmp_victim_key, cur_edge_idx);

                // Update per-victim cacheinfos
                updatePervictimCacheinfos_(pervictim_cacheinfos, tmp_victim_key, tmp_victim_cacheinfo);

                // Update per-edge victim keyset
                updatePeredgeVictimset_(peredge_synced_victimset, cur_edge_idx, tmp_victim_key);

                // Check if we have found sufficient victims for the required bytes
                const ObjectSize tmp_victim_object_size = tmp_victim_cacheinfo.getObjectSize();
                tmp_saved_bytes += tmp_victim_object_size;
                if (tmp_saved_bytes >= tmp_required_bytes) // With sufficient victims for the required bytes
                {
                    break;
                }
            } // End of tmp_victim_cacheinfos_ref in the tmp_edge_idx

            if (tmp_saved_bytes < tmp_required_bytes)
            {
                need_more_victims = true;
            }
        } // End of (object_size > tmp_cache_margin_bytes)

        // Update victim fetch edgeset for lazy victim fetching
        if (need_more_victims)
        {
            if (with_extra_victims) // DISABLE recursive victim fetching
            {
                assert(tmp_saved_bytes < tmp_required_bytes);

                // Enumerate extra victims to update per-victim edgeset, per-victim cacheinfos, and per-edge victim keyset
                std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>::const_iterator peredge_synced_victimset_const_iter = peredge_synced_victimset.find(cur_edge_idx);
                for (std::list<VictimCacheinfo>::const_iterator extra_victim_cacheinfos_const_iter = extra_victim_cacheinfos.begin(); extra_victim_cacheinfos_const_iter != extra_victim_cacheinfos.end(); extra_victim_cacheinfos_const_iter++)
                {
                    const Key tmp_victim_key = extra_victim_cacheinfos_const_iter->getKey();

                    // Skip the extra victim that has already been considered by the current edge idx
                    if (peredge_synced_victimset_const_iter != peredge_synced_victimset.end())
                    {
                        const std::unordered_set<Key, KeyHasher>& synced_victimset_ref = peredge_synced_victimset_const_iter->second;
                        if (synced_victimset_ref.find(tmp_victim_key) != synced_victimset_ref.end())
                        {
                            continue;
                        }
                    }

                    // Update per-victim edgeset
                    updatePervictimEdgeset_(pervictim_edgeset, tmp_victim_key, cur_edge_idx);

                    // Update per-victim cacheinfos
                    updatePervictimCacheinfos_(pervictim_cacheinfos, tmp_victim_key, *extra_victim_cacheinfos_const_iter);

                    // Update per-edge victimset
                    updatePeredgeVictimset_(peredge_fetched_victimset, cur_edge_idx, tmp_victim_key);

                    // Check if we have found sufficient victims for the required bytes
                    const ObjectSize tmp_victim_object_size = extra_victim_cacheinfos_const_iter->getObjectSize();
                    tmp_saved_bytes += tmp_victim_object_size;
                    if (tmp_saved_bytes >= tmp_required_bytes) // With sufficient victims for the required bytes
                    {
                        break;
                    }
                } // End of extra victims
            }
            else // Lazy vicim fetching
            {
                std::unordered_set<uint32_t>::iterator victim_fetch_edgeset_iter = victim_fetch_edgeset.find(cur_edge_idx);
                assert(victim_fetch_edgeset_iter == victim_fetch_edgeset.end());
                victim_fetch_edgeset_iter = victim_fetch_edgeset.insert(cur_edge_idx).first;
                assert(victim_fetch_edgeset_iter != victim_fetch_edgeset.end());
            }
        }

        return;
    }

    void EdgelevelVictimMetadata::removeVictimsForPlacement(const std::unordered_set<Key, KeyHasher>& victim_keyset)
    {
        // Remove victims from victim_cacheinfos_
        for (std::unordered_set<Key, KeyHasher>::const_iterator victim_keyset_const_iter = victim_keyset.begin(); victim_keyset_const_iter != victim_keyset.end(); victim_keyset_const_iter++)
        {
            bool found_victim = false;
            for (std::list<VictimCacheinfo>::iterator cacheinfo_list_iter = victim_cacheinfos_.begin(); cacheinfo_list_iter != victim_cacheinfos_.end(); cacheinfo_list_iter++)
            {
                if (cacheinfo_list_iter->getKey() == *victim_keyset_const_iter) // Find the correpsonding victim cacheinfo
                {
                    victim_cacheinfos_.erase(cacheinfo_list_iter);
                    found_victim = true;
                    break;
                }
            }

            // NOTE: as victim keyset is generated by victim tracker based on victim_cacheinfos_, each synced victim MUST have the corresponding victim cacheinfo
            assert(found_victim == true);
        }

        // NOTE: even if all victim cacheinfos are removed, we still keep the EdgeLevelVictimMetadata for placement calculation (see VictimTracker::findVictimsForPlacement_())
        
        return;
    }

    const EdgelevelVictimMetadata& EdgelevelVictimMetadata::operator=(const EdgelevelVictimMetadata& other)
    {
        if (this != &other)
        {
            cache_margin_bytes_ = other.cache_margin_bytes_;
            victim_cacheinfos_ = other.victim_cacheinfos_;
        }

        return *this;
    }

    void EdgelevelVictimMetadata::updatePervictimEdgeset_(std::unordered_map<Key, Edgeset, KeyHasher>& pervictim_edgeset, const Key& victim_key, const uint32_t edge_idx) const
    {
        std::unordered_map<Key, Edgeset, KeyHasher>::iterator pervictim_edgeset_iter = pervictim_edgeset.find(victim_key);
        if (pervictim_edgeset_iter == pervictim_edgeset.end())
        {
            pervictim_edgeset_iter = pervictim_edgeset.insert(std::pair(victim_key, Edgeset())).first;
        }
        assert(pervictim_edgeset_iter != pervictim_edgeset.end());
        pervictim_edgeset_iter->second.insert(edge_idx);
        return;
    }

    void EdgelevelVictimMetadata::updatePervictimCacheinfos_(std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos, const Key& victim_key, const VictimCacheinfo& victim_cacheinfo) const
    {
        std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>::iterator pervictim_cacheinfos_iter = pervictim_cacheinfos.find(victim_key);
        if (pervictim_cacheinfos_iter == pervictim_cacheinfos.end())
        {
            pervictim_cacheinfos_iter = pervictim_cacheinfos.insert(std::pair(victim_key, std::list<VictimCacheinfo>())).first;
        }
        assert(pervictim_cacheinfos_iter != pervictim_cacheinfos.end());
        pervictim_cacheinfos_iter->second.push_back(victim_cacheinfo);
        return;
    }

    void EdgelevelVictimMetadata::updatePeredgeVictimset_(std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>& peredge_victimset, const uint32_t& edge_idx, const Key& victim_key) const
    {
        std::unordered_map<uint32_t, std::unordered_set<Key, KeyHasher>>::iterator peredge_victimset_iter = peredge_victimset.find(edge_idx);
        if (peredge_victimset_iter == peredge_victimset.end())
        {
            peredge_victimset_iter = peredge_victimset.insert(std::pair(edge_idx, std::unordered_set<Key, KeyHasher>())).first;
        }
        assert(peredge_victimset_iter != peredge_victimset.end());
        peredge_victimset_iter->second.insert(victim_key);
        return;
    }
}
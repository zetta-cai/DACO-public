#include "core/victim/edgelevel_victim_metadata.h"

#include <assert.h>

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

    bool EdgelevelVictimMetadata::findVictimsForObjectSize(const uint32_t& cur_edge_idx, const ObjectSize& object_size, std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos) const
    {
        // NOTE: NO need to clear pervictim_edgeset and pervictim_cacheinfos, which has been done by VictimTracker::findVictimsForPlacement_()

        bool need_more_victims = false;

        // NOTE: If object size of admitted object <= cache margin bytes, the edge node does NOT need to evict any victim and hence contribute zero to eviction cost; otherwise, find victims based on required bytes and trigger proactive fetching if necessary (i.e., set need_more_victims = true)
        if (object_size > cache_margin_bytes_) // Without sufficient cache space
        {
            const uint64_t tmp_required_bytes = object_size - cache_margin_bytes_;

            uint64_t tmp_saved_bytes = 0;
            for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = victim_cacheinfos_.begin(); cacheinfo_list_const_iter != victim_cacheinfos_.end(); cacheinfo_list_const_iter++) // Note that victim_cacheinfos_ follows the ascending order of local rewards
            {
                const VictimCacheinfo& tmp_victim_cacheinfo = *cacheinfo_list_const_iter;
                const Key& tmp_victim_key = tmp_victim_cacheinfo.getKey();

                // Update per-victim edgeset
                std::unordered_map<Key, std::unordered_set<uint32_t>, KeyHasher>::iterator pervictim_edgeset_iter = pervictim_edgeset.find(tmp_victim_key);
                if (pervictim_edgeset_iter == pervictim_edgeset.end())
                {
                    pervictim_edgeset_iter = pervictim_edgeset.insert(std::pair(tmp_victim_key, std::unordered_set<uint32_t>())).first;
                }
                assert(pervictim_edgeset_iter != pervictim_edgeset.end());
                pervictim_edgeset_iter->second.insert(cur_edge_idx);

                // Update per-victim cacheinfos
                std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>::iterator pervictim_cacheinfos_iter = pervictim_cacheinfos.find(tmp_victim_key);
                if (pervictim_cacheinfos_iter == pervictim_cacheinfos.end())
                {
                    pervictim_cacheinfos_iter = pervictim_cacheinfos.insert(std::pair(tmp_victim_key, std::list<VictimCacheinfo>())).first;
                }
                assert(pervictim_cacheinfos_iter != pervictim_cacheinfos.end());
                pervictim_cacheinfos_iter->second.push_back(tmp_victim_cacheinfo);

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

        return need_more_victims;
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

            // NOTE: as victim keyset is generated by victim tracker based on victim_cacheinfos_, each victim MUST have the corresponding victim cacheinfo
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
}
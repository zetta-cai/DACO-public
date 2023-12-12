#include "core/victim/edgelevel_victim_metadata.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string EdgelevelVictimMetadata::kClassName = "EdgelevelVictimMetadata";

    EdgelevelVictimMetadata::EdgelevelVictimMetadata() : is_valid_(false), cache_margin_bytes_(0)
    {
        victim_cacheinfos_.clear();
    }

    EdgelevelVictimMetadata::EdgelevelVictimMetadata(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& victim_cacheinfos)
    {
        is_valid_ = true;
        cache_margin_bytes_ = cache_margin_bytes;
        victim_cacheinfos_ = victim_cacheinfos;

        for (std::list<VictimCacheinfo>::const_iterator victim_cacheinfos_const_iter = victim_cacheinfos.begin(); victim_cacheinfos_const_iter != victim_cacheinfos.end(); victim_cacheinfos_const_iter++)
        {
            assert(victim_cacheinfos_const_iter->isComplete()); // NOTE: victim cacheinfos in edge-level victim metadata of victim tracker MUST be complete
        }
    }

    EdgelevelVictimMetadata::~EdgelevelVictimMetadata() {}

    bool EdgelevelVictimMetadata::isValid() const
    {
        return is_valid_;
    }

    void EdgelevelVictimMetadata::updateCacheMarginBytes(const uint64_t& cache_margin_bytes)
    {
        checkValidity_();

        cache_margin_bytes_ = cache_margin_bytes;
        return;
    }

    uint64_t EdgelevelVictimMetadata::getCacheMarginBytes() const
    {
        checkValidity_();

        return cache_margin_bytes_;
    }

    std::list<VictimCacheinfo> EdgelevelVictimMetadata::getVictimCacheinfos() const
    {
        checkValidity_();

        return victim_cacheinfos_;
    }

    const std::list<VictimCacheinfo>& EdgelevelVictimMetadata::getVictimCacheinfosRef() const
    {
        checkValidity_();

        return victim_cacheinfos_;
    }

    void EdgelevelVictimMetadata::findVictimsForObjectSize(const uint32_t& cur_edge_idx, const ObjectSize& object_size, std::unordered_map<Key, Edgeset, KeyHasher>& pervictim_edgeset, std::unordered_map<Key, std::list<VictimCacheinfo>, KeyHasher>& pervictim_cacheinfos, std::unordered_map<uint32_t, std::list<Key>>& peredge_synced_victimset, std::unordered_map<uint32_t, std::list<Key>>& peredge_fetched_victimset, Edgeset& victim_fetch_edgeset, const std::list<VictimCacheinfo>& extra_victim_cacheinfos) const
    {
        checkValidity_();

        // NOTE: NO need to clear pervictim_edgeset, pervictim_cacheinfos, peredge_synced_victimset, and peredge_fetched_victimset, which has been done by VictimTracker::findVictimsForPlacement_()

        // NOTE: NOT check seqnums here, as placement calculation allows temporarily inconsistent victim information

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
                assert(tmp_victim_cacheinfo.isComplete()); // NOTE: victim cacheinfos in edge-level victim metadata of victim tracker MUST be complete
                const Key& tmp_victim_key = tmp_victim_cacheinfo.getKey();

                // Update per-victim edgeset
                updatePervictimEdgeset_(pervictim_edgeset, tmp_victim_key, cur_edge_idx);

                // Update per-victim cacheinfos
                updatePervictimCacheinfos_(pervictim_cacheinfos, tmp_victim_key, tmp_victim_cacheinfo);

                // Update per-edge victim keyset
                updatePeredgeVictimset_(peredge_synced_victimset, cur_edge_idx, tmp_victim_key);

                // Check if we have found sufficient victims for the required bytes
                ObjectSize tmp_victim_object_size = 0;
                bool with_complete_victim_object_size = tmp_victim_cacheinfo.getObjectSize(tmp_victim_object_size);
                assert(with_complete_victim_object_size == true); // NOTE: victim cacheinfo in victim_cacheinfos_ MUST be complete
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
                std::unordered_map<uint32_t, std::list<Key>>::const_iterator peredge_synced_victimset_const_iter = peredge_synced_victimset.find(cur_edge_idx);
                for (std::list<VictimCacheinfo>::const_iterator extra_victim_cacheinfos_const_iter = extra_victim_cacheinfos.begin(); extra_victim_cacheinfos_const_iter != extra_victim_cacheinfos.end(); extra_victim_cacheinfos_const_iter++)
                {
                    assert(extra_victim_cacheinfos_const_iter->isComplete()); // NOTE: victim cacheinfos of extra fetched victims MUST be complete
                    const Key tmp_victim_key = extra_victim_cacheinfos_const_iter->getKey();

                    // Skip the extra victim that has already been considered by the current edge idx
                    if (peredge_synced_victimset_const_iter != peredge_synced_victimset.end())
                    {
                        // Check if the extra victim is already found from current edge-level victim metadata
                        bool victim_is_found = false;
                        const std::list<Key>& synced_victimset_ref = peredge_synced_victimset_const_iter->second;
                        for (std::list<Key>::const_iterator tmp_victim_const_iter = synced_victimset_ref.begin(); tmp_victim_const_iter != synced_victimset_ref.end(); tmp_victim_const_iter++)
                        {
                            if (*tmp_victim_const_iter == tmp_victim_key)
                            {
                                victim_is_found = true;
                                break;
                            }
                        }

                        if (victim_is_found)
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
                    ObjectSize tmp_victim_object_size = 0;
                    bool with_complete_victim_object_size = extra_victim_cacheinfos_const_iter->getObjectSize(tmp_victim_object_size);
                    assert(with_complete_victim_object_size); // NOTE: victim cacheinfo in extra_victim_cacheinfos MUST be complete
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

    bool EdgelevelVictimMetadata::removeVictimsForPlacement(const std::list<Key>& victim_keyset, uint64_t& removed_cacheinfos_size)
    {
        checkValidity_();

        // NOTE: victim removal after placement calculation does NOT affect seqnums

        // Remove victims from victim_cacheinfos_
        removed_cacheinfos_size = 0;
        for (std::list<Key>::const_iterator victim_keyset_const_iter = victim_keyset.begin(); victim_keyset_const_iter != victim_keyset.end(); victim_keyset_const_iter++)
        {
            bool found_victim = false;
            for (std::list<VictimCacheinfo>::iterator cacheinfo_list_iter = victim_cacheinfos_.begin(); cacheinfo_list_iter != victim_cacheinfos_.end(); cacheinfo_list_iter++)
            {
                if (cacheinfo_list_iter->getKey() == *victim_keyset_const_iter) // Find the correpsonding victim cacheinfo
                {
                    removed_cacheinfos_size += cacheinfo_list_iter->getSizeForCapacity();

                    victim_cacheinfos_.erase(cacheinfo_list_iter);
                    found_victim = true;
                    break;
                }
            }

            // NOTE: although victim keyset is generated by victim tracker based on victim_cacheinfos_ during placement calculation, victim cacheinfos could be removed by victim synchroanization of other messages before victim removal during cache placement, so victim may NOT be found during victim removal
            if (found_victim == false)
            {
                std::ostringstream oss;
                oss << "removeVictimsForPlacement() cannot find victims, which may already be removed by victim synchronization after placement calculation yet before victim removal";
                Util::dumpInfoMsg(kClassName, oss.str());
            }

            // (OBSOLETE) NOTE: as victim keyset is generated by victim tracker based on victim_cacheinfos_, each synced victim MUST have the corresponding victim cacheinfo
            //assert(found_victim == true);
        }

        // NOTE: even if all victim cacheinfos are removed, we still keep the EdgeLevelVictimMetadata for placement calculation (see VictimTracker::findVictimsForPlacement_())
        
        bool is_empty = (victim_cacheinfos_.size() == 0);
        return is_empty;
    }

    const EdgelevelVictimMetadata& EdgelevelVictimMetadata::operator=(const EdgelevelVictimMetadata& other)
    {
        if (this != &other)
        {
            is_valid_ = other.is_valid_;
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

    void EdgelevelVictimMetadata::updatePeredgeVictimset_(std::unordered_map<uint32_t, std::list<Key>>& peredge_victimset, const uint32_t& edge_idx, const Key& victim_key) const
    {
        std::unordered_map<uint32_t, std::list<Key>>::iterator peredge_victimset_iter = peredge_victimset.find(edge_idx);
        if (peredge_victimset_iter == peredge_victimset.end())
        {
            peredge_victimset_iter = peredge_victimset.insert(std::pair(edge_idx, std::list<Key>())).first;
        }
        assert(peredge_victimset_iter != peredge_victimset.end());
        peredge_victimset_iter->second.push_back(victim_key);
        return;
    }

    void EdgelevelVictimMetadata::checkValidity_() const
    {
        assert(is_valid_);
        return;
    }
}
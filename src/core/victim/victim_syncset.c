#include "core/victim/victim_syncset.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string VictimSyncset::kClassName = "VictimSyncset";

    const uint8_t VictimSyncset::INVALID_BITMAP = 0b11111111;
    const uint8_t VictimSyncset::COMPLETE_BITMAP = 0b00000000;
    const uint8_t VictimSyncset::COMPRESS_MASK = 0b00000001;
    const uint8_t VictimSyncset::CACHE_MARGIN_BYTES_DELTA_MASK = 0b00000010 | COMPRESS_MASK;
    const uint8_t VictimSyncset::LOCAL_SYNCED_VICTIMS_DEDUP_MASK = 0b00000100 | COMPRESS_MASK;
    const uint8_t VictimSyncset::LOCAL_SYNCED_VICTIMS_EMPTY_MASK = 0b00001000;
    const uint8_t VictimSyncset::LOCAL_BEACONED_VICTIMS_DEDUP_MASK = 0b00010000 | COMPRESS_MASK;
    const uint8_t VictimSyncset::LOCAL_BEACONED_VICTIMS_EMPTY_MASK = 0b00100000;

    VictimSyncset VictimSyncset::compress(const VictimSyncset& current_victim_syncset, const VictimSyncset& prev_victim_syncset)
    {
        // Perform dedup/delta-compression based on two complete victim syncsets
        assert(current_victim_syncset.isComplete());
        assert(prev_victim_syncset.isComplete());

        VictimSyncset compressed_victim_syncset = current_victim_syncset;

        // (1) Perform delta compression on cache margin bytes

        int cache_margin_delta_bytes = 0; // Compressed cache margin bytes
        bool with_cache_margin_bytes_complete = true;

        // Calculate delta of cache margin bytes
        uint64_t current_cache_margin_bytes = current_victim_syncset.cache_margin_bytes_;
        uint64_t prev_cache_margin_bytes = prev_victim_syncset.cache_margin_bytes_;
        if (current_cache_margin_bytes >= prev_cache_margin_bytes)
        {
            uint64_t tmp_delta = current_cache_margin_bytes - prev_cache_margin_bytes;
            uint64_t max_int = static_cast<uint64_t>(std::numeric_limits<int>::max());
            if (tmp_delta <= max_int) // Within valid range to be delta-compressed
            {
                cache_margin_delta_bytes = static_cast<int>(tmp_delta);
                with_cache_margin_bytes_complete = false;
            }
        }
        else
        {
            uint64_t tmp_delta_abs = prev_cache_margin_bytes - current_cache_margin_bytes;
            uint64_t min_int_abs = static_cast<uint64_t>(-1 * std::numeric_limits<int>::min());
            if (tmp_delta_abs <= min_int_abs) // Within valid range to be delta-compressed
            {
                cache_margin_delta_bytes = -1 * static_cast<int>(tmp_delta_abs);
                with_cache_margin_bytes_complete = false;
            }
        }

        // Set cache margin delta bytes if necessary
        if (!with_cache_margin_bytes_complete)
        {
            compressed_victim_syncset.setCacheMarginDeltaBytes(cache_margin_delta_bytes);
        }

        // (2) Perform dedup on victim cacheinfos

        std::list<VictimCacheinfo> final_local_synced_victims; // Complete/compressed victim cacheinfos
        bool with_complete_local_synced_victims = true;
        uint32_t total_payload_size_for_current_local_synced_victims = 0;
        uint32_t total_payload_size_for_final_local_synced_victims = 0;

        // Get complete/deduped victim cacheinfos
        std::unordered_map<Key, VictimCacheinfo, KeyHasher> current_local_synced_victims_map;
        const bool current_with_complete_local_synced_victims = current_victim_syncset.getLocalSyncedVictimsAsMap(current_local_synced_victims_map);
        assert(current_with_complete_local_synced_victims == true);
        std::unordered_map<Key, VictimCacheinfo, KeyHasher> prev_local_synced_victims_map;
        const bool prev_with_complete_local_synced_victims = prev_victim_syncset.getLocalSyncedVictimsAsMap(prev_local_synced_victims_map);
        assert(prev_with_complete_local_synced_victims == true);
        for (std::unordered_map<Key, VictimCacheinfo, KeyHasher>::const_iterator current_local_synced_victims_map_const_iter = current_local_synced_victims_map.begin(); current_local_synced_victims_map_const_iter != current_local_synced_victims_map.end(); current_local_synced_victims_map_const_iter++)
        {
            const Key& tmp_current_victim_key = current_local_synced_victims_map_const_iter->first;
            const VictimCacheinfo& tmp_current_victim_cacheinfo = current_local_synced_victims_map_const_iter->second;
            total_payload_size_for_current_local_synced_victims += tmp_current_victim_cacheinfo.getVictimCacheinfoPayloadSize();

            std::unordered_map<Key, VictimCacheinfo, KeyHasher>::const_iterator tmp_prev_local_synced_victims_map_const_iter = prev_local_synced_victims_map.find(tmp_current_victim_key);
            if (tmp_prev_local_synced_victims_map_const_iter == prev_local_synced_victims_map.end()) // No previous victim cacheinfo for the given key
            {
                // Add current complete victim cacheinfo
                assert(tmp_current_victim_cacheinfo.isComplete());
                final_local_synced_victims.push_back(tmp_current_victim_cacheinfo);

                total_payload_size_for_final_local_synced_victims += tmp_current_victim_cacheinfo.getVictimCacheinfoPayloadSize();
            }
            else // Previous victim cacheinfo exists for the given key
            {
                // Add final complete/deduped victim cacheinfo
                const VictimCacheinfo& tmp_prev_victim_cacheinfo = tmp_prev_local_synced_victims_map_const_iter->second;
                VictimCacheinfo tmp_final_victim_cacheinfo = VictimCacheinfo::dedup(tmp_current_victim_cacheinfo, tmp_prev_victim_cacheinfo);
                if (!tmp_final_victim_cacheinfo.isFullyDeduped()) // NOTE: NO need to transmit fully-deduped victim cacheinfo (i.e., w/o any change vs. prev cacheinfo)
                {
                    final_local_synced_victims.push_back(tmp_final_victim_cacheinfo);

                    total_payload_size_for_final_local_synced_victims += tmp_final_victim_cacheinfo.getVictimCacheinfoPayloadSize();
                }
            }
        }

        // Get stale victim cacheinfos
        for (std::unordered_map<Key, VictimCacheinfo, KeyHasher>::const_iterator prev_local_synced_victims_map_const_iter = prev_local_synced_victims_map.begin(); prev_local_synced_victims_map_const_iter != prev_local_synced_victims_map.end(); prev_local_synced_victims_map_const_iter++)
        {
            const Key& tmp_prev_victim_key = prev_local_synced_victims_map_const_iter->first;
            std::unordered_map<Key, VictimCacheinfo, KeyHasher>::const_iterator tmp_current_local_synced_victims_map_const_iter = current_local_synced_victims_map.find(tmp_prev_victim_key);
            if (tmp_current_local_synced_victims_map_const_iter == current_local_synced_victims_map.end()) // Prev victim cacheinfo is stale
            {
                VictimCacheinfo tmp_stale_victim_cacheinfo = prev_local_synced_victims_map_const_iter->second;
                tmp_stale_victim_cacheinfo.markStale();
                final_local_synced_victims.push_back(tmp_stale_victim_cacheinfo);

                total_payload_size_for_final_local_synced_victims += tmp_stale_victim_cacheinfo.getVictimCacheinfoPayloadSize();
            }
        }

        // Set local synced victims for compression if necessary
        with_complete_local_synced_victims = (total_payload_size_for_current_local_synced_victims <= total_payload_size_for_final_local_synced_victims);
        if (!with_complete_local_synced_victims)
        {
            compressed_victim_syncset.setLocalSyncedVictimsForCompress(final_local_synced_victims);
        }

        // (3) Perform delta compression on victim dirinfo sets

        std::unordered_map<Key, DirinfoSet, KeyHasher> final_local_beaconed_victims; // Complete/compressed victim dirinfo sets
        bool with_complete_local_beaconed_victims = true;
        uint32_t total_payload_size_for_current_local_beaconed_victims = 0;
        uint32_t total_payload_size_for_final_local_beaconed_victims = 0;

        // Get complete/delta-compressed victim dirinfo sets
        std::unordered_map<Key, DirinfoSet, KeyHasher> current_local_beaconed_victims = current_victim_syncset.local_beaconed_victims_;
        const bool current_with_complete_local_beaconed_victims = current_victim_syncset.getLocalBeaconedVictims(current_local_beaconed_victims);
        assert(current_with_complete_local_beaconed_victims == true);
        std::unordered_map<Key, DirinfoSet, KeyHasher> prev_local_beaconed_victims = prev_victim_syncset.local_beaconed_victims_;
        const bool prev_with_complete_local_beaconed_victims = prev_victim_syncset.getLocalBeaconedVictims(prev_local_beaconed_victims);
        assert(prev_with_complete_local_beaconed_victims == true);
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator current_local_beaconed_victims_const_iter = current_local_beaconed_victims.begin(); current_local_beaconed_victims_const_iter != current_local_beaconed_victims.end(); current_local_beaconed_victims_const_iter++)
        {
            const DirinfoSet& tmp_current_dirinfo_set = current_local_beaconed_victims_const_iter->second;
            total_payload_size_for_current_local_beaconed_victims += tmp_current_dirinfo_set.getDirinfoSetPayloadSize(); // NOTE: NOT count key size as both complete and compressed dirinfo set have the same key

            const Key& tmp_victim_key = current_local_beaconed_victims_const_iter->first;
            std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator tmp_prev_local_beaconed_victims_const_iter = prev_local_beaconed_victims.find(tmp_victim_key);
            if (tmp_prev_local_beaconed_victims_const_iter == prev_local_beaconed_victims.end()) // No previous victim dirinfo set
            {
                // Add current complete victim dirinfo set
                assert(tmp_current_dirinfo_set.isComplete());
                final_local_beaconed_victims.insert(std::pair(tmp_victim_key, tmp_current_dirinfo_set));

                total_payload_size_for_final_local_beaconed_victims += tmp_current_dirinfo_set.getDirinfoSetPayloadSize(); // NOTE: NOT count key size as both complete and compressed dirinfo set have the same key
            }
            else // Previous victim dirinfo set exists for the given key
            {
                // Add final complete/delta-compressed victim dirinfo set
                const DirinfoSet& tmp_prev_dirinfo_set = tmp_prev_local_beaconed_victims_const_iter->second;
                const DirinfoSet tmp_final_dirinfo_set = DirinfoSet::compress(tmp_current_dirinfo_set, tmp_prev_dirinfo_set);
                if (tmp_final_dirinfo_set.isFullyCompressed()) // NOTE: NO need to transmit fully-compressed victim dirinfo set (i.e., w/o any change vs. prev dirinfo set)
                {
                    final_local_beaconed_victims.insert(std::pair(tmp_victim_key, tmp_final_dirinfo_set));

                    total_payload_size_for_final_local_beaconed_victims += tmp_final_dirinfo_set.getDirinfoSetPayloadSize(); // NOTE: NOT count key size as both complete and compressed dirinfo set have the same key
                }
            }
        }

        // NOTE: we do NOT need to track stale victim dirinfo sets of local-/neighbor-unsynced keys, as they will be removed based on refcnts automatically

        // Set local beaconed victims for compression if necessary
        with_complete_local_beaconed_victims = (total_payload_size_for_current_local_beaconed_victims <= total_payload_size_for_final_local_beaconed_victims);
        if (!with_complete_local_beaconed_victims)
        {
            compressed_victim_syncset.setLocalBeaconedVictimsForCompress(final_local_beaconed_victims);
        }

        // (4) Get final victim syncset
        const uint32_t compressed_victim_syncset_payload_size = compressed_victim_syncset.getVictimSyncsetPayloadSize();
        const uint32_t current_victim_syncset_payload_size = current_victim_syncset.getVictimSyncsetPayloadSize();
        if (compressed_victim_syncset_payload_size < current_victim_syncset_payload_size)
        {
            #ifdef DEBUG_VICTIM_SYNCSET
                Util::dumpVariablesForDebug(kClassName, 5, "use compressed victim syncset!", "compressed_victim_syncset_payload_size:", std::to_string(compressed_victim_syncset_payload_size).c_str(), "current_victim_syncset_payload_size:", std::to_string(current_victim_syncset_payload_size).c_str());
            #endif
            return compressed_victim_syncset;
        }
        else
        {
            #ifdef DEBUG_VICTIM_SYNCSET
                Util::dumpVariablesForDebug(kClassName, 5, "use complete victim syncset!", "compressed_victim_syncset_payload_size:", std::to_string(compressed_victim_syncset_payload_size).c_str(), "current_victim_syncset_payload_size:", std::to_string(current_victim_syncset_payload_size).c_str());
            #endif
            return current_victim_syncset;
        }
    }

    VictimSyncset::VictimSyncset() : compressed_bitmap_(INVALID_BITMAP), cache_margin_bytes_(0), cache_margin_delta_bytes_(0)
    {
        local_synced_victims_.clear();
        local_beaconed_victims_.clear();
    }

    VictimSyncset::VictimSyncset(const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims) : cache_margin_bytes_(cache_margin_bytes)
    {
        compressed_bitmap_ = COMPLETE_BITMAP;
        cache_margin_delta_bytes_ = 0;

        local_synced_victims_ = local_synced_victims;
        if (local_synced_victims.size() == 0)
        {
            compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
        }

        local_beaconed_victims_ = local_beaconed_victims;
        if (local_beaconed_victims.size() == 0)
        {
            compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
        }

        assertAllCacheinfosComplete_();
        assertAllDirinfoSetsComplete_();
    }

    VictimSyncset::~VictimSyncset() {}

    bool VictimSyncset::isComplete() const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        return (compressed_bitmap_ & COMPRESS_MASK == COMPLETE_BITMAP & COMPRESS_MASK);
    }

    // For both complete and compressed victim syncsets

    bool VictimSyncset::getCacheMarginBytesOrDelta(uint64_t& cache_margin_bytes, int& cache_margin_delta_bytes) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        if (!with_complete_cache_margin_bytes)
        {
            cache_margin_delta_bytes = cache_margin_delta_bytes_;
        }
        else
        {
            cache_margin_bytes = cache_margin_bytes_;
        }

        return with_complete_cache_margin_bytes;
    }

    bool VictimSyncset::getLocalSyncedVictims(std::list<VictimCacheinfo>& local_synced_vitims) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) != LOCAL_SYNCED_VICTIMS_DEDUP_MASK);

        bool is_empty = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        if (is_empty)
        {
            local_synced_vitims.clear();
        }
        else
        {
            if (!with_complete_local_synced_victims) // At least one victim cacheinfo is deduped
            {
                assertAtLeastOneCacheinfoDeduped_();
            }
            else // All victim cacheinfos should be complete
            {
                assertAllCacheinfosComplete_();
            }

            local_synced_vitims = local_synced_victims_;
        }

        return with_complete_local_synced_victims;
    }

    bool VictimSyncset::getLocalSyncedVictimsAsMap(std::unordered_map<Key,VictimCacheinfo, KeyHasher>& local_synced_vitims_map) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        std::list<VictimCacheinfo> tmp_local_synced_vitims;
        bool with_complete_local_synced_victims = getLocalSyncedVictims(tmp_local_synced_vitims);

        local_synced_vitims_map.clear();
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = tmp_local_synced_vitims.begin(); cacheinfo_list_const_iter != tmp_local_synced_vitims.end(); cacheinfo_list_const_iter++)
        {
            local_synced_vitims_map.insert(std::pair(cacheinfo_list_const_iter->getKey(), *cacheinfo_list_const_iter));
        }

        return with_complete_local_synced_victims;
    }

    bool VictimSyncset::getLocalBeaconedVictims(std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims) const
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        bool with_complete_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) != LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        bool is_empty = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);

        if (is_empty)
        {
            local_beaconed_victims.clear();
        }
        else
        {
            if (!with_complete_local_beaconed_victims) // At least one dirinfo set is deduped
            {
                assertAtLeastOneDirinfoSetCompressed_();
            }
            else // All dirinfo sets should be complete
            {
                assertAllDirinfoSetsComplete_();
            }

            local_beaconed_victims = local_beaconed_victims_;
        }

        return with_complete_local_beaconed_victims;
    }

    // For complete victim syncset

    /*void VictimSyncset::setCacheMarginBytes(const uint64_t& cache_margin_bytes)
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        compressed_bitmap_ &= (~(CACHE_MARGIN_BYTES_DELTA_MASK & ~COMPRESS_MASK)); // Remove 2nd lowest bit for complete cache margin bytes
        if (((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) != LOCAL_SYNCED_VICTIMS_DEDUP_MASK) && ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) != LOCAL_BEACONED_VICTIMS_DEDUP_MASK)) // NOT a compressed victim syncset yet
        {
            compressed_bitmap_ &= ~COMPRESS_MASK; // Remove 1st lowest bit for complete victim syncset
        }
        cache_margin_bytes_ = cache_margin_bytes;
        cache_margin_delta_bytes_ = 0;
        return;
    }*/

    // For compressed victim syncset

    void VictimSyncset::setCacheMarginDeltaBytes(const int& cache_margin_delta_bytes)
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        compressed_bitmap_ |= CACHE_MARGIN_BYTES_DELTA_MASK;
        cache_margin_delta_bytes_ = cache_margin_delta_bytes;
        cache_margin_bytes_ = 0;
        return;
    }

    void VictimSyncset::setLocalSyncedVictimsForCompress(const std::list<VictimCacheinfo>& local_synced_victims)
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_DEDUP_MASK;
        local_synced_victims_ = local_synced_victims;

        if (local_synced_victims.size() == 0)
        {
            compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
        }

        assertAtLeastOneCacheinfoDeduped_();
        return;
    }

    void VictimSyncset::setLocalBeaconedVictimsForCompress(const std::unordered_map<Key, DirinfoSet, KeyHasher>& local_beaconed_victims)
    {
        assert(compressed_bitmap_ != INVALID_BITMAP);

        compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_DEDUP_MASK;
        local_beaconed_victims_ = local_beaconed_victims;

        if (local_beaconed_victims.size() == 0)
        {
            compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
        }

        assertAtLeastOneDirinfoSetCompressed_();
        return;
    }

    uint32_t VictimSyncset::getVictimSyncsetPayloadSize() const
    {
        // TODO: we can tune the sizes of local synched/beaconed victims, as the numbers are limited under our design (at most peredge_synced_victimcnt and peredge_synced_victimcnt * edgecnt)

        uint32_t victim_syncset_payload_size = 0;

        // Compressed bitmap
        victim_syncset_payload_size += sizeof(uint8_t);

        // Cache margin bytes
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        if (with_complete_cache_margin_bytes)
        {
            victim_syncset_payload_size += sizeof(uint64_t);
        }
        else
        {
            victim_syncset_payload_size += sizeof(int);
        }

        // Local synced victims
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_synced_victims) // Non-empty local synced victims
        {
            assert(local_synced_victims_.size() > 0);

            victim_syncset_payload_size += sizeof(uint32_t); // Size of local_synced_victims_
            for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); cacheinfo_list_iter++)
            {
                victim_syncset_payload_size += cacheinfo_list_iter->getVictimCacheinfoPayloadSize(); // Complete or compressed
            }
        }

        // Local beaconed victims
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            assert(local_beaconed_victims_.size() > 0);

            victim_syncset_payload_size += sizeof(uint32_t); // Size of local_beaconed_victims_
            for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); dirinfo_map_iter++)
            {
                victim_syncset_payload_size += dirinfo_map_iter->first.getKeyPayloadSize();

                // Dirinfo set for the given key
                const DirinfoSet& tmp_dirinfo_set = dirinfo_map_iter->second;
                victim_syncset_payload_size += tmp_dirinfo_set.getDirinfoSetPayloadSize(); // Complete or compressed
            }
        }

        return victim_syncset_payload_size;
    }

    uint32_t VictimSyncset::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        uint32_t size = position;

        // Compressed bitmap
        msg_payload.serialize(size, (char*)&compressed_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);

        // Cache margin bytes
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        if (with_complete_cache_margin_bytes)
        {
            msg_payload.deserialize(size, (const char*)&cache_margin_bytes_, sizeof(uint64_t));
            size += sizeof(uint64_t);
        }
        else
        {
            msg_payload.deserialize(size, (const char*)&cache_margin_delta_bytes_, sizeof(int));
            size += sizeof(int);
        }

        // Size of local synced victims
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_synced_victims) // Non-empty local synced victims
        {
            uint32_t local_synced_victims_size = local_synced_victims_.size();
            msg_payload.deserialize(size, (const char*)&local_synced_victims_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Local synced victims
            for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); cacheinfo_list_iter++)
            {
                uint32_t cacheinfo_serialize_size = cacheinfo_list_iter->serialize(msg_payload, size); // Complete or compressed
                size += cacheinfo_serialize_size;
            }
        }

        // Size of local beaconed victims
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            uint32_t local_beaconed_victims_size = local_beaconed_victims_.size();
            msg_payload.deserialize(size, (const char*)&local_beaconed_victims_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Local beaconed victims
            for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_iter = local_beaconed_victims_.begin(); dirinfo_map_iter != local_beaconed_victims_.end(); dirinfo_map_iter++)
            {
                uint32_t key_serialize_size = dirinfo_map_iter->first.serialize(msg_payload, size);
                size += key_serialize_size;

                // Dirinfo set for the given key
                const DirinfoSet& tmp_dirinfo_set = dirinfo_map_iter->second;
                uint32_t dirinfo_set_serialize_size = tmp_dirinfo_set.serialize(msg_payload, size); // Complete or compressed
                size += dirinfo_set_serialize_size;
            }
        }

        return size - position;
    }

    uint32_t VictimSyncset::deserialize(const DynamicArray& msg_payload, const uint32_t& position)
    {
        uint32_t size = position;

        // Compressed bitmap
        msg_payload.serialize(size, (char*)&compressed_bitmap_, sizeof(uint8_t));
        size += sizeof(uint8_t);

        // Cache margin bytes
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        if (with_complete_cache_margin_bytes)
        {
            msg_payload.serialize(size, (char*)&cache_margin_bytes_, sizeof(uint64_t));
            size += sizeof(uint64_t);
        }
        else
        {
            msg_payload.serialize(size, (char*)&cache_margin_delta_bytes_, sizeof(int));
            size += sizeof(int);
        }

        // Size of local synced victims
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_synced_victims) // Non-empty local synced victims
        {
            uint32_t local_synced_victims_size = 0;
            msg_payload.serialize(size, (char*)&local_synced_victims_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Local synced victims
            local_synced_victims_.clear();
            for (uint32_t i = 0; i < local_synced_victims_size; i++)
            {
                VictimCacheinfo cacheinfo;
                uint32_t cacheinfo_deserialize_size = cacheinfo.deserialize(msg_payload, size); // Complete or compressed
                size += cacheinfo_deserialize_size;
                local_synced_victims_.push_back(cacheinfo);
            }
        }

        // Size of local beaconed victims
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            uint32_t local_beaconed_victims_size = 0;
            msg_payload.serialize(size, (char*)&local_beaconed_victims_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Local beaconed victims
            local_beaconed_victims_.clear();
            for (uint32_t i = 0; i < local_beaconed_victims_size; i++)
            {
                Key key;
                uint32_t key_deserialize_size = key.deserialize(msg_payload, size);
                size += key_deserialize_size;

                // Dirinfo set for the given key
                DirinfoSet tmp_dirinfo_set;
                uint32_t dirinfo_set_deserialize_size = tmp_dirinfo_set.deserialize(msg_payload, size); // Complete or compressed
                size += dirinfo_set_deserialize_size;

                local_beaconed_victims_.insert(std::pair(key, tmp_dirinfo_set));
            }
        }

        return size - position;
    }

    const VictimSyncset& VictimSyncset::operator=(const VictimSyncset& other)
    {
        compressed_bitmap_ = other.compressed_bitmap_;
        cache_margin_bytes_ = other.cache_margin_bytes_;
        cache_margin_delta_bytes_ = other.cache_margin_delta_bytes_;
        local_synced_victims_ = other.local_synced_victims_;
        local_beaconed_victims_ = other.local_beaconed_victims_;
        
        return *this;
    }

    void VictimSyncset::assertAtLeastOneCacheinfoDeduped_() const
    {
        bool tmp_at_least_one_dedup = false;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = local_synced_victims_.begin(); cacheinfo_list_const_iter != local_synced_victims_.end(); cacheinfo_list_const_iter++)
        {
            if (cacheinfo_list_const_iter->isDeduped())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);

        return;
    }

    void VictimSyncset::assertAllCacheinfosComplete_() const
    {
        bool tmp_all_complete = true;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = local_synced_victims_.begin(); cacheinfo_list_const_iter != local_synced_victims_.end(); cacheinfo_list_const_iter++)
        {
            if (!cacheinfo_list_const_iter->isComplete())
            {
                tmp_all_complete = false;
                break;
            }
        }
        assert(tmp_all_complete == true);

        return;
    }

    void VictimSyncset::assertAtLeastOneDirinfoSetCompressed_() const
    {
        bool tmp_at_least_one_dedup = false;
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_const_iter = local_beaconed_victims_.begin(); dirinfo_map_const_iter != local_beaconed_victims_.end(); dirinfo_map_const_iter++)
        {
            if (dirinfo_map_const_iter->second.isCompressed())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);

        return;
    }

    void VictimSyncset::assertAllDirinfoSetsComplete_() const
    {
        bool tmp_all_complete = true;
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_const_iter = local_beaconed_victims_.begin(); dirinfo_map_const_iter != local_beaconed_victims_.end(); dirinfo_map_const_iter++)
        {
            if (!dirinfo_map_const_iter->second.isComplete())
            {
                tmp_all_complete = false;
                break;
            }
        }
        assert(tmp_all_complete == true);

        return;
    }
}
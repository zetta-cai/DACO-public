#include "core/victim/victim_syncset.h"

#include <assert.h>

#include "common/kv_list_helper_impl.h"
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
    const uint8_t VictimSyncset::FULLY_COMPRESSED_BITMAP = 0b00111111;

    VictimSyncset VictimSyncset::compress(const VictimSyncset& current_victim_syncset, const VictimSyncset& prev_victim_syncset)
    {
        // Perform dedup/delta-compression based on two complete victim syncsets
        assert(current_victim_syncset.isComplete());
        assert(prev_victim_syncset.isComplete());

        // NOTE: dedup-/delta-based victim syncset compression MUST follow strict seqnum order
        assert(current_victim_syncset.getSeqnum() == Util::uint64Add(prev_victim_syncset.getSeqnum(), 1)); // Current seqnum MUST be prev seqnum + 1

        VictimSyncset compressed_victim_syncset = current_victim_syncset; // Use seqnum and is_enforce_complete of current victim syncset

        // (1) Perform delta compression on cache margin bytes

        #ifndef DISABLE_CACHE_MARGIN_BYTES_DELTA
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
            // NOTE: directly using (-1 * std::numeric_limits<int>::min()) will incur integer overflow as std::numeric_limits<int>::max() = -1 * std::numeric_limits<int>::min() - 1
            uint64_t min_int_abs = static_cast<uint64_t>(-1 * static_cast<int64_t>(std::numeric_limits<int>::min()));
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
        #endif

        // (2) Perform dedup on victim cacheinfos

        std::list<VictimCacheinfo> final_local_synced_victims; // Complete/compressed victim cacheinfos
        bool with_complete_local_synced_victims = true;
        uint32_t total_payload_size_for_current_local_synced_victims = 0;
        uint32_t total_payload_size_for_final_local_synced_victims = 0;

        // Get complete/deduped victim cacheinfos
        bool current_with_complete_local_synced_victims = false;
        const std::list<VictimCacheinfo>* current_local_synced_victim_cacheinfos_ptr = current_victim_syncset.getLocalSyncedVictimsPtr(current_with_complete_local_synced_victims);
        assert(current_with_complete_local_synced_victims == true);
        bool prev_with_complete_local_synced_victims = false;
        const std::list<VictimCacheinfo>* prev_local_synced_victim_cacheinfos_ptr;
        prev_local_synced_victim_cacheinfos_ptr = prev_victim_syncset.getLocalSyncedVictimsPtr(prev_with_complete_local_synced_victims);
        assert(prev_with_complete_local_synced_victims == true);

        for (std::list<VictimCacheinfo>::const_iterator current_local_synced_victim_cacheinfos_const_iter = current_local_synced_victim_cacheinfos_ptr->begin(); current_local_synced_victim_cacheinfos_const_iter != current_local_synced_victim_cacheinfos_ptr->end(); current_local_synced_victim_cacheinfos_const_iter++)
        {
            const VictimCacheinfo& tmp_current_victim_cacheinfo = *current_local_synced_victim_cacheinfos_const_iter;
            const Key& tmp_current_victim_key = tmp_current_victim_cacheinfo.getKey();
            total_payload_size_for_current_local_synced_victims += tmp_current_victim_cacheinfo.getVictimCacheinfoPayloadSize();

            std::list<VictimCacheinfo>::const_iterator tmp_prev_local_synced_victim_cacheinfos_const_iter = VictimCacheinfo::findVictimCacheinfoForKey(tmp_current_victim_key, *prev_local_synced_victim_cacheinfos_ptr);
            if (tmp_prev_local_synced_victim_cacheinfos_const_iter == prev_local_synced_victim_cacheinfos_ptr->end()) // No previous victim cacheinfo for the given key
            {
                // Add current complete victim cacheinfo
                assert(tmp_current_victim_cacheinfo.isComplete());
                final_local_synced_victims.push_back(tmp_current_victim_cacheinfo);

                total_payload_size_for_final_local_synced_victims += tmp_current_victim_cacheinfo.getVictimCacheinfoPayloadSize();
            }
            else // Previous victim cacheinfo exists for the given key
            {
                // Add final complete/deduped victim cacheinfo
                const VictimCacheinfo& tmp_prev_victim_cacheinfo = *tmp_prev_local_synced_victim_cacheinfos_const_iter;
                VictimCacheinfo tmp_final_victim_cacheinfo = VictimCacheinfo::dedup(tmp_current_victim_cacheinfo, tmp_prev_victim_cacheinfo);
                if (!tmp_final_victim_cacheinfo.isFullyDeduped()) // NOTE: NO need to transmit fully-deduped victim cacheinfo (i.e., w/o any change vs. prev cacheinfo)
                {
                    final_local_synced_victims.push_back(tmp_final_victim_cacheinfo);

                    total_payload_size_for_final_local_synced_victims += tmp_final_victim_cacheinfo.getVictimCacheinfoPayloadSize();
                }
            }
        }

        // Get stale victim cacheinfos
        for (std::list<VictimCacheinfo>::const_iterator prev_local_synced_victim_cacheinfos_const_iter = prev_local_synced_victim_cacheinfos_ptr->begin(); prev_local_synced_victim_cacheinfos_const_iter != prev_local_synced_victim_cacheinfos_ptr->end(); prev_local_synced_victim_cacheinfos_const_iter++)
        {
            const Key& tmp_prev_victim_key = prev_local_synced_victim_cacheinfos_const_iter->getKey();
            std::list<VictimCacheinfo>::const_iterator tmp_current_local_synced_victims_map_const_iter = VictimCacheinfo::findVictimCacheinfoForKey(tmp_prev_victim_key, *current_local_synced_victim_cacheinfos_ptr);
            if (tmp_current_local_synced_victims_map_const_iter == current_local_synced_victim_cacheinfos_ptr->end()) // Prev victim cacheinfo is stale
            {
                VictimCacheinfo tmp_stale_victim_cacheinfo = *prev_local_synced_victim_cacheinfos_const_iter;
                tmp_stale_victim_cacheinfo.markStale();
                final_local_synced_victims.push_back(tmp_stale_victim_cacheinfo);

                total_payload_size_for_final_local_synced_victims += tmp_stale_victim_cacheinfo.getVictimCacheinfoPayloadSize();
            }
        }

        // Set local synced victims for compression if necessary
        with_complete_local_synced_victims = (total_payload_size_for_current_local_synced_victims <= total_payload_size_for_final_local_synced_victims);
        if (!with_complete_local_synced_victims)
        {
            // NOTE: if with at least one compressed or fully-deduped (yet NOT in list) victim cacheinfo, NO need to sort the list of victim cacheinfos (will be sorted during VictimSyncset::recover() after receiving neighbor victim syncset)
            compressed_victim_syncset.setLocalSyncedVictimsForCompress(final_local_synced_victims);
        }

        // (3) Perform delta compression on victim dirinfo sets

        std::list<std::pair<Key, DirinfoSet>> final_local_beaconed_victims; // Complete/compressed victim dirinfo sets
        bool with_complete_local_beaconed_victims = true;
        uint32_t total_payload_size_for_current_local_beaconed_victims = 0;
        uint32_t total_payload_size_for_final_local_beaconed_victims = 0;

        // Get complete/delta-compressed victim dirinfo sets
        bool current_with_complete_local_beaconed_victims = false;
        const std::list<std::pair<Key, DirinfoSet>>* current_local_beaconed_victims_ptr = current_victim_syncset.getLocalBeaconedVictimsPtr(current_with_complete_local_beaconed_victims);
        assert(current_with_complete_local_beaconed_victims == true);
        bool prev_with_complete_local_beaconed_victims = false;
        const std::list<std::pair<Key, DirinfoSet>>* prev_local_beaconed_victims_ptr = prev_victim_syncset.getLocalBeaconedVictimsPtr(prev_with_complete_local_beaconed_victims);
        assert(prev_with_complete_local_beaconed_victims == true);
        for (std::list<std::pair<Key, DirinfoSet>>::const_iterator current_local_beaconed_victims_const_iter = current_local_beaconed_victims_ptr->begin(); current_local_beaconed_victims_const_iter != current_local_beaconed_victims_ptr->end(); current_local_beaconed_victims_const_iter++)
        {
            const DirinfoSet& tmp_current_dirinfo_set = current_local_beaconed_victims_const_iter->second;
            total_payload_size_for_current_local_beaconed_victims += tmp_current_dirinfo_set.getDirinfoSetPayloadSize(); // NOTE: NOT count key size as both complete and compressed dirinfo set have the same key

            const Key& tmp_victim_key = current_local_beaconed_victims_const_iter->first;
            std::list<std::pair<Key, DirinfoSet>>::const_iterator tmp_prev_local_beaconed_victims_const_iter = KVListHelper<Key, DirinfoSet>::findVFromListForK(tmp_victim_key, *prev_local_beaconed_victims_ptr);
            if (tmp_prev_local_beaconed_victims_const_iter == prev_local_beaconed_victims_ptr->end()) // No previous victim dirinfo set
            {
                // Add current complete victim dirinfo set
                assert(tmp_current_dirinfo_set.isComplete());
                final_local_beaconed_victims.push_back(std::pair(tmp_victim_key, tmp_current_dirinfo_set));

                total_payload_size_for_final_local_beaconed_victims += tmp_current_dirinfo_set.getDirinfoSetPayloadSize(); // NOTE: NOT count key size as both complete and compressed dirinfo set have the same key
            }
            else // Previous victim dirinfo set exists for the given key
            {
                // Add final complete/delta-compressed victim dirinfo set
                const DirinfoSet& tmp_prev_dirinfo_set = tmp_prev_local_beaconed_victims_const_iter->second;
                const DirinfoSet tmp_final_dirinfo_set = DirinfoSet::compress(tmp_current_dirinfo_set, tmp_prev_dirinfo_set);
                if (!tmp_final_dirinfo_set.isFullyCompressed()) // NOTE: NO need to transmit fully-compressed victim dirinfo set (i.e., w/o any change vs. prev dirinfo set)
                {
                    final_local_beaconed_victims.push_back(std::pair(tmp_victim_key, tmp_final_dirinfo_set));

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

            return compressed_victim_syncset; // Use seqnum and is_enforce_complete of current victim syncset
        }
        else
        {
            #ifdef DEBUG_VICTIM_SYNCSET
                Util::dumpVariablesForDebug(kClassName, 5, "use complete victim syncset!", "compressed_victim_syncset_payload_size:", std::to_string(compressed_victim_syncset_payload_size).c_str(), "current_victim_syncset_payload_size:", std::to_string(current_victim_syncset_payload_size).c_str());
            #endif

            return current_victim_syncset; // Use seqnum and is_enforce_complete of current victim syncset
        }
    }

    //VictimSyncset VictimSyncset::recover(const VictimSyncset& compressed_victim_syncset, const VictimSyncset& existing_victim_syncset)
    VictimSyncset VictimSyncset::recover(const VictimSyncset& compressed_victim_syncset, const VictimSyncset& existing_victim_syncset, const Key& key) // TMPDEBUG231211
    {
        // Recover complete victim syncset based on existing complete victim syncset of source edge idx and received compressed_victim_syncset if compressed
        assert(compressed_victim_syncset.isCompressed());
        assert(existing_victim_syncset.isComplete());

        // NOTE: dedup-/delta-based victim syncset recovery MUST follow strict seqnum order (NOTE: compressed_victim_syncset must NOT complete here)
        assert(compressed_victim_syncset.getSeqnum() == Util::uint64Add(existing_victim_syncset.getSeqnum(), 1)); // Compressed seqnum MUST be existing seqnum + 1

        VictimSyncset complete_victim_syncset = compressed_victim_syncset; // Use seqnum and is_enforce_complete of compressed victim syncset

        // (1) Recover cache margin bytes if necessary

        uint64_t synced_victim_cache_margin_bytes = 0;
        int synced_victim_cache_margin_delta_bytes = 0;
        bool with_complete_cache_margin_bytes = compressed_victim_syncset.getCacheMarginBytesOrDelta(synced_victim_cache_margin_bytes, synced_victim_cache_margin_delta_bytes);
        #ifndef DISABLE_CACHE_MARGIN_BYTES_DELTA
        if (!with_complete_cache_margin_bytes) // Recover ONLY if compressed
        {
            // Start from existing cache margin bytes
            uint64_t complete_cache_margin_bytes = 0;
            int unused_cache_margin_delta_bytes = 0;
            bool tmp_with_complete_cache_margin_bytes = existing_victim_syncset.getCacheMarginBytesOrDelta(complete_cache_margin_bytes, unused_cache_margin_delta_bytes);
            assert(tmp_with_complete_cache_margin_bytes);
            UNUSED(unused_cache_margin_delta_bytes);

            // Recover complete cache margin bytes
            if (synced_victim_cache_margin_delta_bytes >= 0)
            {
                complete_cache_margin_bytes = Util::uint64Add(complete_cache_margin_bytes, static_cast<uint64_t>(synced_victim_cache_margin_delta_bytes));
            }
            else
            {
                complete_cache_margin_bytes = Util::uint64Minus(complete_cache_margin_bytes, static_cast<uint64_t>(-1 * synced_victim_cache_margin_delta_bytes));
            }

            // Replace existing cache margin bytes with recovered ones
            complete_victim_syncset.setCacheMarginBytes(complete_cache_margin_bytes);
        } // End of !with_complete_cache_margin_bytes
        /*else // Directly use if not compressed
        {
            // Replace existing cache margin bytes with synced ones
            complete_victim_syncset.setCacheMarginBytes(synced_victim_cache_margin_bytes);
        } // End of with_complete_cache_margin_bytes*/
        #else
        assert(with_complete_cache_margin_bytes);
        #endif

        // (2) Recover complete victim cacheinfos and beacom edge indexes if necessary

        bool with_complete_synced_victim_cacheinfos = false;
        const std::list<VictimCacheinfo>* synced_victim_cacheinfos_ptr = compressed_victim_syncset.getLocalSyncedVictimsPtr(with_complete_synced_victim_cacheinfos);

        if (!with_complete_synced_victim_cacheinfos) // Recover ONLY if compressed
        {
            std::list<VictimCacheinfo> complete_victim_cacheinfos;

            // Start from existing synced victim cacheinfos
            bool tmp_with_complete_existing_victim_cacheinfos = existing_victim_syncset.getLocalSyncedVictims(complete_victim_cacheinfos);
            assert(tmp_with_complete_existing_victim_cacheinfos); // Existing victim cacheinfos MUST be complete

            // Recover complete victim cacheinfos based on existing complete victim cacheinfos and synced victim cacheinfos if compressed
            for (std::list<VictimCacheinfo>::const_iterator synced_victim_cacheinfos_const_iter = synced_victim_cacheinfos_ptr->begin(); synced_victim_cacheinfos_const_iter != synced_victim_cacheinfos_ptr->end(); synced_victim_cacheinfos_const_iter++)
            {
                const Key& tmp_synced_victim_key = synced_victim_cacheinfos_const_iter->getKey();
                const VictimCacheinfo& tmp_synced_victim_cacheinfo = *synced_victim_cacheinfos_const_iter;

                std::list<VictimCacheinfo>::iterator tmp_complete_victim_cacheinfos_list_iter = VictimCacheinfo::findVictimCacheinfoForKey(tmp_synced_victim_key, complete_victim_cacheinfos);

                if (tmp_synced_victim_cacheinfo.isComplete()) // Complete victim cacheinfo indicates a new neighbor synced victim (should not exist in existing victim cacheinfos)
                {
                    if (tmp_complete_victim_cacheinfos_list_iter == complete_victim_cacheinfos.end())
                    {
                        complete_victim_cacheinfos.push_back(tmp_synced_victim_cacheinfo);
                    }
                    else
                    {
                        // TODO: Actually complete victim cacheinfo may also exist in prev victim syncset, as VictimCacheinfo::dedup() may return a complete victim cacheinfo -> but we still warn here, as in most time VictimCacheinfo::dedup() will return deduped victim cacheinfos
                        std::ostringstream oss;
                        oss << "New complete victim cacheinfo already exists for key " << tmp_synced_victim_key.getKeystr() << " in existing complete victim cacheinfos!";
                        Util::dumpWarnMsg(kClassName, oss.str());

                        *tmp_complete_victim_cacheinfos_list_iter = tmp_synced_victim_cacheinfo;
                    }
                } // End of tmp_synced_victim_cacheinfo.isComplete()
                else if(tmp_synced_victim_cacheinfo.isStale()) // Stale victim cacheinfo (should exist in existing victim cacheinfos or already removed by victim removal)
                {
                    if (tmp_complete_victim_cacheinfos_list_iter != complete_victim_cacheinfos.end())
                    {
                        complete_victim_cacheinfos.erase(tmp_complete_victim_cacheinfos_list_iter);
                    }
                    // NOTE: NOT warn here as stale victim cacheinfo may be already removed by victim removal
                    //else {}
                } // End of tmp_synced_victim_cacheinfo.isStale()
                else // Compressed victim cacheinfo (should exist in existing victim cacheinfos or already removed by victim removal)
                {
                    // NOTE: received victim cacheinfo MUST NOT fully deduped
                    assert(!tmp_synced_victim_cacheinfo.isFullyDeduped());
                    
                    if (tmp_complete_victim_cacheinfos_list_iter != complete_victim_cacheinfos.end())
                    {
                        VictimCacheinfo tmp_complete_victim_cacheinfo = VictimCacheinfo::recover(tmp_synced_victim_cacheinfo, *tmp_complete_victim_cacheinfos_list_iter);
                        *tmp_complete_victim_cacheinfos_list_iter = tmp_complete_victim_cacheinfo;
                    }
                    // NOTE: NOT warn here as stale victim cacheinfo may be already removed by victim removal
                    //else {}
                } // End of tmp_synced_victim_cacheinfo.isCompressed()
            } // End of enumeration of synced vicitm cacheinfos

            #ifdef DEBUG_VICTIM_SYNCSET
            // std::ostringstream oss;
            // oss << "[before sort]" << std::endl;
            // uint32_t i = 0;
            // for (std::list<VictimCacheinfo>::const_iterator complete_victim_cacheinfos_const_iter = complete_victim_cacheinfos.begin(); complete_victim_cacheinfos_const_iter != complete_victim_cacheinfos.end(); complete_victim_cacheinfos_const_iter++)
            // {
            //     const VictimCacheinfo& tmp_complete_victim_cacheinfo = *complete_victim_cacheinfos_const_iter;
            //     Reward local_reward = 0.0;
            //     tmp_complete_victim_cacheinfo.getLocalReward(local_reward);
            //     oss <<"[" << i << "] key: " << tmp_complete_victim_cacheinfo.getKey().getKeystr() << " reward: " << local_reward << std::endl;
            //     i++;
            // }
            // Util::dumpDebugMsg(kClassName, oss.str());
            #endif

            // Sort by local rewards in an ascending order
            VictimCacheinfo::sortByLocalRewards(complete_victim_cacheinfos);

            #ifdef DEBUG_VICTIM_SYNCSET
            // oss.str("");
            // oss.clear();
            // oss << "[after sort]" << std::endl;
            // uint32_t j = 0;
            // for (std::list<VictimCacheinfo>::const_iterator complete_victim_cacheinfos_const_iter = complete_victim_cacheinfos.begin(); complete_victim_cacheinfos_const_iter != complete_victim_cacheinfos.end(); complete_victim_cacheinfos_const_iter++)
            // {
            //     const VictimCacheinfo& tmp_complete_victim_cacheinfo = *complete_victim_cacheinfos_const_iter;
            //     Reward local_reward = 0.0;
            //     tmp_complete_victim_cacheinfo.getLocalReward(local_reward);
            //     oss <<"[" << j << "] key: " << tmp_complete_victim_cacheinfo.getKey().getKeystr() << " reward: " << local_reward << std::endl;
            //     j++;
            // }
            // Util::dumpDebugMsg(kClassName, oss.str());
            #endif

            // Replace existing complete victim cacheinfos with recovered ones
            complete_victim_syncset.setLocalSyncedVictims(complete_victim_cacheinfos);
        } // End of !with_complete_synced_victim_cacheinfos
        /*else // Directly use if not compressed
        {
            // Replace existing complete victim cacheinfos with synced ones
            complete_victim_syncset.setLocalSyncedVictims(synced_victim_cacheinfos);
        } // End of with_complete_synced_victim_cacheinfos*/

        // (3) Recover complete victim dirinfo sets if necessary

        bool with_complete_synced_victim_dirinfo_sets = false;
        const std::list<std::pair<Key, DirinfoSet>>* synced_victim_dirinfo_sets_ptr = compressed_victim_syncset.getLocalBeaconedVictimsPtr(with_complete_synced_victim_dirinfo_sets);
        if (!with_complete_synced_victim_dirinfo_sets) // Recover ONLY if compressed
        {
            // Start from existing complete victim dirinfos sets
            std::list<std::pair<Key, DirinfoSet>> tmp_complete_victim_dirinfo_sets;
            bool tmp_with_complete_victim_dirinfo_sets = existing_victim_syncset.getLocalBeaconedVictims(tmp_complete_victim_dirinfo_sets);
            assert(tmp_with_complete_victim_dirinfo_sets);

            // Recover complete victim dirinfo sets
            for (std::list<std::pair<Key, DirinfoSet>>::const_iterator synced_victim_dirinfo_sets_const_iter = synced_victim_dirinfo_sets_ptr->begin(); synced_victim_dirinfo_sets_const_iter != synced_victim_dirinfo_sets_ptr->end(); synced_victim_dirinfo_sets_const_iter++)
            {
                const Key& tmp_synced_victim_key = synced_victim_dirinfo_sets_const_iter->first;
                const DirinfoSet& tmp_synced_dirinfo_set = synced_victim_dirinfo_sets_const_iter->second;
                std::list<std::pair<Key, DirinfoSet>>::iterator tmp_complete_victim_dirinfo_sets_iter = KVListHelper<Key, DirinfoSet>::findVFromListForK(tmp_synced_victim_key, tmp_complete_victim_dirinfo_sets);

                if (tmp_synced_dirinfo_set.isComplete()) // Complete victim dirinfo set for new neighbor beaconed victim (should NOT exist in existing dirinfo sets) or existing neighbor beaconed victim (exist in existing dirinfo sets)
                {
                    if (tmp_complete_victim_dirinfo_sets_iter == tmp_complete_victim_dirinfo_sets.end())
                    {
                        tmp_complete_victim_dirinfo_sets.push_back(std::pair(tmp_synced_victim_key, tmp_synced_dirinfo_set));
                    }
                    else
                    {
                        // NOTE: as DirinfoSet::compress() could return a complete dirinfo set, the complete dirinfo set may also exist in prev victim syncset -> we do NOT warn here, as this is NOT a minor case
                        // std::ostringstream oss;
                        // oss << "New complete victim dirinfo set already exists for key " << tmp_synced_victim_key.getKeystr() << " in existing complete victim dirinfo sets!";
                        // Util::dumpWarnMsg(kClassName, oss.str());

                        tmp_complete_victim_dirinfo_sets_iter->second = tmp_synced_dirinfo_set;
                    }
                } // End of tmp_synced_dirinfo_set.isComplete()
                else // Compressed dirinfo sets (should exist in existing dirinfo sets or already removed by victim removal)
                {
                    // NOTE: received victim dirinfo set MUST NOT fully compressed
                    assert(!tmp_synced_dirinfo_set.isFullyCompressed());

                    if (tmp_complete_victim_dirinfo_sets_iter != tmp_complete_victim_dirinfo_sets.end())
                    {
                        DirinfoSet tmp_complete_dirinfo_set = DirinfoSet::recover(tmp_synced_dirinfo_set, tmp_complete_victim_dirinfo_sets_iter->second);
                        tmp_complete_victim_dirinfo_sets_iter->second = tmp_complete_dirinfo_set;
                    }
                    // NOTE: NOT warn here as stale victim dirinfo sets may be already removed by victim removal
                    //else {}
                } // End of tmp_synced_dirinfo_set.isCompressed()
            } // End of enumeration of synced dirinfo sets

            // Replace existing complete victim dirinfo sets with recovered ones
            complete_victim_syncset.setLocalBeaconedVictims(tmp_complete_victim_dirinfo_sets);
        } // End of !with_complete_synced_victim_dirinfo_sets
        /*else // Directly use if not compressed
        {
            // Replace existing complete victim dirinfo sets with synced ones
            complete_victim_syncset.setLocalBeaconedVictims(synced_victim_dirinfo_sets);
        } // End of with_complete_synced_victim_dirinfo_sets*/

        // NOTE: use the same seqnum and is_enforce_complete as compressed victim syncset
        assert(complete_victim_syncset.isComplete());
        assert(complete_victim_syncset.getSeqnum() == compressed_victim_syncset.getSeqnum());
        assert(complete_victim_syncset.isEnforceComplete() == compressed_victim_syncset.isEnforceComplete());

        return complete_victim_syncset;
    }

    VictimSyncset VictimSyncset::getFullCompressedVictimSyncset(const SeqNum& seqnum, const bool& is_enforce_complete)
    {
        VictimSyncset tmp_victim_syncset;
        tmp_victim_syncset.compressed_bitmap_ = FULLY_COMPRESSED_BITMAP;
        tmp_victim_syncset.seqnum_ = seqnum;
        tmp_victim_syncset.is_enforce_complete_ = is_enforce_complete;
        tmp_victim_syncset.cache_margin_bytes_ = 0;
        tmp_victim_syncset.cache_margin_delta_bytes_ = 0;
        tmp_victim_syncset.local_synced_victims_.clear();
        tmp_victim_syncset.local_beaconed_victims_.clear();
        return tmp_victim_syncset;
    }

    VictimSyncset::VictimSyncset() : seqnum_(0), is_enforce_complete_(false), cache_margin_bytes_(0), cache_margin_delta_bytes_(0)
    {
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ = INVALID_BITMAP;
        #else
        is_valid_ = false;
        is_complete_ = false;
        is_cache_margin_bytes_delta_ = false;
        is_local_beaconed_victims_dedup_ = false;
        is_local_synced_victims_empty_ = false;
        is_local_beaconed_victims_dedup_ = false;
        is_local_beaconed_victims_empty_ = false;
        #endif

        local_synced_victims_.clear();
        local_beaconed_victims_.clear();
    }

    VictimSyncset::VictimSyncset(const VictimSyncset& other)
    {
        *this = other;
    }

    VictimSyncset::VictimSyncset(const SeqNum& seqnum, const bool& is_enforce_complete, const uint64_t& cache_margin_bytes, const std::list<VictimCacheinfo>& local_synced_victims, const std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victims) : seqnum_(seqnum), is_enforce_complete_(is_enforce_complete), cache_margin_bytes_(cache_margin_bytes)
    {
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ = COMPLETE_BITMAP;
        #else
        is_valid_ = true;
        is_complete_ = true;
        is_cache_margin_bytes_delta_ = false;
        is_local_beaconed_victims_dedup_ = false;
        is_local_synced_victims_empty_ = false;
        is_local_beaconed_victims_dedup_ = false;
        is_local_beaconed_victims_empty_ = false;
        #endif

        cache_margin_delta_bytes_ = 0;

        local_synced_victims_ = local_synced_victims;
        if (local_synced_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
            #else
            is_local_synced_victims_empty_ = true;
            #endif
        }
        else
        {
            assertAllCacheinfosComplete_();
        }

        local_beaconed_victims_ = local_beaconed_victims;
        if (local_beaconed_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
            #else
            is_local_beaconed_victims_empty_ = true;
            #endif
        }
        else
        {
            assertAllDirinfoSetsComplete_();
        }
    }

    VictimSyncset::~VictimSyncset() {}

    bool VictimSyncset::isComplete() const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        return ((compressed_bitmap_ & COMPRESS_MASK) == (COMPLETE_BITMAP & COMPRESS_MASK));
        #else
        return is_complete_;
        #endif
    }

    bool VictimSyncset::isCompressed() const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        return ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) == CACHE_MARGIN_BYTES_DELTA_MASK) || ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) == LOCAL_SYNCED_VICTIMS_DEDUP_MASK) || ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) == LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        #else
        return is_cache_margin_bytes_delta_ || is_local_synced_victims_dedup_ || is_local_beaconed_victims_dedup_;
        #endif
    }

    uint64_t VictimSyncset::getSeqnum() const
    {
        checkValidity_();

        return seqnum_;
    }

    void VictimSyncset::setSeqnum(const SeqNum& seqnum)
    {
        checkValidity_();

        seqnum_ = seqnum;
        return;
    }

    bool VictimSyncset::isEnforceComplete() const
    {
        checkValidity_();

        return is_enforce_complete_;
    }

    void VictimSyncset::setEnforceComplete(const bool& is_enforce_complete)
    {
        checkValidity_();

        is_enforce_complete_ = is_enforce_complete;
        return;
    }

    // For both complete and compressed victim syncsets

    bool VictimSyncset::getCacheMarginBytesOrDelta(uint64_t& cache_margin_bytes, int& cache_margin_delta_bytes) const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        #else
        bool with_complete_cache_margin_bytes = !is_cache_margin_bytes_delta_;
        #endif
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

    bool VictimSyncset::getLocalSyncedVictims(std::list<VictimCacheinfo>& local_synced_victims) const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) != LOCAL_SYNCED_VICTIMS_DEDUP_MASK);
        bool is_empty = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool with_complete_local_synced_victims = !is_local_synced_victims_dedup_;
        bool is_empty = is_local_synced_victims_empty_;
        #endif

        if (is_empty)
        {
            local_synced_victims.clear();
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

            local_synced_victims = local_synced_victims_;
        }

        return with_complete_local_synced_victims;
    }

    const std::list<VictimCacheinfo>* VictimSyncset::getLocalSyncedVictimsPtr(bool& is_complete) const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) != LOCAL_SYNCED_VICTIMS_DEDUP_MASK);
        bool is_empty = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool with_complete_local_synced_victims = !is_local_synced_victims_dedup_;
        bool is_empty = is_local_synced_victims_empty_;
        #endif

        if (with_complete_local_synced_victims)
        {
            if (!is_empty)
            {
                assertAllCacheinfosComplete_();
            }

            is_complete = true;
        }
        else
        {
            if (!is_empty)
            {
                assertAtLeastOneCacheinfoDeduped_();
            }

            is_complete = false;
        }

        return &local_synced_victims_;
    }

    bool VictimSyncset::getLocalBeaconedVictims(std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victims) const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) != LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        bool is_empty = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool with_complete_local_beaconed_victims = !is_local_beaconed_victims_dedup_;
        bool is_empty = is_local_beaconed_victims_empty_;
        #endif

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

    const std::list<std::pair<Key, DirinfoSet>>* VictimSyncset::getLocalBeaconedVictimsPtr(bool& is_complete) const
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) != LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        bool is_empty = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool with_complete_local_beaconed_victims = !is_local_beaconed_victims_dedup_;
        bool is_empty = is_local_beaconed_victims_empty_;
        #endif

        if (with_complete_local_beaconed_victims)
        {
            if (!is_empty)
            {
                assertAllDirinfoSetsComplete_();
            }

            is_complete = true;
        }
        else
        {
            if (!is_empty)
            {
                assertAtLeastOneDirinfoSetCompressed_();
            }

            is_complete = false;
        }

        return &local_beaconed_victims_;
    }

    // For complete victim syncset

    void VictimSyncset::setCacheMarginBytes(const uint64_t& cache_margin_bytes)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ &= (~(CACHE_MARGIN_BYTES_DELTA_MASK & ~COMPRESS_MASK)); // Remove 2nd lowest bit for complete cache margin bytes
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            compressed_bitmap_ &= ~COMPRESS_MASK; // Remove 1st lowest bit for complete victim syncset
        }
        #else
        is_cache_margin_bytes_delta_ = false;
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            is_complete_ = true;
        }
        #endif

        cache_margin_bytes_ = cache_margin_bytes;
        cache_margin_delta_bytes_ = 0;
        return;
    }

    void VictimSyncset::setLocalSyncedVictims(const std::list<VictimCacheinfo>& local_synced_victims)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ &= (~(LOCAL_SYNCED_VICTIMS_DEDUP_MASK & ~COMPRESS_MASK)); // Remove 3rd lowest bit for complete local synced victims
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            compressed_bitmap_ &= ~COMPRESS_MASK; // Remove 1st lowest bit for complete victim syncset
        }
        #else
        is_local_synced_victims_dedup_ = false;
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            is_complete_ = true;
        }
        #endif

        local_synced_victims_ = local_synced_victims;

        if (local_synced_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
            #else
            is_local_synced_victims_empty_ = true;
            #endif
        }
        else
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ &= ~LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
            #else
            is_local_synced_victims_empty_ = false;
            #endif
            assertAllCacheinfosComplete_();
        }

        return;
    }

    void VictimSyncset::setLocalBeaconedVictims(const std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victims)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ &= (~(LOCAL_BEACONED_VICTIMS_DEDUP_MASK & ~COMPRESS_MASK)); // Remove 4th lowest bit for complete local beaconed victims
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            compressed_bitmap_ &= ~COMPRESS_MASK; // Remove 1st lowest bit for complete victim syncset
        }
        #else
        is_local_beaconed_victims_dedup_ = false;
        if (!isCompressed()) // NOT a compressed victim syncset yet
        {
            is_complete_ = true;
        }
        #endif

        local_beaconed_victims_ = local_beaconed_victims;

        if (local_beaconed_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
            #else
            is_local_beaconed_victims_empty_ = true;
            #endif
        }
        else
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ &= ~LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
            #else
            is_local_beaconed_victims_empty_ = false;
            #endif
            assertAllDirinfoSetsComplete_();
        }

        return;
    }

    // For compressed victim syncset

    void VictimSyncset::setCacheMarginDeltaBytes(const int& cache_margin_delta_bytes)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ |= CACHE_MARGIN_BYTES_DELTA_MASK;
        #else
        is_cache_margin_bytes_delta_ = true;
        is_complete_ = false;
        #endif

        cache_margin_delta_bytes_ = cache_margin_delta_bytes;
        cache_margin_bytes_ = 0;
        return;
    }

    void VictimSyncset::setLocalSyncedVictimsForCompress(const std::list<VictimCacheinfo>& local_synced_victims)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_DEDUP_MASK;
        #else
        is_local_synced_victims_dedup_ = true;
        is_complete_ = false;
        #endif

        local_synced_victims_ = local_synced_victims;

        if (local_synced_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
            #else
            is_local_synced_victims_empty_ = true;
            #endif
        }
        else
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ &= ~LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
            #else
            is_local_synced_victims_empty_ = false;
            #endif
            assertAtLeastOneCacheinfoDeduped_();
        }

        return;
    }

    void VictimSyncset::setLocalBeaconedVictimsForCompress(const std::list<std::pair<Key, DirinfoSet>>& local_beaconed_victims)
    {
        checkValidity_();

        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_DEDUP_MASK;
        #else
        is_local_beaconed_victims_dedup_ = true;
        is_complete_ = false;
        #endif

        local_beaconed_victims_ = local_beaconed_victims;

        if (local_beaconed_victims.size() == 0)
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
            #else
            is_local_beaconed_victims_empty_ = true;
            #endif
        }
        else
        {
            #ifdef VICTIM_SYNCSET_USE_BITMAP
            compressed_bitmap_ &= ~LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
            #else
            is_local_beaconed_victims_empty_ = false;
            #endif
            assertAtLeastOneDirinfoSetCompressed_();
        }
        
        return;
    }

    uint32_t VictimSyncset::getVictimSyncsetPayloadSize() const
    {
        // TODO: we can tune the sizes of local synched/beaconed victims, as the numbers are limited under our design (at most peredge_synced_victimcnt and peredge_synced_victimcnt * edgecnt)

        uint32_t victim_syncset_payload_size = 0;

        // Compressed bitmap
        victim_syncset_payload_size += sizeof(uint8_t);

        // Seqnum
        victim_syncset_payload_size += sizeof(SeqNum);

        // is_enforce_complete
        victim_syncset_payload_size += sizeof(bool);

        // Cache margin bytes
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        #else
        bool with_complete_cache_margin_bytes = !is_cache_margin_bytes_delta_;
        #endif
        if (with_complete_cache_margin_bytes)
        {
            victim_syncset_payload_size += sizeof(uint64_t);
        }
        else
        {
            victim_syncset_payload_size += sizeof(int);
        }

        // Local synced victims
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_synced_victims = is_local_synced_victims_empty_;
        #endif
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
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_beaconed_victims = is_local_beaconed_victims_empty_;
        #endif
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            assert(local_beaconed_victims_.size() > 0);

            victim_syncset_payload_size += sizeof(uint32_t); // Size of local_beaconed_victims_
            for (std::list<std::pair<Key, DirinfoSet>>::const_iterator key_dirinfoset_list_iter = local_beaconed_victims_.begin(); key_dirinfoset_list_iter != local_beaconed_victims_.end(); key_dirinfoset_list_iter++)
            {
                victim_syncset_payload_size += key_dirinfoset_list_iter->first.getKeyPayloadSize();

                // Dirinfo set for the given key
                const DirinfoSet& tmp_dirinfo_set = key_dirinfoset_list_iter->second;
                victim_syncset_payload_size += tmp_dirinfo_set.getDirinfoSetPayloadSize(); // Complete or compressed
            }
        }

        return victim_syncset_payload_size;
    }

    uint32_t VictimSyncset::serialize(DynamicArray& msg_payload, const uint32_t& position) const
    {
        checkValidity_();

        uint32_t size = position;

        // Compressed bitmap
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        msg_payload.deserialize(size, (const char*)&compressed_bitmap_, sizeof(uint8_t));
        #else
        uint8_t compressed_bitmap = COMPLETE_BITMAP;
        if (!is_complete_)
        {
            if (is_cache_margin_bytes_delta_)
            {
                compressed_bitmap |= CACHE_MARGIN_BYTES_DELTA_MASK;
            }
            if (is_local_synced_victims_dedup_)
            {
                compressed_bitmap |= LOCAL_SYNCED_VICTIMS_DEDUP_MASK;
            }
            if (is_local_beaconed_victims_dedup_)
            {
                compressed_bitmap |= LOCAL_BEACONED_VICTIMS_DEDUP_MASK;
            }
        }
        if (is_local_synced_victims_empty_)
        {
            assert(local_synced_victims_.size() == 0);
            compressed_bitmap |= LOCAL_SYNCED_VICTIMS_EMPTY_MASK;
        }
        if (is_local_beaconed_victims_empty_)
        {
            assert(local_beaconed_victims_.size() == 0);
            compressed_bitmap |= LOCAL_BEACONED_VICTIMS_EMPTY_MASK;
        }
        msg_payload.deserialize(size, (const char*)&compressed_bitmap, sizeof(uint8_t));
        #endif
        size += sizeof(uint8_t);

        // Seqnum
        msg_payload.deserialize(size, (const char*)&seqnum_, sizeof(SeqNum));
        size += sizeof(SeqNum);

        // is_enforce_complete
        msg_payload.deserialize(size, (const char*)&is_enforce_complete_, sizeof(bool));
        size += sizeof(bool);

        // Cache margin bytes
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        #else
        bool with_complete_cache_margin_bytes = !is_cache_margin_bytes_delta_;
        #endif
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
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_synced_victims = is_local_synced_victims_empty_;
        #endif
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
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_beaconed_victims = is_local_beaconed_victims_empty_;
        #endif
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            uint32_t local_beaconed_victims_size = local_beaconed_victims_.size();
            msg_payload.deserialize(size, (const char*)&local_beaconed_victims_size, sizeof(uint32_t));
            size += sizeof(uint32_t);

            // Local beaconed victims
            for (std::list<std::pair<Key, DirinfoSet>>::const_iterator key_dirinfoset_list_iter = local_beaconed_victims_.begin(); key_dirinfoset_list_iter != local_beaconed_victims_.end(); key_dirinfoset_list_iter++)
            {
                uint32_t key_serialize_size = key_dirinfoset_list_iter->first.serialize(msg_payload, size);
                size += key_serialize_size;

                // Dirinfo set for the given key
                const DirinfoSet& tmp_dirinfo_set = key_dirinfoset_list_iter->second;
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
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        msg_payload.serialize(size, (char*)&compressed_bitmap_, sizeof(uint8_t));
        assert(compressed_bitmap_ != INVALID_BITMAP);
        #else
        uint8_t compressed_bitmap = INVALID_BITMAP;
        msg_payload.serialize(size, (char*)&compressed_bitmap, sizeof(uint8_t));
        assert(compressed_bitmap != INVALID_BITMAP);
        is_valid_ = true;
        if (((compressed_bitmap & COMPRESS_MASK) == (COMPLETE_BITMAP & COMPRESS_MASK)))
        {
            is_complete_ = true;
        }
        else
        {
            is_cache_margin_bytes_delta_ = ((compressed_bitmap & CACHE_MARGIN_BYTES_DELTA_MASK) == CACHE_MARGIN_BYTES_DELTA_MASK);
            is_local_synced_victims_dedup_ = ((compressed_bitmap & LOCAL_SYNCED_VICTIMS_DEDUP_MASK) == LOCAL_SYNCED_VICTIMS_DEDUP_MASK);
            is_local_beaconed_victims_dedup_ = ((compressed_bitmap & LOCAL_BEACONED_VICTIMS_DEDUP_MASK) == LOCAL_BEACONED_VICTIMS_DEDUP_MASK);
        }
        is_local_synced_victims_empty_ = ((compressed_bitmap & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        is_local_beaconed_victims_empty_ = ((compressed_bitmap & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #endif
        size += sizeof(uint8_t);

        // Seqnum
        msg_payload.serialize(size, (char*)&seqnum_, sizeof(SeqNum));
        size += sizeof(SeqNum);

        // is_enforce_complete
        msg_payload.serialize(size, (char*)&is_enforce_complete_, sizeof(bool));
        size += sizeof(bool);

        // Cache margin bytes
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        #else
        bool with_complete_cache_margin_bytes = !is_cache_margin_bytes_delta_;
        #endif
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
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_synced_victims = is_local_synced_victims_empty_;
        #endif
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
                assert(!cacheinfo.isInvalid());
                
                local_synced_victims_.push_back(cacheinfo);
            }
        }

        // Size of local beaconed victims
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_beaconed_victims = is_local_beaconed_victims_empty_;
        #endif
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
                assert(!tmp_dirinfo_set.isInvalid());

                local_beaconed_victims_.push_back(std::pair(key, tmp_dirinfo_set));
            }
        }

        return size - position;
    }

    uint64_t VictimSyncset::getSizeForCapacity() const
    {
        // TODO: we can tune the sizes of local synched/beaconed victims, as the numbers are limited under our design (at most peredge_synced_victimcnt and peredge_synced_victimcnt * edgecnt)

        uint32_t victim_syncset_size_bytes = 0;

        // Compressed bitmap
        victim_syncset_size_bytes += sizeof(uint8_t);

        // Seqnum
        victim_syncset_size_bytes += sizeof(SeqNum);

        // is_enforce_complete
        victim_syncset_size_bytes += sizeof(bool);

        // Cache margin bytes
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool with_complete_cache_margin_bytes = ((compressed_bitmap_ & CACHE_MARGIN_BYTES_DELTA_MASK) != CACHE_MARGIN_BYTES_DELTA_MASK);
        #else
        bool with_complete_cache_margin_bytes = !is_cache_margin_bytes_delta_;
        #endif
        if (with_complete_cache_margin_bytes)
        {
            victim_syncset_size_bytes += sizeof(uint64_t);
        }
        else
        {
            victim_syncset_size_bytes += sizeof(int);
        }

        // Local synced victims
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_synced_victims = ((compressed_bitmap_ & LOCAL_SYNCED_VICTIMS_EMPTY_MASK) == LOCAL_SYNCED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_synced_victims = is_local_synced_victims_empty_;
        #endif
        if (!is_empty_local_synced_victims) // Non-empty local synced victims
        {
            assert(local_synced_victims_.size() > 0);

            for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_iter = local_synced_victims_.begin(); cacheinfo_list_iter != local_synced_victims_.end(); cacheinfo_list_iter++)
            {
                victim_syncset_size_bytes += cacheinfo_list_iter->getSizeForCapacity(); // Complete or compressed
            }
        }

        // Local beaconed victims
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        bool is_empty_local_beaconed_victims = ((compressed_bitmap_ & LOCAL_BEACONED_VICTIMS_EMPTY_MASK) == LOCAL_BEACONED_VICTIMS_EMPTY_MASK);
        #else
        bool is_empty_local_beaconed_victims = is_local_beaconed_victims_empty_;
        #endif
        if (!is_empty_local_beaconed_victims) // Non-empty local beaconed victims
        {
            assert(local_beaconed_victims_.size() > 0);

            for (std::list<std::pair<Key, DirinfoSet>>::const_iterator key_dirinfoset_list_iter = local_beaconed_victims_.begin(); key_dirinfoset_list_iter != local_beaconed_victims_.end(); key_dirinfoset_list_iter++)
            {
                victim_syncset_size_bytes += key_dirinfoset_list_iter->first.getKeyPayloadSize();

                // Dirinfo set for the given key
                const DirinfoSet& tmp_dirinfo_set = key_dirinfoset_list_iter->second;
                victim_syncset_size_bytes += tmp_dirinfo_set.getSizeForCapacity(); // Complete or compressed
            }
        }

        return victim_syncset_size_bytes;
    }

    // void VictimSyncset::dumpForDebug() const
    // {
    //     #ifndef VICTIM_SYNCSET_USE_BITMAP
    //     std::ostringstream oss;
    //     oss << "is_valid_: " << Util::toString(is_valid_) << std::endl;
    //     oss << "is_complete_: " << Util::toString(is_complete_) << std::endl;
    //     oss << "is_cache_margin_bytes_delta_: " << Util::toString(is_cache_margin_bytes_delta_) << std::endl;
    //     oss << "is_local_synced_victims_dedup_: " << Util::toString(is_local_synced_victims_dedup_) << std::endl;
    //     oss << "is_local_synced_victims_empty_: " << Util::toString(is_local_synced_victims_empty_) << std::endl;
    //     oss << "is_local_beaconed_victims_dedup_: " << Util::toString(is_local_beaconed_victims_dedup_) << std::endl;
    //     oss << "is_local_beaconed_victims_empty_: " << Util::toString(is_local_beaconed_victims_empty_) << std::endl;
    //     Util::dumpDebugMsg(kClassName, oss.str());
    //     #endif
    //     return;
    // }

    const VictimSyncset& VictimSyncset::operator=(const VictimSyncset& other)
    {
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        compressed_bitmap_ = other.compressed_bitmap_;
        #else
        is_valid_ = other.is_valid_;
        is_complete_ = other.is_complete_;
        is_cache_margin_bytes_delta_ = other.is_cache_margin_bytes_delta_;
        is_local_synced_victims_dedup_ = other.is_local_synced_victims_dedup_;
        is_local_synced_victims_empty_ = other.is_local_synced_victims_empty_;
        is_local_beaconed_victims_dedup_ = other.is_local_beaconed_victims_dedup_;
        is_local_beaconed_victims_empty_ = other.is_local_beaconed_victims_empty_;
        #endif

        seqnum_ = other.seqnum_;
        is_enforce_complete_ = other.is_enforce_complete_;
        cache_margin_bytes_ = other.cache_margin_bytes_;
        cache_margin_delta_bytes_ = other.cache_margin_delta_bytes_;
        local_synced_victims_ = other.local_synced_victims_;
        local_beaconed_victims_ = other.local_beaconed_victims_;
        
        return *this;
    }

    void VictimSyncset::checkValidity_() const
    {
        #ifdef VICTIM_SYNCSET_USE_BITMAP
        assert(compressed_bitmap_ != INVALID_BITMAP);
        #else
        assert(is_valid_);
        #endif

        return;
    }

    void VictimSyncset::assertAtLeastOneCacheinfoDeduped_() const
    {
        assert(local_synced_victims_.size() > 0);

        // NOTE: as fully-deduped victim cacheinfos will NOT be tracked by victim syncset, the list of remaining victim cacheinfos could still be complete

        /*bool tmp_at_least_one_dedup = false;
        for (std::list<VictimCacheinfo>::const_iterator cacheinfo_list_const_iter = local_synced_victims_.begin(); cacheinfo_list_const_iter != local_synced_victims_.end(); cacheinfo_list_const_iter++)
        {
            if (cacheinfo_list_const_iter->isDeduped())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);*/

        return;
    }

    void VictimSyncset::assertAllCacheinfosComplete_() const
    {
        assert(local_synced_victims_.size() > 0);

        if (Config::isAssert())
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
        }

        return;
    }

    void VictimSyncset::assertAtLeastOneDirinfoSetCompressed_() const
    {
        assert(local_beaconed_victims_.size() > 0);

        /*bool tmp_at_least_one_dedup = false;
        for (std::unordered_map<Key, DirinfoSet, KeyHasher>::const_iterator dirinfo_map_const_iter = local_beaconed_victims_.begin(); dirinfo_map_const_iter != local_beaconed_victims_.end(); dirinfo_map_const_iter++)
        {
            if (dirinfo_map_const_iter->second.isCompressed())
            {
                tmp_at_least_one_dedup = true;
                break;
            }
        }
        assert(tmp_at_least_one_dedup == true);*/

        return;
    }

    void VictimSyncset::assertAllDirinfoSetsComplete_() const
    {
        assert(local_beaconed_victims_.size() > 0);

        if (Config::isAssert())
        {
            bool tmp_all_complete = true;
            for (std::list<std::pair<Key, DirinfoSet>>::const_iterator key_dirinfoset_const_iter = local_beaconed_victims_.begin(); key_dirinfoset_const_iter != local_beaconed_victims_.end(); key_dirinfoset_const_iter++)
            {
                if (!key_dirinfoset_const_iter->second.isComplete())
                {
                    tmp_all_complete = false;
                    break;
                }
            }
            assert(tmp_all_complete == true);
        }

        return;
    }
}
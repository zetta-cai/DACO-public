#include "core/popularity/aggregated_uncached_popularity.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string AggregatedUncachedPopularity::kClassName("AggregatedUncachedPopularity");

    AggregatedUncachedPopularity::AggregatedUncachedPopularity() : key_(), object_size_(0), sum_local_uncached_popularity_(0.0), exist_edgecnt_(0)
    {
        topk_edgeidx_local_uncached_popularity_pairs_.clear();
        bitmap_.clear();
    }

    AggregatedUncachedPopularity::AggregatedUncachedPopularity(const Key& key, const uint32_t& edgecnt) : key_(key), object_size_(0), sum_local_uncached_popularity_(0.0), exist_edgecnt_(0)
    {
        topk_edgeidx_local_uncached_popularity_pairs_.clear();
        bitmap_.resize(edgecnt, false);
    }
    
    AggregatedUncachedPopularity::~AggregatedUncachedPopularity() {}

    const Key& AggregatedUncachedPopularity::getKey() const
    {
        return key_;
    }

    ObjectSize AggregatedUncachedPopularity::getObjectSize() const
    {
        return object_size_;
    }

    Popularity AggregatedUncachedPopularity::getSumLocalUncachedPopularity() const
    {
        return sum_local_uncached_popularity_;
    }

    uint32_t AggregatedUncachedPopularity::getTopkListLength() const
    {
        return topk_edgeidx_local_uncached_popularity_pairs_.size();
    }

    bool AggregatedUncachedPopularity::hasLocalUncachedPopularity(const uint32_t& source_edge_idx) const
    {
        assert(source_edge_idx < bitmap_.size());
        return bitmap_[source_edge_idx];
    }

    void AggregatedUncachedPopularity::update(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt, const ObjectSize& object_size)
    {
        assert(source_edge_idx < bitmap_.size());

        bool is_existing = bitmap_[source_edge_idx];
        if (!is_existing) // Never receive any local uncached popularity from the source edge idx before
        {
            // Update sum of local uncached popularity for the new edge idx
            sum_local_uncached_popularity_ = Util::popularityAdd(sum_local_uncached_popularity_, local_uncached_popularity);

            // Update top-k local uncached popularities for the new edge idx if necessary
            bool is_insert = tryToInsertForNontopkEdgeIdx_(source_edge_idx, local_uncached_popularity, topk_edgecnt); // NOTE: new edge idx MUST not exist in top-k list
            UNUSED(is_insert);

            // Update bitmap for the new edge idx
            bitmap_[source_edge_idx] = true;
            exist_edgecnt_ += 1;
        }
        else // The source edge idx has reported local uncached popularity before
        {
            // Get original local uncahced popularity for the existing source edge idx
            Popularity original_local_uncached_popularity = getLocalUncachedPopularityForExistingEdgeIdx_(source_edge_idx);

            // Update sum of local uncached popularity for the existing edge idx
            sum_local_uncached_popularity_ = Util::popularityAdd(sum_local_uncached_popularity_, local_uncached_popularity);
            minusLocalUncachedPopularityFromSum_(original_local_uncached_popularity);

            // Update top-k local uncached popularities for the existing edge idx if necessary
            bool is_insert = updateTopkForExistingEdgeIdx_(source_edge_idx, local_uncached_popularity, topk_edgecnt); // NOTE: existing edge idx may already exist in top-k list or not
            UNUSED(is_insert);

            // NO need to update bitmap for the existing edge idx
        }
        object_size_ = object_size;

        return;
    }

    bool AggregatedUncachedPopularity::clearForPlacement(const Edgeset& placement_edgeset)
    {
        for (std::unordered_set<uint32_t>::const_iterator placement_edgeset_const_iter = placement_edgeset.begin(); placement_edgeset_const_iter != placement_edgeset.end(); placement_edgeset_const_iter++)
        {
            const uint32_t tmp_edge_idx = *placement_edgeset_const_iter;
            assert(tmp_edge_idx < bitmap_.size());

            // NOTE: calculate placement and preserve edgeset are NOT a single atomic operation in CoveredCacheManager, if cache server (sender is beacon) and beacon server get the same placement for the same key and preserve edgeset simultaneously, the latter one may observe false bits in bitmap_ here after the former one preserves the edgeset
            // (OBSOLETE) NOTE: as placement edgeset is selected from top-k list of aggregated uncached popularity, tmp_edge_idx MUST exist in bitmap_
            //assert(bitmap_[tmp_edge_idx] == true);

            // Release local uncached popularity of tmp_edge_idx from sum/topk/bitmap in aggregated uncached popularity
            bool tmp_is_clear = false;
            clear(tmp_edge_idx, tmp_is_clear);
            UNUSED(tmp_is_clear);
        }

        return (exist_edgecnt_ == 0);
    }

    bool AggregatedUncachedPopularity::clear(const uint32_t& source_edge_idx, bool& is_clear)
    {
        assert(source_edge_idx < bitmap_.size());
        is_clear = false;

        // NOTE: NO need to remove old local uncached popularity if never received any from the source edge idx before
        bool is_existing = bitmap_[source_edge_idx];
        if (is_existing) // The source edge idx has reported local uncached popularity before
        {
            // Get original local uncahced popularity for the existing source edge idx
            Popularity original_local_uncached_popularity = getLocalUncachedPopularityForExistingEdgeIdx_(source_edge_idx);

            // Update sum of local uncached popularity for the existing edge idx
            minusLocalUncachedPopularityFromSum_(original_local_uncached_popularity);

            // Remove old local uncached popularity from top-k list if necessary
            clearTopkForExistingEdgeIdx_(source_edge_idx); // NOTE: existing edge idx may already exist in top-k list or not

            // Update bitmap for the removal
            bitmap_[source_edge_idx] = false;
            assert(exist_edgecnt_ >= 1);
            exist_edgecnt_ -= 1;

            is_clear = true;
        }

        return (exist_edgecnt_ == 0);
    }

    DeltaReward AggregatedUncachedPopularity::calcMaxAdmissionBenefit(const EdgeWrapper* edge_wrapper_ptr, const bool& is_global_cached) const
    {
        Edgeset placement_edgeset;
        DeltaReward max_admission_benefit = calcAdmissionBenefit(edge_wrapper_ptr, topk_edgeidx_local_uncached_popularity_pairs_.size(), is_global_cached, placement_edgeset);
        UNUSED(placement_edgeset);
        
        return max_admission_benefit;
    }

    DeltaReward AggregatedUncachedPopularity::calcAdmissionBenefit(const EdgeWrapper* edge_wrapper_ptr, const uint32_t& topicnt, const bool& is_global_cached, Edgeset& placement_edgeset) const
    {
        // TODO: Use a heuristic or learning-based approach for parameter tuning to calculate delta rewards for max admission benefits (refer to state-of-the-art studies such as LRB and GL-Cache)

        Popularity topi_local_uncached_popularity_ = getTopiLocalUncachedPopularitySum_(topicnt, placement_edgeset);
        if (!(Util::isApproxLargerEqual(sum_local_uncached_popularity_, topi_local_uncached_popularity_)))
        {
            std::ostringstream oss;
            oss << "sum_local_uncached_popularity_ " << sum_local_uncached_popularity_ << " should >= topi_local_uncached_popularity_ " << topi_local_uncached_popularity_ << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        Popularity sum_minus_topi = Util::popularityNonegMinus(sum_local_uncached_popularity_, topi_local_uncached_popularity_);
        DeltaReward admission_benefit = edge_wrapper_ptr->calcLocalUncachedReward(topi_local_uncached_popularity_, is_global_cached, sum_minus_topi);

        return admission_benefit;
    }

    uint64_t AggregatedUncachedPopularity::getSizeForCapacity() const
    {
        uint64_t total_size = 0;

        total_size += key_.getKeyLength();
        total_size += sizeof(ObjectSize);
        total_size += sizeof(Popularity);
        total_size += topk_edgeidx_local_uncached_popularity_pairs_.size() * (sizeof(uint32_t) + sizeof(Popularity));
        total_size += (bitmap_.size() * sizeof(bool) - 1) / 8 + 1; // NOTE: we just use std::vector<bool> to simulate a bitmap, but it can be replaced by a bitset with bitwise operations
        total_size += sizeof(uint32_t); // exist_edgecnt_

        return total_size;
    }

    const AggregatedUncachedPopularity& AggregatedUncachedPopularity::operator=(const AggregatedUncachedPopularity& other)
    {
        if (this != &other)
        {
            key_ = other.key_;
            object_size_ = other.object_size_;
            sum_local_uncached_popularity_ = other.sum_local_uncached_popularity_;
            topk_edgeidx_local_uncached_popularity_pairs_ = other.topk_edgeidx_local_uncached_popularity_pairs_;
            bitmap_ = other.bitmap_;
            exist_edgecnt_ = other.exist_edgecnt_;
        }
        return *this;
    }

    Popularity AggregatedUncachedPopularity::getLocalUncachedPopularityForExistingEdgeIdx_(const uint32_t& source_edge_idx) const
    {
        assert(bitmap_[source_edge_idx] == true);

        // Get original local uncached popularity
        Popularity local_uncached_popularity = 0.0;
        std::list<edgeidx_popularity_pair_t>::const_iterator topk_list_iter = getTopkListIterForEdgeIdx_(source_edge_idx);
        if (topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end()) // The existing source edge idx is in top-k list
        {
            local_uncached_popularity = topk_list_iter->second;
        }
        else // The existing source edge idx is not in top-k list
        {
            Popularity sum_topk_local_uncached_popularity = 0.0;
            for (std::list<edgeidx_popularity_pair_t>::const_iterator tmp_list_iter_for_sum = topk_edgeidx_local_uncached_popularity_pairs_.begin(); tmp_list_iter_for_sum != topk_edgeidx_local_uncached_popularity_pairs_.end(); tmp_list_iter_for_sum++)
            {
                sum_topk_local_uncached_popularity += tmp_list_iter_for_sum->second;
            }
            assert(sum_topk_local_uncached_popularity <= sum_local_uncached_popularity_);

            // NOTE: exist_edgecnt MUST be larger than topk_edgecnt, as the existing source edge idx is NOT in top-k list
            uint32_t topk_edgecnt = topk_edgeidx_local_uncached_popularity_pairs_.size();
            assert(topk_edgecnt < exist_edgecnt_);

            // Use average of non-topk local uncached popularities to approximate the original one
            local_uncached_popularity = (sum_local_uncached_popularity_ - sum_topk_local_uncached_popularity) / (exist_edgecnt_ - topk_edgecnt);
        }

        return local_uncached_popularity;
    }

    // For sum

    void AggregatedUncachedPopularity::minusLocalUncachedPopularityFromSum_(const Popularity& local_uncached_popularity)
    {
        sum_local_uncached_popularity_ -= local_uncached_popularity;
        if (sum_local_uncached_popularity_ < 0.0)
        {
            sum_local_uncached_popularity_ = 0.0;
        }
        return;
    }

    // For top-k list

    Popularity AggregatedUncachedPopularity::getTopiLocalUncachedPopularitySum_(const uint32_t& topicnt, Edgeset& placement_edgeset) const
    {
        const uint32_t topk_list_length = topk_edgeidx_local_uncached_popularity_pairs_.size();
        assert(topicnt <= topk_list_length);

        // NOTE: topk_edgeidx_local_uncached_popularity_pairs_ follows an ascending order of local uncached popularity -> we need to choose the last topicnt local uncached popularities
        const uint32_t offset_to_head = topk_list_length - topicnt;
        std::list<edgeidx_popularity_pair_t>::const_iterator topk_list_iter = topk_edgeidx_local_uncached_popularity_pairs_.begin();
        if (offset_to_head > 0)
        {
            std::advance(topk_list_iter, offset_to_head);
        }

        // From the offset of list head to the list tail
        Popularity topi_local_uncached_popularity_ = 0.0;
        placement_edgeset.clear();
        for (; topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end(); topk_list_iter++)
        {
            topi_local_uncached_popularity_ = Util::popularityAdd(topi_local_uncached_popularity_, topk_list_iter->second);
            placement_edgeset.insert(topk_list_iter->first);
        }

        return topi_local_uncached_popularity_;
    }

    std::list<AggregatedUncachedPopularity::edgeidx_popularity_pair_t>::const_iterator AggregatedUncachedPopularity::getTopkListIterForEdgeIdx_(const uint32_t& source_edge_idx) const
    {
        std::list<edgeidx_popularity_pair_t>::const_iterator topk_list_iter = topk_edgeidx_local_uncached_popularity_pairs_.begin();
        while (topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end())
        {
            if (topk_list_iter->first == source_edge_idx)
            {
                break;
            }
            topk_list_iter++;
        }
        return topk_list_iter;
    }

    bool AggregatedUncachedPopularity::updateTopkForExistingEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt)
    {
        assert(bitmap_[source_edge_idx] == true);

        // Check if local uncached popularity of the existing edge idx should be inserted into top-k list
        bool must_insert = false;
        std::list<edgeidx_popularity_pair_t>::const_iterator topk_list_iter = getTopkListIterForEdgeIdx_(source_edge_idx);
        if (topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end()) // The existing source edge idx is in top-k list
        {
            // Remove the old list entry -> now the existing source edge idx becomes a non-topk edge idx
            topk_edgeidx_local_uncached_popularity_pairs_.erase(topk_list_iter);

            must_insert = true; // As top-k list MUST be not full after removal of the old list entry, local uncached popularity of the existing source edge idx MUST be inserted into top-k list
        }
        
        bool is_insert = tryToInsertForNontopkEdgeIdx_(source_edge_idx, local_uncached_popularity, topk_edgecnt);
        if (must_insert)
        {
            assert(is_insert == true);
        }

        return is_insert;
    }

    bool AggregatedUncachedPopularity::tryToInsertForNontopkEdgeIdx_(const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const uint32_t& topk_edgecnt)
    {
        assert(getTopkListIterForEdgeIdx_(source_edge_idx) == topk_edgeidx_local_uncached_popularity_pairs_.end());

        // Check if local uncached popularity of the non-topk edge idx should be inserted into top-k list
        bool need_insert = false;
        if (topk_edgeidx_local_uncached_popularity_pairs_.size() >= topk_edgecnt) // If top-k list is full
        {
            Popularity min_topk_local_uncached_popularity = topk_edgeidx_local_uncached_popularity_pairs_.front().second;
            if (local_uncached_popularity > min_topk_local_uncached_popularity) // If local uncached popularity is larger than the minimum one in top-k list
            {
                // Remove minimum local uncached popularity from the top-k
                //const uint32_t min_edge_idx = topk_edgeidx_local_uncached_popularity_pairs_.front().first;
                topk_edgeidx_local_uncached_popularity_pairs_.pop_front();

                // NOTE: NO need to remove min_edge_idx from sum and bitmap, as it still tracks the given key in local uncached metadata, which should hold in sum and bitmap

                // (OBSOLETE) Update sum and bitmap for the minimum local uncached popularity
                //minusLocalUncachedPopularityFromSum_(min_topk_local_uncached_popularity);
                //assert(bitmap_[min_edge_idx] == true);
                //bitmap_[min_edge_idx] = false;

                need_insert = true;
            }
        }
        else // Top-k list is not full
        {
            need_insert = true;
        }

        // Insert local uncached popularity of the non-topk edge idx into top-k list in ascending order
        if (need_insert)
        {
            std::list<edgeidx_popularity_pair_t>::iterator topk_list_iter = topk_edgeidx_local_uncached_popularity_pairs_.begin();
            while (topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end())
            {
                if (local_uncached_popularity < topk_list_iter->second)
                {
                    break;
                }
                topk_list_iter++;
            }
            topk_edgeidx_local_uncached_popularity_pairs_.insert(topk_list_iter, std::pair(source_edge_idx, local_uncached_popularity));
        }

        return need_insert;
    }

    void AggregatedUncachedPopularity::clearTopkForExistingEdgeIdx_(const uint32_t& source_edge_idx)
    {
        assert(bitmap_[source_edge_idx] == true);

        // Check if local uncached popularity of the existing edge idx should be removed from top-k list
        std::list<edgeidx_popularity_pair_t>::const_iterator topk_list_iter = getTopkListIterForEdgeIdx_(source_edge_idx);
        if (topk_list_iter != topk_edgeidx_local_uncached_popularity_pairs_.end()) // The existing source edge idx is in top-k list
        {
            // Remove the old list entry
            topk_edgeidx_local_uncached_popularity_pairs_.erase(topk_list_iter);
        }

        return;
    }
}
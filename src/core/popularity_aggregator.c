#include "core/popularity_aggregator.h"

#include "common/util.h"

namespace covered
{
    const std::string PopularityAggregator::kClassName("PopularityAggregator");

    PopularityAggregator::PopularityAggregator(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint64_t& popularity_aggregation_capacity_bytes, const uint32_t& topk_edgecnt) : edgecnt_(edgecnt), popularity_aggregation_capacity_bytes_(popularity_aggregation_capacity_bytes), topk_edgecnt_(topk_edgecnt)
    {
        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        oss.clear();
        oss.str("");
        oss << instance_name_ << " " << "rwlock_for_popularity_aggregator_";
        rwlock_for_popularity_aggregator_ = new Rwlock(oss.str());
        assert(rwlock_for_popularity_aggregator_ != NULL);

        size_bytes_ = 0;
        benefit_popularity_multimap_.clear();
        perkey_benefit_popularity_table_.clear();
        perkey_preserved_edgeset_.clear();
    }
    
    PopularityAggregator::~PopularityAggregator()
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        delete rwlock_for_popularity_aggregator_;
        rwlock_for_popularity_aggregator_ = NULL;
    }

    bool PopularityAggregator::getAggregatedUncachedPopularity(const Key& key, AggregatedUncachedPopularity& aggregated_uncached_popularity) const
    {
        checkPointers_();

        // Acquire a READ lock to get aggregated uncached popularity atomically
        const std::string context_name = "PopularityAggregator::getAggregatedUncachedPopularity()";
        rwlock_for_popularity_aggregator_->acquire_lock_shared(context_name);

        bool is_found = getAggregatedUncachedPopularity_(key, aggregated_uncached_popularity);

        rwlock_for_popularity_aggregator_->unlock_shared(context_name);
        return is_found;
    }

    uint32_t PopularityAggregator::getTopkEdgecnt() const
    {
        checkPointers_();

        // Acquire a read lock to get top-k list length atomically (TODO: maybe NO need to acquire a read lock for topk_edgecnt_ here)
        std::string context_name = "PopularityAggregator::getTopkEdgecnt()";
        rwlock_for_popularity_aggregator_->acquire_lock_shared(context_name);

        uint32_t topk_edgecnt = topk_edgecnt_;

        rwlock_for_popularity_aggregator_->unlock_shared(context_name);
        return topk_edgecnt;
    }

    bool PopularityAggregator::isKeyBeingAdmitted(const Key& key) const
    {
        checkPointers_();

        // Acquire a read lock to check if key is being admitted (i.e., with preserved edge nodes) atomically (TODO: maybe NO need to acquire a read lock for is_being_admitted here)
        std::string context_name = "PopularityAggregator::isKeyBeingAdmitted()";
        rwlock_for_popularity_aggregator_->acquire_lock_shared(context_name);

        bool is_being_admitted = (perkey_preserved_edgeset_.find(key) != perkey_preserved_edgeset_.end());

        rwlock_for_popularity_aggregator_->unlock_shared(context_name);
        return is_being_admitted;
    }

    void PopularityAggregator::updateAggregatedUncachedPopularity(const Key& key, const uint32_t& source_edge_idx, const CollectedPopularity& collected_popularity, const bool& is_global_cached, const bool& is_source_cached, FastPathHint& fast_path_hint)
    {
        checkPointers_();

        // Acquire a write lock to update aggregated uncached popularity and max admission benefit atomically
        const std::string context_name = "PopularityAggregator::updateAggregatedUncachedPopularity()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        #ifdef ENABLE_FAST_PATH_PLACEMENT
        // NOTE: ONLY if key is NOT tracked by sender local uncached metadata, NOT in preserved edgeset (i.e., NOT being admited), NO dirinfo of sender (i.e., NOT finish placement notification in sender), and NOT in bitmap of aggregated pop that needs to be removed (i.e., NOT tracked before), we will treat the key as need_fast_path_hint (i.e., possibly the first cache miss w/o objsize)
        bool need_fast_path_hint = true; // For fast-path single-placement calculation
        #endif

        bool is_tracked_by_source_edge_node = collected_popularity.isTracked(); // If key is tracked by local uncached metadata in the source edge node (i.e., if local uncached popularity is valid)
        #ifdef ENABLE_FAST_PATH_PLACEMENT
        if (is_tracked_by_source_edge_node)
        {
            // Tracked by sender local uncached metadata
            need_fast_path_hint = false;
        }
        #endif

        // Ignore local uncached popularity if necessary
        if (is_source_cached) // Ignore local uncached popularity if key is cached by source edge node based on directory table
        {
            // NOTE: we do NOT add/update latest local uncached popularity if key has directory info for the source edge node, to avoid duplicate admission on the source edge node, after directory update request with is_admit = true clearing preserved edgeset in the beacon node, yet the source edge node has NOT admitted the object into local edge cache to clear local uncached metadata
            // NOTE: this will delay the latest local uncached popularity of newly-evicted keys, after the source edge node has evicted the object and updated local uncached metadata for metadata preservation, yet directory update request with is_admit = false has NOT cleared directory info in the beacon node -> BUT acceptable as this is a corner case (newly-evicted keys are NOT popular enough and NO need to add/update latest local uncached popularity in most cases after eviciton)
            is_tracked_by_source_edge_node = false;

            #ifdef ENABLE_FAST_PATH_PLACEMENT
            // With dirinfo of sender (i.e., finish placement notification in sender)
            need_fast_path_hint = false;
            #endif
        }

        perkey_preserved_edgeset_t::const_iterator perkey_preserved_edgeset_const_iter = perkey_preserved_edgeset_.find(key);
        if (perkey_preserved_edgeset_const_iter != perkey_preserved_edgeset_.end() && perkey_preserved_edgeset_const_iter->second.isPreserved(source_edge_idx)) // Ignore local uncached popularity if source edge node is in preserved edgeset
        {
            // NOTE: we do NOT add/update latest local uncached popularity if source edge node is in preserved edgeset for the given key to avoid duplication admission on the source edge node
            is_tracked_by_source_edge_node = false;

            #ifdef ENABLE_FAST_PATH_PLACEMENT
            // In preserved edgeset (i.e., being admited)
            need_fast_path_hint = false;
            #endif
        }

        Popularity local_uncached_popularity = 0.0;
        ObjectSize object_size = 0;
        perkey_benefit_popularity_table_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        if (is_tracked_by_source_edge_node) // Add/update latest local uncached popularity for key in source edge node into new/existing aggregated uncached popularity
        {
            local_uncached_popularity = collected_popularity.getLocalUncachedPopularity();
            object_size = collected_popularity.getObjectSize();
            if (perkey_benefit_popularity_iter == perkey_benefit_popularity_table_.end()) // New key
            {
                addAggregatedUncachedPopularityForNewKey_(key, source_edge_idx, local_uncached_popularity, object_size, is_global_cached);
            }
            else // Existing key
            {
                // Add/update local uncached popularity of source edge node
                bool is_successful = updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, object_size, is_global_cached);
                assert(is_successful);
            }

            // NOTE: we try to discard global less populart objects even if we update aggregated uncached popularity for an existing key (besides add aggregated uncached popularity for a new key), as AggregatedUncachedPopularity could add new local uncached popularity for the existing key
            // NOTE: this is different from local uncached metadata, which will NOT increase cache size usage if update local uncached metadata for an existing key (ONLY trigger removal if add local uncached metadata for a new key)

            // Discard the objects with small max admission benefits if popularity aggregation capacity bytes are used up
            discardGlobalLessPopularObjects_();
        }
        else // Remove old local uncached popularity for key in source edge node from existing aggregated uncached popularity if any
        {
            // NOTE: NO need to remove old local uncached popularity if without aggregated uncached popularity for the key

            if (perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end()) // Existing key
            {
                // Remove old local uncached popularity of source edge node if any
                // NOTE: local_uncached_popularity and object_size are NOT used when is_tracked_by_source_edge_node = false
                bool is_successful = updateAggregatedUncachedPopularityForExistingKey_(key, source_edge_idx, is_tracked_by_source_edge_node, local_uncached_popularity, object_size, is_global_cached);
                #ifdef ENABLE_FAST_PATH_PLACEMENT
                if (is_successful)
                {
                    // In bitmap of aggregated uncached popularity that needs to be removed (i.e., tracked before)
                    need_fast_path_hint = false;
                }
                #else
                UNUSED(is_successful);
                #endif
            }

            // NOTE: NO need to try to discard global less popular objects here, as removing old local uncached popularity if any will NEVER increase cache size usage
        }

        #ifdef ENABLE_FAST_PATH_PLACEMENT
        // Prepare the correpsonding FastPathHint if necessary
        if (need_fast_path_hint) // Key is possibly the first cache miss w/o objsize
        {
            // Prepare sum_local_uncached_popularity (NOTE: sum_local_uncached_popularity MUST NOT include local uncached popularity of sender edge node)
            Popularity tmp_sum_local_uncached_popularity = 0.0;
            AggregatedUncachedPopularity tmp_aggregated_uncached_popularity;
            bool tmp_has_aggregated_uncached_popularity = getAggregatedUncachedPopularity_(key, tmp_aggregated_uncached_popularity);
            if (tmp_has_aggregated_uncached_popularity)
            {
                tmp_sum_local_uncached_popularity = tmp_aggregated_uncached_popularity.getSumLocalUncachedPopularity();
            }
            
            // Approximately estimate margin bytes of popularity aggregation capacity limitation
            uint64_t tmp_margin_bytes = 0;
            if (tmp_has_aggregated_uncached_popularity)
            {
                tmp_margin_bytes = sizeof(uint32_t) + sizeof(Popularity); // Sender edgeidx + local uncached popularity
            }
            else
            {
                tmp_margin_bytes = sizeof(DeltaReward); // Max admission benefit
                tmp_margin_bytes += key.getKeyLength() + sizeof(ObjectSize) + sizeof(Popularity) + (sizeof(uint32_t) + sizeof(Popularity)) + (edgecnt_ * sizeof(bool) - 1) / 8 + 1 + sizeof(uint32_t); // Aggregated uncached popularity
                tmp_margin_bytes += key.getKeyLength() + sizeof(benefit_popularity_multimap_iter_t); // Key and iterator for lookup table
            }

            // Prepare smallest_max_admission_benefit
            DeltaReward smallest_max_admission_benefit = MIN_ADMISSION_BENEFIT; // NOTE: the smallest max_admission_benefit == MIN_ADMISSION_BENEFIT (i.e., 0.0) means that beacon has sufficient space for popularity aggregation and the current object MUST be global popular to trigger fast-path placement calculation
            if (size_bytes_ + tmp_margin_bytes >= popularity_aggregation_capacity_bytes_ && benefit_popularity_multimap_.size() > 0)
            {
                smallest_max_admission_benefit = benefit_popularity_multimap_.begin()->first; // NOTE: benefit_popularity_multimap_ follows ascending order of max admission benefit by default
            }

            fast_path_hint = FastPathHint(tmp_sum_local_uncached_popularity, smallest_max_admission_benefit);
            assert(fast_path_hint.isValid());
        }
        #endif

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    void PopularityAggregator::updatePreservedEdgesetForPlacement(const Key& key, const Edgeset& placement_edgeset, const bool& is_global_cached)
    {
        checkPointers_();

        // Acquire a write lock to preserve edge ndoes for non-blocking placement deployment atomically
        const std::string context_name = "PopularityAggregator::updatePreservedEdgesetForPlacement()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        // Preserve edge nodes of placement edgeset to ignore subsequent local uncached popularities from them
        perkey_preserved_edgeset_t::iterator perkey_preserved_edgeset_iter = perkey_preserved_edgeset_.find(key);
        if (perkey_preserved_edgeset_iter == perkey_preserved_edgeset_.end())
        {
            perkey_preserved_edgeset_iter = perkey_preserved_edgeset_.insert(std::pair(key, PreservedEdgeset(edgecnt_))).first;
            size_bytes_ += key.getKeyLength(); // Key
            size_bytes_ += perkey_preserved_edgeset_iter->second.getSizeForCapacity(); // Preserved edgeset
        }
        assert(perkey_preserved_edgeset_iter != perkey_preserved_edgeset_.end());
        perkey_preserved_edgeset_iter->second.preserveEdgesetForPlacement(placement_edgeset);

        // Get aggregated uncached popularity for local uncached popularity removal and max admission benefit update
        AggregatedUncachedPopularity existing_aggregated_uncached_popularity;
        bool has_aggregated_uncached_popularity = getAggregatedUncachedPopularity_(key, existing_aggregated_uncached_popularity);
        // (OBSOLETE) NOTE: key MUST have aggregated uncached popularity if with placement edgeset
        //assert(has_aggregated_uncached_popularity == true);
        // NOTE: the corresponding aggregated uncached popularity may already be removed, if all local uncached popularities are removed during popularity aggregation (is_tracked = false) or after placement calculation (clear placement edgeset), else if capacity bytes for popularity aggregation are used up
        if (has_aggregated_uncached_popularity == false)
        {
            std::ostringstream oss;
            oss << "aggregated uncached popularity for key " << key.getKeystr() << " has already been erased due to multi-threading access -> NO need to clear local cached popularities for placement";
            Util::dumpInfoMsg(instance_name_, oss.str());
        }
        else
        {
            // Remove local uncached popularities of the edge nodes from aggregated uncached popularity to avoid duplicate placement
            bool is_aggregated_uncached_popularity_empty = false;
            is_aggregated_uncached_popularity_empty = existing_aggregated_uncached_popularity.clearForPlacement(placement_edgeset);

            // Update benefit-popularity multimap and per-key lookup table for existing yet updated aggregated uncached popularity
            //const bool is_size_bytes_increased = false or true; // NOTE: NOT sure if size_bytes_ is increased or decreased here, as we may add preserved edgeset yet release local uncached popularities
            updateBenefitPopularityForExistingKey_(key, existing_aggregated_uncached_popularity, is_aggregated_uncached_popularity_empty, is_global_cached);
        }

        // Discard the objects with small max admission benefits if popularity aggregation capacity bytes are used up
        discardGlobalLessPopularObjects_();

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    void PopularityAggregator::clearPreservedEdgesetAfterAdmission(const Key& key, const uint32_t& source_edge_idx)
    {
        checkPointers_();

        // Acquire a write lock to clear preserved edge ndoes from aggregated uncached popularity atomically
        const std::string context_name = "PopularityAggregator::clearPreservedEdgesetAfterAdmission()";
        rwlock_for_popularity_aggregator_->acquire_lock(context_name);

        // Clear preserved edge node for the source edge node after admission notification
        perkey_preserved_edgeset_t::iterator perkey_preserved_edgeset_iter = perkey_preserved_edgeset_.find(key);
        if (perkey_preserved_edgeset_iter != perkey_preserved_edgeset_.end())
        {
            bool is_empty = perkey_preserved_edgeset_iter->second.clearPreservedEdgeNode(source_edge_idx);
            if (is_empty) // All preserved edge nodes have received local/remote placement notification
            {
                perkey_preserved_edgeset_.erase(perkey_preserved_edgeset_iter);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Key " << key.getKeystr() << " has NO preserved edgeset for non-blocking placement deployment";
            Util::dumpWarnMsg(instance_name_, oss.str());
        }

        // NOTE: NO need to try to discard objects for popularity aggregation capacity bytes, as size_byte_ will NOT increase here

        rwlock_for_popularity_aggregator_->unlock(context_name);
        return;
    }

    // Utils

    bool PopularityAggregator::getAggregatedUncachedPopularity_(const Key& key, AggregatedUncachedPopularity& aggregated_uncached_popularity) const
    {
        // NOTE: we have already acquired a read lock in getAggregatedUncachedPopularity() or updatePreservedEdgesetForPlacement() or updateAggregatedUncachedPopularityForExistingKey_() (from updateAggregatedUncachedPopularity()) for thread safety

        bool is_found = false;

        perkey_benefit_popularity_table_const_iter_t perkey_benefit_popularity_const_iter = perkey_benefit_popularity_table_.find(key);
        if (perkey_benefit_popularity_const_iter != perkey_benefit_popularity_table_.end())
        {
            benefit_popularity_multimap_iter_t benefit_popularity_iter = perkey_benefit_popularity_const_iter->second;
            assert(benefit_popularity_iter != benefit_popularity_multimap_.end());
            aggregated_uncached_popularity = benefit_popularity_iter->second; // Deep copy

            is_found = true;
        }

        return is_found;
    }

    void PopularityAggregator::addAggregatedUncachedPopularityForNewKey_(const Key& key, const uint32_t& source_edge_idx, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        // Prepare new aggregated uncached popularity for the new key
        AggregatedUncachedPopularity new_aggregated_uncached_popularity(key, edgecnt_);
        new_aggregated_uncached_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_, object_size);

        // Update benefit-popularity multimap and per-key lookup table for new aggregated uncached popularity
        addBenefitPopularityForNewKey_(key, new_aggregated_uncached_popularity, is_global_cached);

        return;
    }

    bool PopularityAggregator::updateAggregatedUncachedPopularityForExistingKey_(const Key& key, const uint32_t& source_edge_idx, const bool& is_tracked_by_source_edge_node, const Popularity& local_uncached_popularity, const ObjectSize& object_size, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        bool is_successful = false; // Is add/remove local uncached popularity successfully

        // Find the aggregated uncached popularity for the existing key
        AggregatedUncachedPopularity existing_aggregated_uncached_popularity;
        bool has_aggregatd_uncached_popularity = getAggregatedUncachedPopularity_(key, existing_aggregated_uncached_popularity); // Deep copy
        assert(has_aggregatd_uncached_popularity == true); // For existing key

        // Update the aggregated uncached popularity for the existing key
        bool is_aggregated_uncached_popularity_empty = false;
        if (is_tracked_by_source_edge_node) // Add/update local uncached popularity of source edge node
        {
            existing_aggregated_uncached_popularity.update(source_edge_idx, local_uncached_popularity, topk_edgecnt_, object_size);
            is_successful = true;
        }
        else // Clear aggregated uncached popularity to remove old local uncached popularity of source edge node if any
        {
            is_aggregated_uncached_popularity_empty = existing_aggregated_uncached_popularity.clear(source_edge_idx, is_successful);
        }

        // Update benefit-popularity multimap and per-key lookup table for existing yet updated aggregated uncached popularity
        //const bool is_size_bytes_increased = is_tracked_by_source_edge_node;
        updateBenefitPopularityForExistingKey_(key, existing_aggregated_uncached_popularity, is_aggregated_uncached_popularity_empty, is_global_cached);

        return;
    }

    void PopularityAggregator::addBenefitPopularityForNewKey_(const Key& key, const AggregatedUncachedPopularity& new_aggregated_uncached_popularity, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() for thread safety

        perkey_benefit_popularity_table_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        assert(perkey_benefit_popularity_iter == perkey_benefit_popularity_table_.end()); // For new key

        // Prepare new max admission benefit for the new key
        DeltaReward new_max_admission_benefit = new_aggregated_uncached_popularity.calcMaxAdmissionBenefit(is_global_cached);

        // Insert new aggregated uncached popularity and new max admission benefit into benefit_popularity_multimap_ for the new key
        benefit_popularity_multimap_iter_t new_benefit_popularity_iter = benefit_popularity_multimap_.insert(std::pair(new_max_admission_benefit, new_aggregated_uncached_popularity));
        size_bytes_ += sizeof(DeltaReward); // Max admission benefit
        size_bytes_ += new_aggregated_uncached_popularity.getSizeForCapacity(); // Aggrgated uncached popularity

        // Insert new key with new benfit_popularity_iter into perkey_benefit_popularity_table_
        perkey_benefit_popularity_table_.insert(std::pair(key, new_benefit_popularity_iter));
        size_bytes_ += key.getKeyLength(); // Key
        size_bytes_ += sizeof(benefit_popularity_multimap_iter_t); // Lookup table iterator

        return;
    }

    void PopularityAggregator::updateBenefitPopularityForExistingKey_(const Key& key, const AggregatedUncachedPopularity& updated_aggregated_uncached_popularity, const bool& is_aggregated_uncached_popularity_empty, const bool& is_global_cached)
    {
        // NOTE: we have already acquired a write lock in updatePreservedEdgesetForPlacement() and updateAggregatedUncachedPopularityForExistingKey_() (from updateAggregatedUncachedPopularity()) for thread safety
        
        perkey_benefit_popularity_table_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(key);
        assert(perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end()); // For existing key
        benefit_popularity_multimap_iter_t old_benefit_popularity_iter = perkey_benefit_popularity_iter->second;
        assert(old_benefit_popularity_iter != benefit_popularity_multimap_.end());

        // Verify cache size usage changes (OBSOLETE)
        const uint64_t old_aggregated_uncached_popularity_size_bytes = old_benefit_popularity_iter->second.getSizeForCapacity();
        const uint64_t new_aggregated_uncached_popularity_size_bytes = updated_aggregated_uncached_popularity.getSizeForCapacity();
        /*if (is_size_bytes_increased)
        {
            assert(new_aggregated_uncached_popularity_size_bytes >= old_aggregated_uncached_popularity_size_bytes); // Maybe increase cache size usage
        }
        else
        {
            assert(new_aggregated_uncached_popularity_size_bytes <= old_aggregated_uncached_popularity_size_bytes); // NEVER increase cache size usage
        }*/

        // Remove old benefit-popularity pair for the given key
        size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(DeltaReward)); // Old max admission benefit
        size_bytes_ = Util::uint64Minus(size_bytes_, old_aggregated_uncached_popularity_size_bytes); // Old aggregated uncached popularity
        benefit_popularity_multimap_.erase(old_benefit_popularity_iter);

        if (is_aggregated_uncached_popularity_empty) // NO need aggregated uncached popularity for the given key
        {
            perkey_benefit_popularity_table_.erase(perkey_benefit_popularity_iter);
            size_bytes_ = Util::uint64Minus(size_bytes_, key.getKeyLength()); // Erased key
            size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(benefit_popularity_multimap_iter_t)); // Erased lookup table iterator
        }
        else // Still need aggregated uncached popularity for the given key
        {
            // Calculate a new max admission benefit for the existing key
            DeltaReward new_max_admission_benefit = updated_aggregated_uncached_popularity.calcMaxAdmissionBenefit(is_global_cached);

            // Update benefit_popularity_multimap_ for the new max admission benefit of the existing key
            benefit_popularity_multimap_iter_t new_benefit_popularity_iter = benefit_popularity_multimap_.insert(std::pair(new_max_admission_benefit, updated_aggregated_uncached_popularity));
            size_bytes_ += sizeof(DeltaReward); // New max admission benefit
            size_bytes_ += new_aggregated_uncached_popularity_size_bytes; // New aggregated uncached popularity

            // Update perkey_benefit_popularity_table_ for the new benefit_popularity_iter of the existing key
            perkey_benefit_popularity_iter->second = new_benefit_popularity_iter;
        }

        return;
    }

    void PopularityAggregator::discardGlobalLessPopularObjects_()
    {
        // NOTE: we have already acquired a write lock in updateAggregatedUncachedPopularity() or updatePreservedEdgesetForPlacement() for thread safety

        // NOTE: we should NOT discard preserved edge nodes for popularity aggregation capacity bytes!!!

        while (true)
        {
            // Check if popularity aggregation capacity bytes are used up
            if (size_bytes_ <= popularity_aggregation_capacity_bytes_)
            {
                break;
            }
            else
            {
                // Find the object with the minimum max admission benefit
                benefit_popularity_multimap_iter_t min_benefit_popularity_iter = benefit_popularity_multimap_.begin();
                assert(min_benefit_popularity_iter != benefit_popularity_multimap_.end());
                AggregatedUncachedPopularity tmp_aggregated_uncached_popularity = min_benefit_popularity_iter->second; // Aggregated uncached popularity with minimum max admission benefit
                
                // Remove it from benefit_popularity_multimap_
                benefit_popularity_multimap_.erase(min_benefit_popularity_iter);
                size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(DeltaReward)); // Max admission benefit
                size_bytes_ = Util::uint64Minus(size_bytes_, tmp_aggregated_uncached_popularity.getSizeForCapacity()); // Aggrgated uncached popularity
                
                // Remove it from perkey_benefit_popularity_table_
                Key tmp_key = tmp_aggregated_uncached_popularity.getKey(); // Key with minimum max admission benefit
                perkey_benefit_popularity_table_iter_t perkey_benefit_popularity_iter = perkey_benefit_popularity_table_.find(tmp_key);
                assert(perkey_benefit_popularity_iter != perkey_benefit_popularity_table_.end());
                perkey_benefit_popularity_table_.erase(perkey_benefit_popularity_iter);
                size_bytes_ = Util::uint64Minus(size_bytes_, tmp_key.getKeyLength()); // Key
                size_bytes_ = Util::uint64Minus(size_bytes_, sizeof(benefit_popularity_multimap_iter_t)); // Lookup table iterator
            }
        }

        return;
    }

    void PopularityAggregator::checkPointers_() const
    {
        assert(rwlock_for_popularity_aggregator_ != NULL);
        return;
    }
}
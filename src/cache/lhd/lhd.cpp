#include <assert.h>
#include <sstream>
#include "lhd.hpp"
#include "rand.hpp"

#include "common/util.h"

namespace repl {

const uint32_t LHD::SINGLE_TENANT_APPID = 0;

const std::string LHD::kClassName("LHD");

// NOTE: we just keep default ASSOCIATIVITY and ADMISSIONS
LHD::LHD(const uint64_t& capacity_bytes)
    : recentlyAdmitted(ADMISSIONS, covered::Key()) {
    nextReconfiguration = ACCS_PER_RECONFIGURATION;
    explorerBudget = capacity_bytes * EXPLORER_BUDGET_FRACTION;
    
    kvpair_bytes_ = 0;
    internal_bytes_ = 0;
    
    for (uint32_t i = 0; i < NUM_CLASSES; i++) {
        classes.push_back(Class());
        auto& cl = classes.back();
        cl.hits.resize(MAX_AGE, 0);
        cl.evictions.resize(MAX_AGE, 0);
        cl.hitDensities.resize(MAX_AGE, 0);

        // Update internal_bytes_ for classes
        internal_bytes_ = covered::Util::uint64Add(internal_bytes_, cl.getSizeForCapacity());
    }

    // Initialize policy to ~GDSF by default.
    for (uint32_t c = 0; c < NUM_CLASSES; c++) {
        for (age_t a = 0; a < MAX_AGE; a++) {
            classes[c].hitDensities[a] =
                1. * (c + 1) / (a + 1);
        }
    }

    const uint64_t cache_usage_after_initialization = getSizeForCapacity();
    if (cache_usage_after_initialization >= capacity_bytes)
    {
        std::ostringstream oss;
        oss << "too small capacity (" << capacity_bytes << " bytes) for LHD cache, which already uses " << cache_usage_after_initialization << " bytes after initialization!";
        covered::Util::dumpErrorMsg(kClassName, oss.str());
        exit(1);
    }
}

bool LHD::exists(const covered::Key& key) const
{
    const bool is_local_cached = (indices.find(key) != indices.end());
    return is_local_cached;
}

bool LHD::get(const covered::Key& key, covered::Value& value)
{
    bool is_local_cached = false;
    std::unordered_map<covered::Key, uint64_t, covered::KeyHasher>::const_iterator indices_const_iter = indices.find(key);
    if (indices_const_iter == indices.end()) // Key is NOT cached
    {
        is_local_cached = false;
    }
    else // Key is cached
    {
        is_local_cached = true;

        // Update hitcnt for the corresponding class
        assert(indices_const_iter->second < tags.size());
        Tag* tag = &tags[indices_const_iter->second];
        assert(tag->key == key);
        auto age = getAge(*tag);
        auto& cl = getClass(*tag);
        cl.hits[age] += 1;

        // NOTE: NO need to update explorerBudget if key is an explorer, as we do NOT update value and hence NO need to release original object size

        // Update the past two access ages
        tag->lastLastHitAge = tag->lastHitAge;
        tag->lastHitAge = age;

        // Get value from Tag
        value = tag->value;

        // Update metadata of Tag and trigger reconfiguration if necessary after get
        afterGet_(tag, SINGLE_TENANT_APPID);
    }
    return is_local_cached;
}

void LHD::admit(const covered::Key& key, const covered::Value& value)
{
    std::unordered_map<covered::Key, uint64_t, covered::KeyHasher>::const_iterator indices_const_iter = indices.find(key);
    if (indices_const_iter != indices.end()) // Already cached
    {
        std::ostringstream oss;
        oss << "key " << key.getKeyDebugstr() << " already exists in LHD cache (cached objcnt: " << indices.size() << ") for admit()";
        covered::Util::dumpWarnMsg(kClassName, oss.str());
        return;
    }
    else // Not cached yet
    {
        // Create a new Tag for kvpair and metadata
        tags.push_back(Tag{});
        Tag* tag = &tags.back();
        assert(tag != NULL);

        // Update lookup table
        indices_const_iter = indices.insert(std::pair(key, tags.size() - 1)).first;
        assert(indices_const_iter != indices.end());
        
        // Update newly created Tag
        tag->lastLastHitAge = MAX_AGE;
        tag->lastHitAge = 0;
        tag->key = key;
        tag->value = value;

        // Update Tag (kvpair and metadata), mark explorer, update recently-admited vector, and trigger reconfiguration if necessary after admit
        const uint32_t object_size = key.getKeyLength() + value.getValuesize();
        afterAdmitOrUpdate_(tag, key, SINGLE_TENANT_APPID, object_size, true);

        // Update kvpair_bytes_ for newly-admited object
        kvpair_bytes_ = covered::Util::uint64Add(kvpair_bytes_, object_size);

        // Update internal_bytes_ for tags and indices
        internal_bytes_ = covered::Util::uint64Add(internal_bytes_, tag->getSizeForCapacity()); // kvpair and metadata in tags
        internal_bytes_ = covered::Util::uint64Add(internal_bytes_, key.getKeyLength() + sizeof(uint64_t)); // key and tag index in indices
    }
    return;
}

bool LHD::update(const covered::Key& key, const covered::Value& value)
{
    bool is_local_cached = false;

    std::unordered_map<covered::Key, uint64_t, covered::KeyHasher>::const_iterator indices_const_iter = indices.find(key);
    if (indices_const_iter != indices.end()) // Already cached
    {
        is_local_cached = true;

        // Update hitcnt for the correpsonding class
        assert(indices_const_iter->second < tags.size());
        Tag* tag = &tags[indices_const_iter->second];
        assert(tag->key == key);
        auto age = getAge(*tag);
        auto& cl = getClass(*tag);
        cl.hits[age] += 1;
        
        // Update value in Tag
        const uint64_t original_value_size = tag->value.getValuesize();
        const uint64_t original_tag_size = tag->getSizeForCapacity();
        tag->value = value;

        if (tag->explorer) { explorerBudget += tag->size; } // Release original object size for explorer budge, while latest object size will be allocated from explorer budge in afterAdmitOrUpdate_
        
        // Update the past two access ages
        tag->lastLastHitAge = tag->lastHitAge;
        tag->lastHitAge = age;

        // Update Tag (kvpair and metadata), mark explorer, update recently-admited vector, and trigger reconfiguration if necessary after update
        const uint32_t object_size = key.getKeyLength() + value.getValuesize();
        afterAdmitOrUpdate_(tag, key, SINGLE_TENANT_APPID, object_size, false);

        // Update kvpair_bytes_ for updated value
        const uint64_t latest_value_size = value.getValuesize();
        if (latest_value_size > original_value_size) // Value size is increased
        {
            kvpair_bytes_ = covered::Util::uint64Add(kvpair_bytes_, latest_value_size - original_value_size);
        }
        else if (latest_value_size < original_value_size) // Value size is decreased
        {
            kvpair_bytes_ = covered::Util::uint64Minus(kvpair_bytes_, original_value_size - latest_value_size);
        }

        // Update internal_bytes_ for tags
        const uint64_t latest_tag_size = tag->getSizeForCapacity();
        if (latest_tag_size > original_tag_size) // Tag size is increased
        {
            internal_bytes_ = covered::Util::uint64Add(internal_bytes_, latest_tag_size - original_tag_size);
        }
        else if (latest_tag_size < original_tag_size) // Tag size is decreased
        {
            internal_bytes_ = covered::Util::uint64Minus(internal_bytes_, original_tag_size - latest_tag_size);
        }
    }

    return is_local_cached;
}

bool LHD::rank(covered::Key& key) {
    bool has_victim_key = false;

    if (tags.size() == 0)
    {
        return has_victim_key;
    }

    uint64_t victim_idx = 0;
    rank_t victimRank = std::numeric_limits<rank_t>::max();

    // Sample few candidates early in the trace so that we converge
    // quickly to a reasonable policy.
    //
    // This is a hack to let us have shorter warmup so we can evaluate
    // a longer fraction of the trace; doesn't belong in a real
    // system.
    uint32_t candidates =
        (numReconfigurations > 50)?
        ASSOCIATIVITY : 8;

    for (uint32_t i = 0; i < candidates; i++) {
        auto idx = rand.next() % tags.size();
        auto& tag = tags[idx];
        rank_t rank = getHitDensity(tag);

        if (!has_victim_key || rank < victimRank) {
            victim_idx = idx;
            victimRank = rank;

            key = tag.key;
            has_victim_key = true;
        }
    }

    for (uint32_t i = 0; i < ADMISSIONS; i++) {
        auto itr = indices.find(recentlyAdmitted[i]);
        if (itr == indices.end()) { continue; }

        auto idx = itr->second;
        auto& tag = tags[idx];
        assert(tag.key == recentlyAdmitted[i]);
        rank_t rank = getHitDensity(tag);

        if (!has_victim_key || rank < victimRank) {
            victim_idx = idx;
            victimRank = rank;

            key = tag.key;
            has_victim_key = true;
        }
    }

    UNUSED(victim_idx);

    //assert(has_victim_key);
    if (has_victim_key) // Update ewma victim hit density if victim is found
    {
        ewmaVictimHitDensity = EWMA_DECAY * ewmaVictimHitDensity + (1 - EWMA_DECAY) * victimRank;
    }

    return has_victim_key;
}

bool LHD::replaced(const covered::Key& key, covered::Value& value) {
    bool is_evict = false;

    auto itr = indices.find(key);
    //assert(itr != indices.end());
    if (itr != indices.end())
    {
        auto index = itr->second;

        // Get value of victim
        auto& tag = tags[index];
        assert(tag.key == key);
        value = tag.value;
        const uint64_t victim_tag_size = tag.getSizeForCapacity();
        const uint64_t victim_value_size = tag.value.getValuesize();

        // Record stats before removing item
        auto age = getAge(tag);
        auto& cl = getClass(tag);
        cl.evictions[age] += 1;

        if (tag.explorer) { explorerBudget += tag.size; } // Release object size from explorerBudget due to eviction

        // Remove tag for replaced item and update index
        indices.erase(itr);
        tags[index] = tags.back(); // Replace tag of the victim as the tag of the last item
        tags.pop_back(); // NOTE: will decrease tags.size() by 1

        if (index < tags.size()) { // If the victim is NOT the last item
            // Update index in lookup table for the last item key
            const covered::Key last_item_key = tags[index].key;
            std::unordered_map<covered::Key, uint64_t, covered::KeyHasher>::iterator last_item_indices_iter = indices.find(last_item_key);
            assert(last_item_indices_iter != indices.end());
            last_item_indices_iter->second = index;
        }

        // Update kvpair_bytes_ for evicted object
        kvpair_bytes_ = covered::Util::uint64Minus(kvpair_bytes_, key.getKeyLength() + victim_value_size);

        // Update internal_bytes_ for tags and indices
        internal_bytes_ = covered::Util::uint64Minus(internal_bytes_, victim_tag_size + key.getKeyLength() + sizeof(uint64_t));

        is_evict = true;
    }

    return is_evict;
}

uint64_t LHD::getSizeForCapacity() const
{
    uint64_t total_size_usage = covered::Util::uint64Add(sizeof(uint64_t), internal_bytes_); // sizeof(kvpair_bytes_) + internal_bytes_
    total_size_usage = covered::Util::uint64Add(total_size_usage, sizeof(timestamp_t) * 3 + sizeof(int) + sizeof(rank_t) * 2); // sizeof(timestamp) + sizeof(nextReconfiguration) + sizeof(numReconfigurations) + sizeof(ageCoarseningShift) + sizeof(ewmaNumObjects) + sizeof(ewmaNumObjectsMass)
    total_size_usage = covered::Util::uint64Add(total_size_usage, sizeof(uint64_t) + sizeof(int) + sizeof(rank_t) + sizeof(int64_t)); // sizeof(overflows) + sizeof(recentlyAdmittedHead) + sizeof(ewmaVictimHitDensity) + sizeof(explorerBudget)
    for (uint32_t i = 0; i < recentlyAdmitted.size(); i++)
    {
        total_size_usage = covered::Util::uint64Add(total_size_usage, recentlyAdmitted[i].getKeyLength()); // recentlyAdmitted
    }
    return total_size_usage;
}

void LHD::reconfigure() {
    rank_t totalHits = 0;
    rank_t totalEvictions = 0;
    for (auto& cl : classes) {
        updateClass(cl);
        totalHits += cl.totalHits;
        totalEvictions += cl.totalEvictions;
    }

    adaptAgeCoarsening();
        
    modelHitDensity();

    // Just printfs ...
    for (uint32_t c = 0; c < classes.size(); c++) {
        auto& cl = classes[c];
        // printf("Class %d | hits %g, evictions %g, hitRate %g\n",
        //        c,
        //        cl.totalHits, cl.totalEvictions,
        //        cl.totalHits / (cl.totalHits + cl.totalEvictions));

        dumpClassRanks(cl);
    }
    printf("LHD | hits %g, evictions %g, hitRate %g | overflows %lu (%g) | cumulativeHitRate nan\n",
           totalHits, totalEvictions,
           totalHits / (totalHits + totalEvictions),
           overflows,
           1. * overflows / ACCS_PER_RECONFIGURATION);

    overflows = 0;
}

void LHD::updateClass(Class& cl) {
    cl.totalHits = 0;
    cl.totalEvictions = 0;

    for (age_t age = 0; age < MAX_AGE; age++) {
        cl.hits[age] *= EWMA_DECAY;
        cl.evictions[age] *= EWMA_DECAY;

        cl.totalHits += cl.hits[age];
        cl.totalEvictions += cl.evictions[age];
    }
}

void LHD::modelHitDensity() {
    for (uint32_t c = 0; c < classes.size(); c++) {
        rank_t totalEvents = classes[c].hits[MAX_AGE-1] + classes[c].evictions[MAX_AGE-1];
        rank_t totalHits = classes[c].hits[MAX_AGE-1];
        rank_t lifetimeUnconditioned = totalEvents;

        // we use a small trick here to compute expectation in O(N) by
        // accumulating all values at later ages in
        // lifetimeUnconditioned.
        
        for (age_t a = MAX_AGE - 2; a < MAX_AGE; a--) {
            totalHits += classes[c].hits[a];
            
            totalEvents += classes[c].hits[a] + classes[c].evictions[a];

            lifetimeUnconditioned += totalEvents;

            if (totalEvents > 1e-5) {
                classes[c].hitDensities[a] = totalHits / lifetimeUnconditioned;
            } else {
                classes[c].hitDensities[a] = 0.;
            }
        }
    }
}

void LHD::dumpClassRanks(Class& cl) {
    if (!DUMP_RANKS) { return; }
    
    // float objectAvgSize = cl.sizeAccumulator / cl.totalHits; // + cl.totalEvictions);
    float objectAvgSize = 1. * kvpair_bytes_ / indices.size();
    rank_t left;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Ranks for avg object (" << objectAvgSize << "): ";
    for (age_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      rank_t density = cl.hitDensities[a] / objectAvgSize;
      rankStr << density << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Hits: ";
    for (uint32_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      rankStr << cl.hits[a] << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;

    left = cl.totalHits + cl.totalEvictions;
    std::cout << "Evictions: ";
    for (uint32_t a = 0; a < MAX_AGE; a++) {
      std::stringstream rankStr;
      rankStr << cl.evictions[a] << ", ";
      std::cout << rankStr.str();

      left -= cl.hits[a] + cl.evictions[a];
      if (rankStr.str() == "0, " && left < 1e-2) {
        break;
      }
    }
    std::cout << std::endl;
}

// this happens very rarely!
//
// it is simple enough to set the age coarsening if you know roughly
// how big your objects are. to make LHD run on different traces
// without needing to configure this, we set the age coarsening
// automatically near the beginning of the trace.
void LHD::adaptAgeCoarsening() {
    ewmaNumObjects *= EWMA_DECAY;
    ewmaNumObjectsMass *= EWMA_DECAY;

    ewmaNumObjects += indices.size();
    ewmaNumObjectsMass += 1.;

    rank_t numObjects = ewmaNumObjects / ewmaNumObjectsMass;

    rank_t optimalAgeCoarsening = 1. * numObjects / (AGE_COARSENING_ERROR_TOLERANCE * MAX_AGE);

    // Simplify. Just do this once shortly after the trace starts and
    // again after 25 iterations. It only matters that we are within
    // the right order of magnitude to avoid tons of overflows.
    if (numReconfigurations == 5 || numReconfigurations == 25) {
        uint32_t optimalAgeCoarseningLog2 = 1;

        while ((1 << optimalAgeCoarseningLog2) < optimalAgeCoarsening) {
            optimalAgeCoarseningLog2 += 1;
        }

        int32_t delta = optimalAgeCoarseningLog2 - ageCoarseningShift;
        ageCoarseningShift = optimalAgeCoarseningLog2;
        
        // increase weight to delay another shift for a while
        ewmaNumObjects *= 8;
        ewmaNumObjectsMass *= 8;
        
        // compress or stretch distributions to approximate new scaling
        // regime
        if (delta < 0) {
            // stretch
            for (auto& cl : classes) {
                for (age_t a = MAX_AGE >> (-delta); a < MAX_AGE - 1; a++) {
                    cl.hits[MAX_AGE - 1] += cl.hits[a];
                    cl.evictions[MAX_AGE - 1] += cl.evictions[a];
                }
                for (age_t a = MAX_AGE - 2; a < MAX_AGE; a--) {
                    cl.hits[a] = cl.hits[a >> (-delta)] / (1 << (-delta));
                    cl.evictions[a] = cl.evictions[a >> (-delta)] / (1 << (-delta));
                }
            }
        } else if (delta > 0) {
            // compress
            for (auto& cl : classes) {
                for (age_t a = 0; a < MAX_AGE >> delta; a++) {
                    cl.hits[a] = cl.hits[a << delta];
                    cl.evictions[a] = cl.evictions[a << delta];
                    for (int i = 1; i < (1 << delta); i++) {
                        cl.hits[a] += cl.hits[(a << delta) + i];
                        cl.evictions[a] += cl.evictions[(a << delta) + i];
                    }
                }
                for (age_t a = (MAX_AGE >> delta); a < MAX_AGE - 1; a++) {
                    cl.hits[a] = 0;
                    cl.evictions[a] = 0;
                }
            }
        }
    }
    
    printf("LHD at %lu | ageCoarseningShift now %lu | num objects %g | optimal age coarsening %g | current age coarsening %g\n",
           timestamp, ageCoarseningShift,
           numObjects,
           optimalAgeCoarsening,
           1. * (1 << ageCoarseningShift));
}

void LHD::afterAdmitOrUpdate_(Tag* tag, const covered::Key& key, const uint32_t& appid, const uint32_t& object_size, const bool& is_admit)
{
    assert(tag != NULL);

    tag->timestamp = timestamp;
    tag->app = appid % APP_CLASSES;
    tag->size = object_size;

    // with some probability, some candidates will never be evicted
    // ... but limit how many resources we spend on doing this
    bool explore = (rand.next() % EXPLORE_INVERSE_PROBABILITY) == 0;
    if (explore && explorerBudget > 0 && numReconfigurations < 50) {
        tag->explorer = true;
        explorerBudget -= tag->size;
    } else {
        tag->explorer = false;
    }

    // If this candidate looks like something that should be
    // evicted, track it.
    if (is_admit && !explore && getHitDensity(*tag) < ewmaVictimHitDensity) {
        recentlyAdmitted[recentlyAdmittedHead++ % ADMISSIONS] = key;
    }
    
    ++timestamp;

    // Reconfigure if necessary
    if (--nextReconfiguration == 0) {
        reconfigure();
        nextReconfiguration = ACCS_PER_RECONFIGURATION;
        ++numReconfigurations;
    }

    return;
}

void LHD::afterGet_(Tag* tag, const uint32_t& appid)
{
    assert(tag != NULL);

    tag->timestamp = timestamp;
    tag->app = appid % APP_CLASSES;
    // NOTE: object size is NOT changed after get operation

    // NOTE: also NO need to change explorer flag (just keep the original one assigned when admit/update)

    // NOTE: also NO need to update recentlyAdmitted due to NOT admission operation

    ++timestamp;

    // Reconfigure if necessary
    if (--nextReconfiguration == 0) {
        reconfigure();
        nextReconfiguration = ACCS_PER_RECONFIGURATION;
        ++numReconfigurations;
    }

    return;
}

} // namespace repl

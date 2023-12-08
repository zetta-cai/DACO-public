/*
 * LHD: refer to lib/lhd/lhd.hpp.
 *
 * NOTE: due to using recency information to calculate hit density, instead of using greedy dual method, LHD randomly (w/ 1/cached_objcnt probability) chooses ASSOCIATIVITY samples to avoid huge computation overhead of popularity updates. As randomly sampling may NOT evict real victims or mis-evict non-victims, LHD tracks recently-admited ADMISSIONS non-explorer objects to avoid NOT evicting real victims, and randomly (with 1/EXPLORE_INVERSE_PROBABILITY probability) mark admitted objects as explorers (within up to 1% cache capacity) to avoid mis-evicting non-victims.
 *
 * Hack original official simulator into a real cache engine to support key-value caching, required interfaces, and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2023.12.07).
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <limits>
#include "rand.hpp"

#include "common/key.h"
#include "common/value.h"

namespace repl {

class LHD {
  public:
    static const uint32_t SINGLE_TENANT_APPID; // NOTE: we do NOT consider multiple tenants with different application IDs

    LHD(const uint64_t& capacity_bytes);
    ~LHD() {}

    // called whenever and object is referenced
    bool exists(const covered::Key& key) const;
    bool get(const covered::Key& key, covered::Value& value);
    bool update(const covered::Key& key, const covered::Value& value);
    void admit(const covered::Key& key, const covered::Value& value);

    // called when an object is evicted
    bool replaced(const covered::Key& key, covered::Value& value);

    // called to find a victim upon a cache miss
    bool rank(covered::Key& key);

    void dumpStats() { }

    uint64_t getSizeForCapacity() const;
  private:
    // TYPES ///////////////////////////////
    typedef uint64_t timestamp_t;
    typedef uint64_t age_t;
    typedef float rank_t;

    // info we track about each object
    struct Tag {
        // Updates are triggered by the current access of the corresponding key
        age_t timestamp; // The timestamp of the last access
        age_t lastHitAge; // The being-cached time (i.e., age) from the last access to the current acces
        age_t lastLastHitAge; // The being-cached time (i.e., age) from the last last access to the last access
        uint32_t app; // Application ID
        
        covered::Key key;
        covered::Value value;
        rank_t size;
        bool explorer; // Siyuan: randomly (with 1/EXPLORE_INVERSE_PROBABILITY probability) mark admited objects as explorers (within up to 1% cache capacity) to avoid being evicted unless achieving MAX_AGE

        uint64_t getSizeForCapacity()
        {
            const uint64_t tag_size = sizeof(age_t) * 3 + sizeof(uint32_t) + key.getKeyLength() + value.getValuesize() + sizeof(rank_t) + sizeof(bool);
            return tag_size;
        }
    };

    // info we track about each class of objects
    struct Class {
        std::vector<rank_t> hits;
        std::vector<rank_t> evictions;
        rank_t totalHits = 0;
        rank_t totalEvictions = 0;

        std::vector<rank_t> hitDensities;

        uint64_t getSizeForCapacity()
        {
            const uint64_t class_size = sizeof(rank_t) * (hits.size() + evictions.size() + hitDensities.size() + 2);
            return class_size;
        }
    };

    // CONSTANTS ///////////////////////////
    static const std::string kClassName;

    // how to sample candidates; can significantly impact hit
    // ratio. want a value at least 32; diminishing returns after
    // that.
    const uint32_t ASSOCIATIVITY = 32;

    // since our cache simulator doesn't bypass objects, we always
    // consider the last ADMISSIONS objects as eviction candidates
    // (this is important to avoid very large objects polluting the
    // cache.) alternatively, you could do bypassing and randomly
    // admit objects as "explorers" (see below).
    const uint32_t ADMISSIONS = 8;

    // escape local minima by having some small fraction of cache
    // space allocated to objects that aren't evicted. 1% seems to be
    // a good value that has limited impact on hit ratio while quickly
    // discovering the shape of the access pattern.
    static constexpr rank_t EXPLORER_BUDGET_FRACTION = 0.01;
    static constexpr uint32_t EXPLORE_INVERSE_PROBABILITY = 32;

    // these parameters determine how aggressively to classify objects.
    // diminishing returns after a few classes; 16 is safe.
    static constexpr uint32_t HIT_AGE_CLASSES = 16;
    static constexpr uint32_t APP_CLASSES = 16;
    static constexpr uint32_t NUM_CLASSES = HIT_AGE_CLASSES * APP_CLASSES;
    
    // these parameters are tuned for simulation performance, and hit
    // ratio is insensitive to them at reasonable values (like these)
    static constexpr rank_t AGE_COARSENING_ERROR_TOLERANCE = 0.01;
    static constexpr age_t MAX_AGE = 20000;
    static constexpr timestamp_t ACCS_PER_RECONFIGURATION = (1 << 20);
    static constexpr rank_t EWMA_DECAY = 0.9;

    // verbose debugging output?
    static constexpr bool DUMP_RANKS = false;

    // FIELDS //////////////////////////////
    uint64_t kvpair_bytes_; // Siyuan: kvpair size usage for cached objects in units of bytes (NOTE: ONLY used for reconfiguration instead of cache size usage calculation, which has already been counted by internal_bytes_ as a part of tags)
    uint64_t internal_bytes_; // Siyuan: cache size usage for tags, classes, and indices in units of bytes

    // object metadata; indices maps key -> Tag (kvpair and metadata) index
    std::vector<Tag> tags;
    std::vector<Class> classes;
    std::unordered_map<covered::Key, uint64_t, covered::KeyHasher> indices;

    // time is measured in # of requests
    timestamp_t timestamp = 0;
    
    timestamp_t nextReconfiguration = 0;
    int numReconfigurations = 0;
    
    // how much to shift down age values; initial value doesn't really
    // matter, but must be positive. tuned in adaptAgeCoarsening() at
    // the beginning of the trace using ewmaNumObjects.
    timestamp_t ageCoarseningShift = 10;
    rank_t ewmaNumObjects = 0;
    rank_t ewmaNumObjectsMass = 0.;

    // how many objects had age > max age (this should almost never
    // happen -- if you observe non-neglible overflows, something has
    // gone wrong with the age coarsening!!!)
    uint64_t overflows = 0;

    misc::Rand rand;

    // see ADMISSIONS above
    std::vector<covered::Key> recentlyAdmitted;
    int recentlyAdmittedHead = 0;
    rank_t ewmaVictimHitDensity = 0;

    int64_t explorerBudget = 0;
    
    // METHODS /////////////////////////////

    // returns something like log(maxAge - age)
    inline uint32_t hitAgeClass(age_t age) const {
        if (age == 0) { return HIT_AGE_CLASSES - 1; }
        uint32_t log = 0;
        while (age < MAX_AGE && log < HIT_AGE_CLASSES - 1) {
            age <<= 1;
            log += 1;
        }
        return log;
    }

    inline uint32_t getClassId(const Tag& tag) const {
        uint32_t hitAgeId = hitAgeClass(tag.lastHitAge + tag.lastLastHitAge);
        return tag.app * HIT_AGE_CLASSES + hitAgeId;
    }
    
    inline Class& getClass(const Tag& tag) {
        return classes[getClassId(tag)];
    }

    inline age_t getAge(Tag tag) {
        timestamp_t age = (timestamp - (timestamp_t)tag.timestamp) >> ageCoarseningShift;

        if (age >= MAX_AGE) {
            ++overflows;
            return MAX_AGE - 1;
        } else {
            return (age_t) age;
        }
    }

    inline rank_t getHitDensity(const Tag& tag) {
        auto age = getAge(tag);
        if (age == MAX_AGE-1) { return std::numeric_limits<rank_t>::lowest(); }
        auto& cl = getClass(tag);
        rank_t density = cl.hitDensities[age] / tag.size;
        if (tag.explorer) { density += 1.; }
        return density;
    }
        
    void reconfigure();
    void adaptAgeCoarsening();
    void updateClass(Class& cl);
    void modelHitDensity();
    void dumpClassRanks(Class& cl);

    void afterAdmitOrUpdate_(Tag* tag, const covered::Key& key, const uint32_t& appid, const uint32_t& object_size, const bool& is_admit); // Update Tag (kvpair and metadata), mark explorer, update recently-admited vector, and trigger reconfiguration if necessary after admit/update
    void afterGet_(Tag* tag, const uint32_t& appid); // Update metadata of Tag and trigger reconfiguration if necessary after get
};

} // namespace repl

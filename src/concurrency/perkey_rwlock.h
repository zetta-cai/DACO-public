/*
 * PerkeyRwlock: provide key-level read-write lock for fine-grained concurrency control.
 *
 * NOTE: if disable fine-grained locking, PerkeyRwlock will go back to a single read-write lock.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef PERKEY_RWLOCK_H
#define PERKEY_RWLOCK_H

#define DEBUG_PERKEY_RWLOCK // TMPDEBUG24

#include <atomic>
#include <string>
#ifdef DEBUG_PERKEY_RWLOCK
#include <unordered_map>
#endif

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    class PerkeyRwlock
    {
    public:
        PerkeyRwlock(const uint32_t& edge_idx, const uint32_t& fine_grained_locking_size, const bool& enable_fine_grained_locking = true);
        ~PerkeyRwlock();

        // The same interfaces as libboost
        void acquire_lock_shared(const Key& key, const std::string& context_name);
        void unlock_shared(const Key& key, const std::string& context_name);
        void acquire_lock(const Key& key, const std::string& context_name, const bool& is_monitored = false); // TMPDEBUG24
        void unlock(const Key& key, const std::string& context_name);

        bool isReadLocked(const Key& key) const;
        bool isWriteLocked(const Key& key) const;
        bool isReadOrWriteLocked(const Key& key) const;
        uint32_t getRwlockIndex(const Key& key) const;
        uint32_t getFineGrainedLockingSize() const;
    private:
        static const std::string kClassName;

        bool try_lock_shared_(const Key& key, const std::string& context_name);
        bool try_lock_(const Key& key, const std::string& context_name);

        std::string instance_name_;
        const uint32_t fine_grained_locking_size_; // Come from Config::fine_grained_locking_size_
        const bool enable_fine_grained_locking_; // Determined by the specific local edge cache

        // NOTE: we have to use dynamic array for boost::shared_mutex and std::atomic, as they do NOT have copy constructor and operator= for std::vector (e.g., resize() and push_back())
        boost::shared_mutex* rwlock_hashtable_;
        std::atomic<uint32_t>* read_lock_cnts_;
        std::atomic<bool>* write_lock_flags_;
        HashWrapperBase* hash_wrapper_ptr_;

        #ifdef DEBUG_PERKEY_RWLOCK
        boost::mutex* perlock_mutex_for_debug_;
        std::vector<std::unordered_map<std::string, uint32_t>> perlock_current_readers_;
        std::vector<std::string> perlock_current_writer_;
        #endif
    };
}

#endif
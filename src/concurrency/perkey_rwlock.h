/*
 * PerkeyRwlock: provide key-level read-write lock for concurrency control.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef PERKEY_RWLOCK_H
#define PERKEY_RWLOCK_H

#include <string>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"
#include "hash/hash_wrapper_base.h"

namespace covered
{
    class PerkeyRwlock
    {
    public:
        PerkeyRwlock(const uint32_t& edge_idx);
        ~PerkeyRwlock();

        // The same interfaces as libboost
        bool try_lock_shared(const Key& key, const std::string& context_name);
        void unlock_shared(const Key& key);
        bool try_lock(const Key& key, const std::string& context_name);
        void unlock(const Key& key);
    private:
        // NOTE: we cannot use std::vector<boost::shared_mutex> as boost::shared_mutex does not have copy constructor
        typedef std::vector<boost::shared_mutex*> rwlock_hashtable_t; 

        static const std::string kClassName;
        static const uint32_t RWLOCK_HASHTABLE_CAPCITY;

        uint32_t getRwlockIndex_(const Key& key);

        std::string instance_name_;

        rwlock_hashtable_t rwlock_hashtable_;
        HashWrapperBase* hash_wrapper_ptr_;
    };
}

#endif
/*
 * Rwlock: provide an individual read-write lock for concurrency control.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef RWLOCK_H
#define RWLOCK_H

// TMPDEBUG231220
#define DEBUG_RWLOCK

#ifdef DEBUG_RWLOCK
#include <unordered_map>
#include <unordered_set>
#endif
#include <string>

#include <boost/thread/shared_mutex.hpp>

#include "common/key.h"

namespace covered
{
    class Rwlock
    {
    public:
        Rwlock(const std::string& lock_name);
        ~Rwlock();

        // The same interfaces as libboost
        void acquire_lock_shared(const std::string& context_name, std::unordered_set<std::string>* prev_writers_ptr = NULL);
        void unlock_shared(const std::string& context_name);
        void acquire_lock(const std::string& context_name, std::unordered_set<std::string>* prev_writers_ptr = NULL, std::unordered_set<std::string>* prev_readers_ptr = NULL);
        void unlock(const std::string& context_name);
    private:
        static const std::string kClassName;

        bool try_lock_shared_(const std::string& context_name);
        bool try_lock_(const std::string& context_name);

        std::string instance_name_;

        boost::shared_mutex rwlock_;

        #ifdef DEBUG_RWLOCK
        boost::mutex mutex_for_debug_;
        std::unordered_map<std::string, uint32_t> current_readers_;
        std::string current_writer_;
        #endif
    };
}

#endif
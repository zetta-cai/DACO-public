/*
 * Rwlock: provide an individual read-write lock for concurrency control.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef RWLOCK_H
#define RWLOCK_H

//#define DEBUG_RWLOCK

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
        void acquire_lock_shared(const std::string& context_name);
        void unlock_shared(const std::string& context_name);
        void acquire_lock(const std::string& context_name);
        void unlock(const std::string& context_name);
    private:
        static const std::string kClassName;

        bool try_lock_shared_(const std::string& context_name);
        bool try_lock_(const std::string& context_name);

        std::string instance_name_;

        boost::shared_mutex rwlock_;
    };
}

#endif
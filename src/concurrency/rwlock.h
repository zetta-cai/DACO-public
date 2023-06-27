/*
 * Rwlock: provide an individual read-write lock for concurrency control.
 * 
 * By Siyuan Sheng (2023.06.15).
 */

#ifndef RWLOCK_H
#define RWLOCK_H

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
        bool try_lock_shared(const std::string& context_name);
        void unlock_shared();
        bool try_lock(const std::string& context_name);
        void unlock();
    private:
        static const std::string kClassName;

        std::string instance_name_;

        boost::shared_mutex rwlock_;
    };
}

#endif
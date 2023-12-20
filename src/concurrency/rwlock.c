#include "concurrency/rwlock.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string Rwlock::kClassName("Rwlock");

    Rwlock::Rwlock(const std::string& lock_name)
    {
        std::ostringstream oss;
        oss << kClassName << " of " << lock_name;
        instance_name_ = oss.str();
    }

    Rwlock::~Rwlock() {}

    void Rwlock::acquire_lock_shared(const std::string& context_name, std::unordered_set<std::string>* prev_writers_ptr)
    {
        while (true) // Frequent polling
        {
            if (try_lock_shared_(context_name))
            {
                #ifdef DEBUG_RWLOCK
                mutex_for_debug_.lock();
                if (current_readers_.find(context_name) == current_readers_.end())
                {
                    current_readers_.insert(std::pair(context_name, 1));
                }
                else
                {
                    current_readers_[context_name] += 1;
                }
                mutex_for_debug_.unlock();
                #endif

                break;
            }

            #ifdef DEBUG_RWLOCK
            if (prev_writers_ptr != NULL) // Record current writer
            {
                mutex_for_debug_.lock();
                const std::string tmp_current_writer = current_writer_;
                mutex_for_debug_.unlock();

                if (tmp_current_writer != "" && prev_writers_ptr->find(tmp_current_writer) == prev_writers_ptr->end())
                {
                    prev_writers_ptr->insert(tmp_current_writer);
                }
            }
            #endif
        }
        return;
    }

    bool Rwlock::try_lock_shared_(const std::string& context_name)
    {
        bool result = rwlock_.try_lock_shared();

        #ifdef DEBUG_RWLOCK
        // if (result)
        // {
        //     std::ostringstream oss;
        //     oss << "acquire a read lock in " << context_name;
        //     Util::dumpDebugMsg(instance_name_, oss.str());
        // }
        #endif

        return result;
    }

    void Rwlock::unlock_shared(const std::string& context_name)
    {
        #ifdef DEBUG_RWLOCK
        mutex_for_debug_.lock();
        std::unordered_map<std::string, uint32_t>::const_iterator tmp_iter = current_readers_.find(context_name);
        assert(tmp_iter != current_readers_.end());
        if (tmp_iter->second > 1)
        {
            current_readers_[context_name] -= 1;
        }
        else
        {
            current_readers_.erase(tmp_iter);
        }
        mutex_for_debug_.unlock();

        // std::ostringstream oss;
        // oss << "release a read lock in " << context_name;
        // Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        rwlock_.unlock_shared();
        return;
    }

    void Rwlock::acquire_lock(const std::string& context_name, std::unordered_set<std::string>* prev_writers_ptr, std::unordered_set<std::string>* prev_readers_ptr)
    {
        while (true) // Frequent polling
        {
            if (try_lock_(context_name))
            {
                #ifdef DEBUG_RWLOCK
                mutex_for_debug_.lock();
                current_writer_ = context_name;
                mutex_for_debug_.unlock();
                #endif

                break;
            }

            #ifdef DEBUG_RWLOCK
            if (prev_writers_ptr != NULL) // Record current writer
            {
                mutex_for_debug_.lock();
                const std::string tmp_current_writer = current_writer_;
                mutex_for_debug_.unlock();

                if (tmp_current_writer != "" && prev_writers_ptr->find(tmp_current_writer) == prev_writers_ptr->end())
                {
                    prev_writers_ptr->insert(tmp_current_writer);
                }
            }
            if (prev_readers_ptr != NULL) // Record current readers
            {
                mutex_for_debug_.lock();
                std::list<std::string> tmp_current_readers;
                for (std::unordered_map<std::string, uint32_t>::const_iterator current_readers_const_iter = current_readers_.begin(); current_readers_const_iter != current_readers_.end(); current_readers_const_iter++)
                {
                    tmp_current_readers.push_back(current_readers_const_iter->first);
                }
                mutex_for_debug_.unlock();

                for (std::list<std::string>::const_iterator tmp_list_iter = tmp_current_readers.begin(); tmp_list_iter != tmp_current_readers.end(); tmp_list_iter++)
                {
                    if (prev_readers_ptr->find(*tmp_list_iter) == prev_readers_ptr->end())
                    {
                        prev_readers_ptr->insert(*tmp_list_iter);
                    }
                }
            }
            #endif
        }
        return;
    }

    bool Rwlock::try_lock_(const std::string& context_name)
    {
        bool result = rwlock_.try_lock();

        #ifdef DEBUG_RWLOCK
        // if (result)
        // {
        //     std::ostringstream oss;
        //     oss << "acquire a write lock in " << context_name;
        //     Util::dumpDebugMsg(instance_name_, oss.str());
        // }
        #endif

        return result;
    }

    void Rwlock::unlock(const std::string& context_name)
    {
        #ifdef DEBUG_RWLOCK
        mutex_for_debug_.lock();
        current_writer_ = "";
        mutex_for_debug_.unlock();

        // std::ostringstream oss;
        // oss << "release a write lock in " << context_name;
        // Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        rwlock_.unlock();
        return;
    }
}
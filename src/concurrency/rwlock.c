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

    void Rwlock::acquire_lock_shared(const std::string& context_name)
    {
        while (true) // Frequent polling
        {
            if (try_lock_shared_(context_name))
            {
                break;
            }
        }
        return;
    }

    bool Rwlock::try_lock_shared_(const std::string& context_name)
    {
        bool result = rwlock_.try_lock_shared();

        #ifdef DEBUG_RWLOCK
        if (result)
        {
            std::ostringstream oss;
            oss << "acquire a read lock in " << context_name;
            Util::dumpDebugMsg(instance_name_, oss.str());
        }
        #endif

        return result;
    }

    void Rwlock::unlock_shared(const std::string& context_name)
    {
        #ifdef DEBUG_RWLOCK
        std::ostringstream oss;
        oss << "release a read lock in " << context_name;
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        rwlock_.unlock_shared();
        return;
    }

    void Rwlock::acquire_lock(const std::string& context_name)
    {
        while (true) // Frequent polling
        {
            if (try_lock_(context_name))
            {
                break;
            }
        }
        return;
    }

    bool Rwlock::try_lock_(const std::string& context_name)
    {
        bool result = rwlock_.try_lock();

        #ifdef DEBUG_RWLOCK
        if (result)
        {
            std::ostringstream oss;
            oss << "acquire a write lock in " << context_name;
            Util::dumpDebugMsg(instance_name_, oss.str());
        }
        #endif

        return result;
    }

    void Rwlock::unlock(const std::string& context_name)
    {
        #ifdef DEBUG_RWLOCK
        std::ostringstream oss;
        oss << "release a write lock in " << context_name;
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        rwlock_.unlock();
        return;
    }
}
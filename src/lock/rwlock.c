#include "lock/rwlock.h"

#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string Rwlock::kClassName("Rwlock");

    Rwlock::Rwlock(const std::string& lock_name)
    {
        std::ostringstream oss;
        oss << kClassName << " in " << lock_name;
        instance_name_ = oss.str();
    }

    Rwlock::~Rwlock() {}

    bool Rwlock::try_lock_shared()
    {
        bool result = rwlock_.try_lock_shared();

        // TMPDEBUG
        if (result)
        {
            std::ostringstream oss;
            oss << "acquire a read lock";
            Util::dumpDebugMsg(instance_name_, oss.str());
        }

        return result;
    }

    void Rwlock::unlock_shared()
    {
        // TMPDEBUG
        std::ostringstream oss;
        oss << "release a read lock";
        Util::dumpDebugMsg(instance_name_, oss.str());

        rwlock_.unlock_shared();
        return;
    }

    bool Rwlock::try_lock()
    {
        bool result = rwlock_.try_lock();

        // TMPDEBUG
        if (result)
        {
            std::ostringstream oss;
            oss << "acquire a write lock";
            Util::dumpDebugMsg(instance_name_, oss.str());
        }

        return result;
    }

    void Rwlock::unlock()
    {
        // TMPDEBUG
        std::ostringstream oss;
        oss << "release a write lock";
        Util::dumpDebugMsg(instance_name_, oss.str());

        rwlock_.unlock();
        return;
    }
}
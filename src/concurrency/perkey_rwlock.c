#include "concurrency/perkey_rwlock.h"

#include <assert.h>
#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string PerkeyRwlock::kClassName("PerkeyRwlock");
    
    PerkeyRwlock::PerkeyRwlock(const uint32_t& edge_idx, const uint32_t& fine_grained_locking_size)
    {
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        fine_grained_locking_size_ = fine_grained_locking_size;

        // Allocate space for rwlocks
        rwlock_hashtable_ = new boost::shared_mutex[fine_grained_locking_size_];
        assert(rwlock_hashtable_ != NULL);

        // Allocate space for read lock counts
        read_lock_cnts_ = new std::atomic<uint32_t>[fine_grained_locking_size_];
        assert(read_lock_cnts_ != NULL);
        for (uint32_t i = 0; i < fine_grained_locking_size_; i++)
        {
            read_lock_cnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        // Allocate space for write lock flags
        write_lock_flags_ = new std::atomic<bool>[fine_grained_locking_size_];
        assert(write_lock_flags_ != NULL);
        for (uint32_t i = 0; i < fine_grained_locking_size_; i++)
        {
            write_lock_flags_[i].store(false, Util::STORE_CONCURRENCY_ORDER);
        }

        // NOTE: hashing for rwlock index is orthogonal with hash partition for DHT-based content discovery
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapperByHashName(Param::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);
    }

    PerkeyRwlock::~PerkeyRwlock()
    {
        assert(rwlock_hashtable_ != NULL);
        delete[] rwlock_hashtable_;
        rwlock_hashtable_ = NULL;

        assert(read_lock_cnts_ != NULL);
        delete[] read_lock_cnts_;
        read_lock_cnts_ = NULL;

        assert(write_lock_flags_ != NULL);
        delete[] write_lock_flags_;
        write_lock_flags_ = NULL;

        assert(hash_wrapper_ptr_ != NULL);
        delete hash_wrapper_ptr_;
        hash_wrapper_ptr_ = NULL;
    }

    void PerkeyRwlock::acquire_lock_shared(const Key& key, const std::string& context_name)
    {
        while (true) // Frequent polling
        {
            if (try_lock_shared_(key, context_name))
            {
                break;
            }
        }
        return;
    }

    bool PerkeyRwlock::try_lock_shared_(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(read_lock_cnts_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        bool result = rwlock_hashtable_[rwlock_index].try_lock_shared();

        if (result)
        {
            read_lock_cnts_[rwlock_index].fetch_add(1, Util::RMW_CONCURRENCY_ORDER);

            #ifdef DEBUG_PERKEY_RWLOCK
            std::ostringstream oss;
            oss << "acquire a read lock for key " << key.getKeystr() << " with rwlock_index " << rwlock_index << " in " << context_name;
            Util::dumpDebugMsg(instance_name_, oss.str());
            #endif
        }

        return result;
    }

    void PerkeyRwlock::unlock_shared(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(read_lock_cnts_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        read_lock_cnts_[rwlock_index].fetch_sub(1, Util::RMW_CONCURRENCY_ORDER);
        rwlock_hashtable_[rwlock_index].unlock_shared();

        #ifdef DEBUG_PERKEY_RWLOCK
        std::ostringstream oss;
        oss << "release a read lock for key " << key.getKeystr() << " with rwlock_index " << rwlock_index << " in " << context_name;
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        return;
    }

    void PerkeyRwlock::acquire_lock(const Key& key, const std::string& context_name)
    {
        while (true) // Frequent polling
        {
            if (try_lock_(key, context_name))
            {
                break;
            }
        }
        return;
    }

    bool PerkeyRwlock::try_lock_(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(write_lock_flags_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        bool result = rwlock_hashtable_[rwlock_index].try_lock();

        if (result)
        {
            // Original write lock flag MUST be false
            bool original_write_lock_flag = write_lock_flags_[rwlock_index].load(Util::LOAD_CONCURRENCY_ORDER);
            assert(!original_write_lock_flag);

            write_lock_flags_[rwlock_index].store(true, Util::STORE_CONCURRENCY_ORDER);

            #ifdef DEBUG_PERKEY_RWLOCK
            std::ostringstream oss;
            oss << "acquire a write lock for key " << key.getKeystr() << " with rwlock_index " << rwlock_index << " in " << context_name;
            Util::dumpDebugMsg(instance_name_, oss.str());
            #endif
        }

        return result;
    }

    void PerkeyRwlock::unlock(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(write_lock_flags_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        write_lock_flags_[rwlock_index].store(false, Util::STORE_CONCURRENCY_ORDER);
        rwlock_hashtable_[rwlock_index].unlock();
        
        #ifdef DEBUG_PERKEY_RWLOCK
        std::ostringstream oss;
        oss << "release a write lock for key " << key.getKeystr() << " with rwlock_index " << rwlock_index << " in " << context_name;
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        return;
    }

    bool PerkeyRwlock::isReadLocked(const Key& key) const
    {
        assert(read_lock_cnts_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        bool is_read_locked = (read_lock_cnts_[rwlock_index].load(Util::LOAD_CONCURRENCY_ORDER) > 0);

        return is_read_locked;
    }

    bool PerkeyRwlock::isWriteLocked(const Key& key) const
    {
        assert(write_lock_flags_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        bool is_write_locked = (write_lock_flags_[rwlock_index].load(Util::LOAD_CONCURRENCY_ORDER) == true);

        return is_write_locked;
    }

    bool PerkeyRwlock::isReadOrWriteLocked(const Key& key) const
    {
        assert(read_lock_cnts_ != NULL);
        assert(write_lock_flags_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);
        bool is_read_locked = (read_lock_cnts_[rwlock_index].load(Util::LOAD_CONCURRENCY_ORDER) > 0);
        bool is_write_locked = (write_lock_flags_[rwlock_index].load(Util::LOAD_CONCURRENCY_ORDER) == true);

        return is_read_locked || is_write_locked;
    }

    uint32_t PerkeyRwlock::getRwlockIndex(const Key& key) const
    {
        assert(hash_wrapper_ptr_ != NULL);

        // Hash the key
        uint32_t hash_value = hash_wrapper_ptr_->hash(key);
        uint32_t rwlock_index = hash_value % fine_grained_locking_size_;
        
        return rwlock_index;
    }

    uint32_t PerkeyRwlock::getFineGrainedLockingSize() const
    {
        return fine_grained_locking_size_;
    }
}
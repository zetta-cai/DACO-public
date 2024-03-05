#include "concurrency/perkey_rwlock.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string PerkeyRwlock::kClassName("PerkeyRwlock");
    
    PerkeyRwlock::PerkeyRwlock(const uint32_t& edge_idx, const uint32_t& fine_grained_locking_size, const bool& enable_fine_grained_locking) : fine_grained_locking_size_(fine_grained_locking_size), enable_fine_grained_locking_(enable_fine_grained_locking)
    {
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_idx;
        instance_name_ = oss.str();

        uint32_t rwlock_cnt = fine_grained_locking_size;
        if (!enable_fine_grained_locking)
        {
            rwlock_cnt = 1;
        }

        // Allocate space for rwlocks
        rwlock_hashtable_ = new boost::shared_mutex[rwlock_cnt];
        assert(rwlock_hashtable_ != NULL);

        #ifdef DEBUG_PERKEY_RWLOCK
        // Allocate space for debugging variables
        perlock_mutex_for_debug_ = new boost::mutex[rwlock_cnt];
        assert(perlock_mutex_for_debug_ != NULL);
        perlock_current_readers_.resize(rwlock_cnt);
        perlock_current_writer_.resize(rwlock_cnt);
        #endif

        // Allocate space for read lock counts
        read_lock_cnts_ = new std::atomic<uint32_t>[rwlock_cnt];
        assert(read_lock_cnts_ != NULL);
        for (uint32_t i = 0; i < rwlock_cnt; i++)
        {
            read_lock_cnts_[i].store(0, Util::STORE_CONCURRENCY_ORDER);
        }

        // Allocate space for write lock flags
        write_lock_flags_ = new std::atomic<bool>[rwlock_cnt];
        assert(write_lock_flags_ != NULL);
        for (uint32_t i = 0; i < rwlock_cnt; i++)
        {
            write_lock_flags_[i].store(false, Util::STORE_CONCURRENCY_ORDER);
        }

        // NOTE: hashing for rwlock index is orthogonal with hash partition for DHT-based content discovery
        hash_wrapper_ptr_ = HashWrapperBase::getHashWrapperByHashName(Util::MMH3_HASH_NAME);
        assert(hash_wrapper_ptr_ != NULL);
    }

    PerkeyRwlock::~PerkeyRwlock()
    {
        assert(rwlock_hashtable_ != NULL);
        delete[] rwlock_hashtable_;
        rwlock_hashtable_ = NULL;

        #ifdef DEBUG_PERKEY_RWLOCK
        assert(perlock_mutex_for_debug_ != NULL);
        delete[] perlock_mutex_for_debug_;
        perlock_mutex_for_debug_ = NULL;
        #endif

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
                #ifdef DEBUG_PERKEY_RWLOCK
                uint32_t rwlock_index = getRwlockIndex(key);
                perlock_mutex_for_debug_[rwlock_index].lock();
                if (perlock_current_readers_[rwlock_index].find(context_name) == perlock_current_readers_[rwlock_index].end())
                {
                    perlock_current_readers_[rwlock_index].insert(std::pair(context_name, 1));
                }
                else
                {
                    perlock_current_readers_[rwlock_index][context_name] += 1;
                }
                perlock_mutex_for_debug_[rwlock_index].unlock();
                #endif

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
            // std::ostringstream oss;
            // oss << "acquire a read lock for key " << key.getKeyDebugstr() << " with rwlock_index " << rwlock_index << " in " << context_name;
            // Util::dumpDebugMsg(instance_name_, oss.str());
            #endif
        }

        return result;
    }

    void PerkeyRwlock::unlock_shared(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(read_lock_cnts_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);

        #ifdef DEBUG_PERKEY_RWLOCK
        perlock_mutex_for_debug_[rwlock_index].lock();
        std::unordered_map<std::string, uint32_t>::const_iterator tmp_iter = perlock_current_readers_[rwlock_index].find(context_name);
        assert(tmp_iter != perlock_current_readers_[rwlock_index].end());
        if (tmp_iter->second > 1)
        {
            perlock_current_readers_[rwlock_index][context_name] -= 1;
        }
        else
        {
            perlock_current_readers_[rwlock_index].erase(tmp_iter);
        }
        perlock_mutex_for_debug_[rwlock_index].unlock();

        // std::ostringstream oss;
        // oss << "release a read lock for key " << key.getKeyDebugstr() << " with rwlock_index " << rwlock_index << " in " << context_name;
        // Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        read_lock_cnts_[rwlock_index].fetch_sub(1, Util::RMW_CONCURRENCY_ORDER);
        rwlock_hashtable_[rwlock_index].unlock_shared();

        return;
    }

    void PerkeyRwlock::acquire_lock(const Key& key, const std::string& context_name)
    {
        while (true) // Frequent polling
        {
            if (try_lock_(key, context_name))
            {
                #ifdef DEBUG_PERKEY_RWLOCK
                uint32_t rwlock_index = getRwlockIndex(key);
                perlock_mutex_for_debug_[rwlock_index].lock();
                perlock_current_writer_[rwlock_index] = context_name;
                perlock_mutex_for_debug_[rwlock_index].unlock();
                #endif

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
            // std::ostringstream oss;
            // oss << "acquire a write lock for key " << key.getKeyDebugstr() << " with rwlock_index " << rwlock_index << " in " << context_name;
            // Util::dumpDebugMsg(instance_name_, oss.str());
            #endif
        }

        return result;
    }

    void PerkeyRwlock::unlock(const Key& key, const std::string& context_name)
    {
        assert(rwlock_hashtable_ != NULL);
        assert(write_lock_flags_ != NULL);

        uint32_t rwlock_index = getRwlockIndex(key);

        #ifdef DEBUG_PERKEY_RWLOCK
        perlock_mutex_for_debug_[rwlock_index].lock();
        perlock_current_writer_[rwlock_index] = "";
        perlock_mutex_for_debug_[rwlock_index].unlock();

        // std::ostringstream oss;
        // oss << "release a write lock for key " << key.getKeyDebugstr() << " with rwlock_index " << rwlock_index << " in " << context_name;
        // Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        write_lock_flags_[rwlock_index].store(false, Util::STORE_CONCURRENCY_ORDER);
        rwlock_hashtable_[rwlock_index].unlock();
        
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

        uint32_t rwlock_index = 0;
        if (enable_fine_grained_locking_)
        {
            // Hash the key
            uint32_t hash_value = hash_wrapper_ptr_->hash(key);
            //uint32_t hash_value = key.getKeyLength(); // NOTE: use key length as hash value for per-key fine-grained rwlock to trade hash collision for performance (avoid string hashing overhead)
            rwlock_index = hash_value % fine_grained_locking_size_;
        }
        else
        {
            rwlock_index = 0;
        }
        
        return rwlock_index;
    }

    uint32_t PerkeyRwlock::getFineGrainedLockingSize() const
    {
        return fine_grained_locking_size_;
    }
}
/*
 * RocksdbWrapper: a wrapper of RocksDB KVS for persistent storage in cloud.
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#include <string>

#include <rocksdb/db.h>
#include <rocksdb/cache.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>

#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class RocksdbWrapper
    {
    public:
        // NOTE: a single RocksDB size on HDD should NOT exceed 500 GiB
        static const std::string HDD_NAME;

        // DeRocksDBfault settings suggested by official tuning guide
        // Refer to https://github.com/EighteenZi/rocksdb_wiki/blob/master/RocksDB-Tuning-Guide.md
        // Also refer to lib/rocksdb-8.1.1/include/rocksdb/options.h

        // (1) Parallelism options
        static const uint32_t kFlushThreadCnt;
        static const uint32_t kCompactionThreadCnt;

        // (2) General options
        static const uint32_t kBloomfilterBitsPerKey;
        static const uint32_t kBlockCacheCapacity;
        static const uint32_t kBlockCacheShardBits;
        //static const bool kAllowOsBuffer;
        static const int kMaxOpenFiles;
        static const uint32_t kTableCacheNumshardbits;
        static const uint32_t kBlockSize;
        static const uint32_t kBlockSizeForHdd;

        // (3) Flushing options
        static const uint32_t kWriteBufferSize;
        static const uint32_t kWriteBufferSizeForHdd;
        static const uint32_t kMaxWriteBufferNumber;
        static const uint32_t kMaxWriteBufferNumberToMerge;

        // (4) Compaction options
        static const uint32_t kLevel0SstNum;
        static const uint32_t kNumLevels;
        static const uint32_t kLevel1SstNum;
        static const uint32_t kLevel1SstSize;
        static const uint32_t kLevel1SstSizeForHdd;
        static const uint32_t kSstSizeMultiplier;
        static const uint32_t kLevelSizeMultiplier;

        // (5) Other options
        static const uint32_t kWalBytesPerSync;

        // (6) Specific options
        static const uint32_t kMaxFileOpeningThreadsForHdd;
        //static const uint32_t kCompactionReadaheadSizeForHdd;

        RocksdbWrapper(const std::string& db_filepath);
        ~RocksdbWrapper();

        void get(const Key& key, Value& value);
        void put(const Key& key, const Value& value);
        void remove(const Key& key);
    private:
        static const std::string kClassName;

        void open_(const std::string& db_filepath);

        rocksdb::DB* db_ptr_;
    };
}
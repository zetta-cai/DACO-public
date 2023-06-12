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

#include "cloud/cloud_param.h"
#include "common/key.h"
#include "common/value.h"

namespace covered
{
    class RocksdbWrapper
    {
    public:
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

        RocksdbWrapper(const std::string& cloud_storage, const std::string& db_filepath, CloudParam* cloud_param_ptr);
        ~RocksdbWrapper();

        void get(const Key& key, Value& value);
        void put(const Key& key, const Value& value);
        void remove(const Key& key);
    private:
        static const std::string kClassName;

        void open_(const std::string& cloud_storage, const std::string& db_filepath);

        // CloudWrapper only uses cloud index to specify instance_name_, yet not need to check if cloud is running due to no network communication -> no need to maintain cloud_param_ptr_
        std::string instance_name_;

        rocksdb::DB* db_ptr_;
    };
}
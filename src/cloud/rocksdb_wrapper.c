#include "cloud/rocksdb_wrapper.h"

#include <sstream>

#include "common/param.h"
#include "common/util.h"

namespace covered
{
    const std::string RocksdbWrapper::HDD_NAME = "hdd";

    // (1) Parallelism options
    const uint32_t RocksdbWrapper::kFlushThreadCnt = 1; // Default of 1 is good enough
    const uint32_t RocksdbWrapper::kCompactionThreadCnt = 4; // Default of 1 yet can be increased based on your number of CPU cores

    // (2) General options
    const uint32_t RocksdbWrapper::kBloomfilterBitsPerKey = 10; // Default of 10
    const uint32_t RocksdbWrapper::kBlockCacheCapacity = 1 * 1024 * 1024 * 1024; // Default of 1 GiB
    const uint32_t RocksdbWrapper::kBlockCacheShardBits = 4; // Default of 4 (i.e., 16 shards)
    const bool RocksdbWrapper::kAllowOsBuffer = true; // Default of true
    const int RocksdbWrapper::kMaxOpenFiles = -1; // Keep all files open
    const uint32_t RocksdbWrapper::kTableCacheNumshardbits = 6; // Default of 6 (i.e., 64 shards)
    const uint32_t RocksdbWrapper::kBlockSize = 4 * 1024; // Default of 4 KiB
    const uint32_t RocksdbWrapper::kBlockSizeForHdd = 64 * 1024; // At least 64 KiB for HDD

    // (3) Flushing options
    const uint32_t RocksdbWrapper::kWriteBufferSize = 64 * 1024 * 1024; // Default of 64 MiB
    const uint32_t RocksdbWrapper::kWriteBufferSizeForHdd = 256 * 1024 * 1024; // At least 256 MiB for HDD
    const uint32_t RocksdbWrapper::kMaxWriteBufferNumber = 5; // Example of 5
    const uint32_t RocksdbWrapper::kMaxWriteBufferNumberToMerge = 2; // Example of 2

    // (4) Compaction options
    const uint32_t RocksdbWrapper::kLevel0SstNum = 4; // Default of 4
    const uint32_t RocksdbWrapper::kNumLevels = 7; // Default of 7
    const uint32_t RocksdbWrapper::kLevel1SstNum = 10; // Suggestion of 10
    const uint32_t RocksdbWrapper::kLevel1SstSize = 64 * 1024 * 1024; // Default of 64 MiB
    const uint32_t RocksdbWrapper::kLevel1SstSizeForHdd = 64 * 1024 * 1024; //At least 256 MiB for HDD
    const uint32_t RocksdbWrapper::kSstSizeMultiplier = 1; // Default of 1
    const uint32_t RocksdbWrapper::kLevelSizeMultiplier = 10; // Default of 10

    // (5) Other options
    const uint32_t RocksdbWrapper::kWalBytesPerSync = 2 * 1024 * 1024; // Use 2 MiB to avoid OS bottleneck

    // (6) Specific options
    const uint32_t RocksdbWrapper::kDiskCntForHdd = 1; // Set a value larger than 1 if cloud has multiple disks
    const uint32_t kCompactionReadaheadSizeForHdd = 2 * 1024 * 1024; // At least 2 MiB for HDD

    const std::string RocksdbWrapper::kClassName("RocksdbWrapper");

    RocksdbWrapper::RocksdbWrapper(const std::string& dbpath) : db_ptr(NULL)
    {
        bool is_exist = Util::isDirectoryExist(dbpath);
        if (!is_exist)
        {
            std::ostringstream oss;
            oss << "RocksDB path " << dbpath << " does not exist!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        open_(dbpath);
    }

    RocksdbWrapper::~RocksdbWrapper()
    {
        assert(db_ptr_ != NULL);
        delete db_ptr_;
        db_ptr_ = NULL;
    }

    void RocksdbWrapper::get(const Key& key, Value& value)
    {
        std::string key_str = key.getKeystr();
        std::string value_str;
        rocksdb::Status rocksdb_status = db_ptr_->Get(rocksdb::ReadOptions(), key_str, &value_str);
        assert(rocksdb_status.ok());
        value = Value(value_str.length());
        return;
    }

    void RocksdbWrapper::put(const Key& key, const Value& value)
    {
        std::string key_str = key.getKeystr();
        std::string value_str = value.generateValuestr();
        rocksdb::Status rocksdb_status = db_ptr_->Put(rocksdb::WriteOptions(), key_str, value_str);
        assert(rocksdb_status.ok());
        return;
    }

    void RocksdbWrapper::remove(const Key& key)
    {
        std::string key_str = key.getKeystr();
        rocksdb::Status rocksdb_status = db_ptr_->Delete(rocksdb::WriteOptions(), key_str);
        assert(rocksdb_status.ok());
        return;
    }

    void RocksdbWrapper::open_(const std::string& dbpath)
    {
        rocksdb::Options rocksdb_options;
        rocksdb_options.create_if_missing = true;

        // (1) Parallelism options
        rocksdb_options.max_background_flushes = kFlushThreadCnt;
        rocksdb_options.env->SetBackgroundThreads(kFlushThreadCnt, Env::Priority::HIGH);
        rocksdb_options.max_background_compactions = kCompactionThreadCnt;
        rocksdb_options.env->SetBackgroundThreads(kCompactionThreadCnt, Env::Priority::LOW);

        // (2) General options
        rocksdb::BlockBasedTableOptions table_options;
        table_options.filter_policy = std::shared_ptr<const rocksdb::FilterPolicy>(rocksdb::NewBloomFilterPolicy(kBloomfilterBitsPerKey));
        table_options.block_cache = rocksdb::NewLRUCache(kBlockCacheCapacity, kBlockCacheShardBits);
        if (Param::getCloudStorage() == HDD_NAME)
        {
            table_options.block_size = kBlockSizeForHdd;
            table_options.cache_index_and_filter_blocks = true;
        }
        else
        {
            table_options.block_size = kBlockSize;
        }
        rocksdb_options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
        rocksdb_options.allow_os_buffer = kAllowOsBuffer;
        rocksdb_options.max_open_files = kMaxOpenFiles;
        rocksdb_options.table_cache_numshardbits = kTableCacheNumshardbits;

        // (3) Flushing options
        if (Param::getCloudStorage() == HDD_NAME)
        {
            rocksdb_options.write_buffer_size = kWriteBufferSizeForHdd;
        }
        else
        {
            rocksdb_options.write_buffer_size = kWriteBufferSize;
        }
        rocksdb_options.max_write_buffer_number = kMaxWriteBufferNumber;
        rocksdb_options.min_write_buffer_number_to_merge = kMaxWriteBufferNumberToMerge;

        // (4) Compaction options
        rocksdb_options.level0_file_num_compaction_trigger = kLevel0SstNum;
        if (Param::getCloudStorage() == HDD_NAME)
        {
            rocksdb_options.target_file_size_base = kLevel1SstSizeForHdd; // single sst size
            rocksdb_options.max_bytes_for_level_base = kLevel1SstNum * kLevel1SstSizeForHdd;
        }
        else
        {
            rocksdb_options.target_file_size_base = kLevel1SstSize; // single sst size
            rocksdb_options.max_bytes_for_level_base = kLevel1SstNum * kLevel1SstSize;
        }
        rocksdb_options.target_file_size_multiplier = kSstSizeMultiplier;
        rocksdb_options.max_bytes_for_level_multiplier = kLevelSizeMultiplier;
        // NOTE: we use default compression_per_level in RocksDB options
        rocksdb_options.num_levels = kNumLevels;

        // (5) Other options
        rocksdb_options.compaction_style = rocksdb::kCompactionStyleLevel; // level-based compaction
        rocksdb_options.wal_bytes_per_sync = kWalBytesPerSync;

        // (6) Specific options
        if (Param::getCloudStorage() == HDD_NAME)
        {
            rocksdb_options.optimize_filters_for_hits = true;
            rocksdb_options.skip_stats_update_on_db_open = true;
            rocksdb_options.level_compaction_dynamic_level_bytes = true;
            rocksdb_options.rocksdb_options = kDiskCntForHdd;
            rocksdb_options.compaction_readahead_size = kCompactionReadaheadSizeForHdd;
            rocksdb_options.new_table_reader_for_compaction_inputs = true;
        }

        rocksdb::Status rocksdb_status = rocksdb::DB::Open(rocksdb_options, dbpath, &db_ptr_);
        assert(rocksdb_status.ok());
        assert(db_ptr_ != NULL);
        return;
    }
}
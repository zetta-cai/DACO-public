/*
 * GeneratorBase: refer to lib/CacheLib/cachelib/cachebench/workload/GeneratorBase.h.
 *
 * Hack to add an interface named getReq() to provide the key-value dataset item with a given itemidx.
 * 
 * By Siyuan Sheng (2023.08.05).
 */

#pragma once

#include <folly/Benchmark.h>

#include <atomic>

#include <cachelib/cachebench/util/Request.h>

namespace covered {

class GeneratorBase {
 public:
  virtual ~GeneratorBase() {}

  // Grab the next request given the last request in the sequence or using the
  // random number generator.
  // @param poolId cache pool id for the generated request
  // @param gen random number generator
  // @param lastRequestId generator may generate next request based on last
  // request, e.g., piecewise caching in bigcache.
  virtual const facebook::cachelib::cachebench::Request& getReq(const uint32_t& local_client_worker_idx, uint8_t /*poolId*/,
                                std::mt19937_64& /*gen*/,
                                std::optional<uint64_t> /*lastRequestId*/) = 0;
  
  // Siyuan: get dataset item of a specific index
  virtual const facebook::cachelib::cachebench::Request& getDatasetReq(uint32_t itemidx) = 0;

  // Notify the workload generator about the result of the request.
  // Note the workload generator may release the memory for the request.
  virtual void notifyResult(uint64_t /*requestId*/, facebook::cachelib::cachebench::OpResultType /*result*/) {}

  virtual const std::vector<std::string>& getAllKeys() const = 0;

  // Siyuan: average/min/max dataset key/value size
  virtual double getAvgDatasetKeysize() const = 0;
  virtual double getAvgDatasetValuesize() const = 0;
  virtual uint32_t getMinDatasetKeysize() const = 0;
  virtual uint32_t getMinDatasetValuesize() const = 0;
  virtual uint32_t getMaxDatasetKeysize() const = 0;
  virtual uint32_t getMaxDatasetValuesize() const = 0;

  // Siyuan: quick operations for warmup speedup
  virtual void quickDatasetGet(const std::string& key, uint32_t& value_size) const = 0;
  virtual void quickDatasetPut(const std::string& key, const uint32_t& value_size) = 0;
  virtual void quickDatasetDel(const std::string& key) = 0;

  // Siyuan: For dynamic workload patterns
  virtual uint32_t getLargestRank(const uint32_t local_client_worker_idx, uint8_t poolId) = 0;
  virtual void getRankedKeys(const uint32_t local_client_worker_idx, uint8_t poolId, const uint32_t start_rank, const uint32_t ranked_keycnt, std::vector<std::string>& ranked_keys) = 0;
  virtual void getRandomKeys(const uint32_t local_client_worker_idx, uint8_t poolId, const uint32_t random_keycnt, std::vector<std::string>& random_keys) = 0;

  // Notify the workload generator that the nvm cache has already warmed up.
  virtual void setNvmCacheWarmedUp(uint64_t /*timestamp*/) {
    // not implemented by default
  }

  virtual void renderStats(uint64_t /*elapsedTimeNs*/,
                           std::ostream& /*out*/) const {
    // not implemented by default
  }

  virtual void renderStats(uint64_t /*elapsedTimeNs*/,
                           folly::UserCounters& /*counters*/) const {
    // not implemented by default
  }

  // Render the stats based on the data since last time we rendered.
  virtual void renderWindowStats(double /*elapsedSecs*/,
                                 std::ostream& /*out*/) const {
    // not implemented by default
  }

  // Should be called when all working threads are finished, or aborted
  void markShutdown() { isShutdown_.store(true, std::memory_order_relaxed); }

  // Should be called when working thread finish its operations
  virtual void markFinish() {}

 protected:
  bool shouldShutdown() const {
    return isShutdown_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<bool> isShutdown_{false};
};

} // namespace covered

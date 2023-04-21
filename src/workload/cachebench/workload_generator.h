/*
 * WorkloadGenerator: refer to lib/CacheLib/cachelib/cachebench/workload/WorkloadGenerator.*.
 *
 * Hack to set random seeds for multi-client benchmark.
 * Also hack to disable unnecessary outputs for multi-client benchmark.
 * 
 * By Siyuan Sheng (2023.04.21).
 */

#pragma once

#include <folly/Format.h>
#include <folly/Random.h>
#include <folly/logging/xlog.h>

#include <cstdint>
#include <string>
#include <vector>

#include <cachelib/cachebench/cache/Cache.h>
#include <cachelib/cachebench/util/Parallel.h>
#include <cachelib/cachebench/util/Request.h>
#include <cachelib/cachebench/workload/GeneratorBase.h>

#include "workload/cachebench/cachebench_config.h"
#include "workload/cachebench/workload_distribution.h"

namespace covered {

class WorkloadGenerator : public facebook::cachelib::cachebench::GeneratorBase {
 public:
  explicit WorkloadGenerator(const StressorConfig& config);
  virtual ~WorkloadGenerator() {}

  const facebook::cachelib::cachebench::Request& getReq(
      uint8_t poolId,
      std::mt19937_64& gen,
      std::optional<uint64_t> lastRequestId = std::nullopt) override;

  const std::vector<std::string>& getAllKeys() const override { return keys_; }

 private:
  void generateFirstKeyIndexForPool();
  void generateKeys();
  void generateReqs();
  void generateKeyDistributions();
  // if there is only one workloadDistribution, use it for everything.
  size_t workloadIdx(size_t i) { return workloadDist_.size() > 1 ? i : 0; }

  const StressorConfig config_;
  std::vector<std::string> keys_;
  std::vector<std::vector<size_t>> sizes_;
  std::vector<facebook::cachelib::cachebench::Request> reqs_; // key-value pairs
  // @firstKeyIndexForPool_ contains the first key in each pool (As represented
  // by key pool distribution). It's a convenient method for us to populate
  // @keyIndicesForPoo_ which contains all the key indices that each operation
  // in a pool. @keyGenForPool_ contains uniform distributions to select indices
  // contained in @keyIndicesForPool_.
  std::vector<uint32_t> firstKeyIndexForPool_;
  std::vector<std::vector<uint32_t>> keyIndicesForPool_; // operations or requests
  std::vector<std::uniform_int_distribution<uint32_t>> keyGenForPool_;

  std::vector<WorkloadDistribution> workloadDist_;
};
} // namespace covered

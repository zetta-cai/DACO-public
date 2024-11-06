/*
 * WorkloadGenerator: refer to lib/CacheLib/cachelib/cachebench/workload/WorkloadGenerator.*.
 *
 * Hack to set deterministic seeds for random generators under multi-client benchmark.
 * Also hack to disable unnecessary outputs for multi-client benchmark.
 * 
 * By Siyuan Sheng (2023.04.21).
 */

#pragma once

#include <folly/Format.h>
#include <folly/Random.h>
#include <folly/logging/xlog.h>

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <cachelib/cachebench/cache/Cache.h>
#include <cachelib/cachebench/util/Request.h>
//#include <cachelib/cachebench/workload/GeneratorBase.h>
#include "workload/cachebench/generator_base.h"

#include "workload/cachebench/parallel.h"
#include "workload/cachebench/cachebench_config.h"
#include "workload/cachebench/workload_distribution.h"

namespace covered {

class WorkloadGenerator : public covered::GeneratorBase {
 public:
  explicit WorkloadGenerator(const StressorConfig& config, const uint32_t& client_idx, const uint32_t& perclient_workercnt, const bool& is_zipf_generator = false, const float& zipf_alpha = 0.7f); // is_zipf_generator means whether we use zipf distribution to generate workload items
  virtual ~WorkloadGenerator() {}

  virtual const facebook::cachelib::cachebench::Request& getReq(
      const uint32_t& local_client_worker_idx,
      uint8_t poolId,
      std::mt19937_64& gen,
      std::optional<uint64_t> lastRequestId = std::nullopt) override;
  
  // Siyuan: get dataset item of a specific index
  virtual const facebook::cachelib::cachebench::Request& getDatasetReq(uint32_t itemidx) override;

  virtual const std::vector<std::string>& getAllKeys() const override { return keys_; }

  // Siyuan: average/min/max dataset key/value size
  virtual double getAvgDatasetKeysize() const override;
  virtual double getAvgDatasetValuesize() const override;
  virtual uint32_t getMinDatasetKeysize() const override;
  virtual uint32_t getMinDatasetValuesize() const override;
  virtual uint32_t getMaxDatasetKeysize() const override;
  virtual uint32_t getMaxDatasetValuesize() const override;

  // Siyuan: quick operations for warmup speedup
  virtual void quickDatasetGet(const std::string& key, uint32_t& value_size) const override;
  virtual void quickDatasetPut(const std::string& key, const uint32_t& value_size) override;
  virtual void quickDatasetDel(const std::string& key) override;

  // Siyuan: For dynamic workload patterns
  virtual const std::vector<uint32_t>& getRankedUniqueKeyIndicesConstRef(const uint32_t local_client_worker_idx, const uint8_t poolId) const override;

 private:
  static const std::string kClassName;

  void generateFirstKeyIndexForPool();
  void generateKeys();
  void generateReqs();
  void generateKeyDistributions();
  // if there is only one workloadDistribution, use it for everything.
  size_t workloadIdx(size_t i) { return workloadDist_.size() > 1 ? i : 0; }

  // Utility function
  void getDescendingSortedKeyIndices_(const std::vector<uint32_t>& key_indices, std::vector<uint32_t>& descending_sorted_key_indices); // Siyuan: convert opscnt duplicate key indices into unique key indices sorted by descending order of freq

  static bool descendingSortByValue(std::pair<uint32_t, uint32_t>& a, std::pair<uint32_t, uint32_t>& b); // Siyuan: used to get object ranks from default workload distribution

  const StressorConfig config_;
  const uint32_t client_idx_;
  const uint32_t perclient_workercnt_;
  std::vector<std::string> keys_; // Siyuan: dataset keys
  std::vector<std::vector<size_t>> sizes_; // Siyuan: dataset object sizes
  std::unordered_map<std::string, uint32_t> dataset_lookup_table_; // Siyuan: to support quick operations for warmup speedup
  std::vector<facebook::cachelib::cachebench::Request> reqs_; // Siyuan: key-value pairs (dataset items)
  // @firstKeyIndexForPool_ contains the first key in each pool (As represented
  // by key pool distribution). It's a convenient method for us to populate
  // @keyIndicesForPoo_ which contains all the key indices that each operation
  // in a pool. @keyGenForPool_ contains uniform distributions to select indices
  // contained in @keyIndicesForPool_.
  std::vector<uint32_t> firstKeyIndexForPool_; // pool id -> first key index (as we only have 1 pool by default, it only contains two intergers: 0 and numkeys-1)
  std::vector<std::vector<std::vector<uint32_t>>> perworkerKeyIndicesForPool_; // Siyuan: operations or requests (pre-generated workload items to approximate workload distribution); local client worker index -> pool id -> key indices
  std::vector<std::vector<std::uniform_int_distribution<uint32_t>>> perworkerKeyGenForPool_; // local client worker index -> pool id -> uniform distribution to select key indices

  std::vector<WorkloadDistribution> workloadDist_; // Siyuan: only 1 workload dist due to 1 pool -> includes key size dist (for dataset items), value size dist (for dataset items), and operation disk (for workload items)

  // Siyuan: For dynamic workload patterns
  std::vector<std::vector<std::vector<uint32_t>>> perworker_perpool_ranked_unique_key_indices_; // Siyuan: local client worker index -> pool id -> ranked unique key indices
  
  // Siyuan: average/min/max dataset key/value size
  double avg_dataset_keysize_;
  double avg_dataset_valuesize_;
  uint32_t min_dataset_keysize_;
  uint32_t min_dataset_valuesize_;
  uint32_t max_dataset_keysize_;
  uint32_t max_dataset_valuesize_;

  // Siyuan: for zipf-based workload generation
  bool is_zipf_generator_;
  float zipf_alpha_;
};
} // namespace covered

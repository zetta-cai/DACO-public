#include "workload/cachebench/workload_generator.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

#include "common/util.h"

namespace covered {

const std::string WorkloadGenerator::kClassName("WorkloadGenerator");

WorkloadGenerator::WorkloadGenerator(const StressorConfig& config, const uint32_t& client_idx)
    : config_{config}, client_idx_(client_idx) {
  for (const auto& c : config.poolDistributions) {
    if (c.keySizeRange.size() != c.keySizeRangeProbability.size() + 1) {
      throw std::invalid_argument(
          "Key size range and their probabilities do not match up. Check your "
          "test config.");
    }
    workloadDist_.push_back(WorkloadDistribution(c));

    avg_dataset_keysize_ = 0;
    avg_dataset_valuesize_ = 0;
    min_dataset_keysize_ = 0;
    min_dataset_valuesize_ = 0;
    max_dataset_keysize_ = 0;
    max_dataset_valuesize_ = 0;
  }

  if (config_.numKeys > std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument(folly::sformat(
        "Too many keys specified: {}. Maximum allowed is 4 Billion.",
        config_.numKeys));
  }

  generateReqs();
  generateKeyDistributions();
}

// gen is passed by each per-client worker to get different requests
const facebook::cachelib::cachebench::Request& WorkloadGenerator::getReq(uint8_t poolId,
                                         std::mt19937_64& gen,
                                         std::optional<uint64_t>) {
  XDCHECK_LT(poolId, keyIndicesForPool_.size());
  XDCHECK_LT(poolId, keyGenForPool_.size());

  size_t idx = keyIndicesForPool_[poolId][keyGenForPool_[poolId](gen)];
  auto op =
      static_cast<facebook::cachelib::cachebench::OpType>(workloadDist_[workloadIdx(poolId)].sampleOpDist(gen));
  reqs_[idx].setOp(op);
  return reqs_[idx];
}

const facebook::cachelib::cachebench::Request& WorkloadGenerator::getReq(uint8_t poolId, uint32_t itemidx)
{
  uint32_t keycnt = reqs_.size();
  if (itemidx >= keycnt)
  {
    std::ostringstream oss;
    oss << "itemidx " << itemidx << " should NOT >= keycnt " << keycnt;
    Util::dumpErrorMsg("cachebench::workload_generator", oss.str());
    exit(1);
  }

  return reqs_[itemidx];
}

double WorkloadGenerator::getAvgDatasetKeysize() const
{
  return avg_dataset_keysize_;
}

double WorkloadGenerator::getAvgDatasetValuesize() const
{
  return avg_dataset_valuesize_;
}

uint32_t WorkloadGenerator::getMinDatasetKeysize() const
{
  return min_dataset_keysize_;
}

uint32_t WorkloadGenerator::getMinDatasetValuesize() const
{
  return min_dataset_valuesize_;
}

uint32_t WorkloadGenerator::getMaxDatasetKeysize() const
{
  return max_dataset_keysize_;
}

uint32_t WorkloadGenerator::getMaxDatasetValuesize() const
{
  return max_dataset_valuesize_;
}

void WorkloadGenerator::generateKeys() {
  uint8_t pid = 0;
  auto fn = [pid, this](size_t start, size_t end, size_t local_thread_idx) -> void {
    // All keys are printable lower case english alphabet.
    std::uniform_int_distribution<char> charDis('a', 'z');
    //std::mt19937_64 gen(folly::Random::rand64());
    // Siyuan: use Util::KVPAIR_GENERATION_SEED as the deterministic seed to ensure that multiple clients generate the same set of key-value pairs
    std::mt19937_64 gen(Util::KVPAIR_GENERATION_SEED + local_thread_idx);
    for (uint64_t i = start; i < end; i++) {
      size_t keySize =
          facebook::cachelib::util::narrow_cast<size_t>(workloadDist_[pid].sampleKeySizeDist(gen));
      keys_[i].resize(keySize);
      for (auto& c : keys_[i]) {
        c = charDis(gen);
      }
    }
    return;
  };

  size_t totalKeys(0);
  std::chrono::seconds keyGenDuration(0);
  keys_.resize(config_.numKeys);
  for (size_t i = 0; i < config_.keyPoolDistribution.size(); i++) {
    pid = facebook::cachelib::util::narrow_cast<uint8_t>(workloadIdx(i));
    size_t numKeysForPool =
        firstKeyIndexForPool_[i + 1] - firstKeyIndexForPool_[i];
    totalKeys += numKeysForPool;
    keyGenDuration += covered::executeParallel(
        fn, config_.numThreads, numKeysForPool, firstKeyIndexForPool_[i]);
  }

  auto startTime = std::chrono::steady_clock::now();
  for (size_t i = 0; i < config_.keyPoolDistribution.size(); i++) {
    auto poolKeyBegin = keys_.begin() + firstKeyIndexForPool_[i];
    // past the end iterator
    auto poolKeyEnd = keys_.begin() + (firstKeyIndexForPool_[i + 1]);
    // Remove duplicate keys by sort + unique
    std::sort(poolKeyBegin, poolKeyEnd);
    auto newEnd = std::unique(poolKeyBegin, poolKeyEnd);
    // update pool key boundary before invalidating iterators
    for (size_t j = i + 1; j < firstKeyIndexForPool_.size(); j++) {
      firstKeyIndexForPool_[j] -= std::distance(newEnd, poolKeyEnd);
    }
    totalKeys -= std::distance(newEnd, poolKeyEnd);
    keys_.erase(newEnd, poolKeyEnd);
  }
  auto sortDuration = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - startTime);

  // Siyuan: update avg/min/max dataset key size
  for (size_t i = 0; i < keys_.size(); i++)
  {
    uint32_t tmp_keysize = keys_[i].size();
    avg_dataset_keysize_ += static_cast<double>(tmp_keysize);
    if (i == 0)
    {
      min_dataset_keysize_ = tmp_keysize;
      max_dataset_keysize_ = tmp_keysize;
    }
    else
    {
      if (tmp_keysize < min_dataset_keysize_)
      {
        min_dataset_keysize_ = tmp_keysize;
      }
      if (tmp_keysize > max_dataset_keysize_)
      {
        max_dataset_keysize_ = tmp_keysize;
      }
    }
  }
  avg_dataset_keysize_ /= static_cast<double>(keys_.size());

  // Siyuan: disable unnecessary outputs
  //std::cout << folly::sformat("Created {:,} keys in {:.2f} mins",
  //                            totalKeys,
  //                            (keyGenDuration + sortDuration).count() / 60.)
  //          << std::endl;
}

void WorkloadGenerator::generateReqs() {
  generateFirstKeyIndexForPool();
  generateKeys();
  //std::mt19937_64 gen(folly::Random::rand64());
  // Siyuan: Util::KVPAIR_GENERATION_SEED as the deterministic seed to ensure that multiple clients generate the same set of key-value pairs
  std::mt19937_64 gen(Util::KVPAIR_GENERATION_SEED);
  for (size_t i = 0; i < config_.keyPoolDistribution.size(); i++) {
    size_t idx = workloadIdx(i);
    for (size_t j = firstKeyIndexForPool_[i]; j < firstKeyIndexForPool_[i + 1];
         j++) {
      std::vector<size_t> chainSizes;
      chainSizes.push_back(
          facebook::cachelib::util::narrow_cast<size_t>(workloadDist_[idx].sampleValDist(gen)));
      int chainLen =
          facebook::cachelib::util::narrow_cast<int>(workloadDist_[idx].sampleChainedLenDist(gen));
      for (int k = 0; k < chainLen; k++) {
        chainSizes.push_back(facebook::cachelib::util::narrow_cast<size_t>(
            workloadDist_[idx].sampleChainedValDist(gen)));
      }
      sizes_.emplace_back(chainSizes);
      auto reqSizes = sizes_.end() - 1;
      reqs_.emplace_back(keys_[j], reqSizes->begin(), reqSizes->end());

      // Siyuan: update avg/min/max dataset value size
      uint32_t tmp_valuesize = *(reqSizes->begin());
      avg_dataset_valuesize_ += static_cast<double>(tmp_valuesize);
      if (i == 0 && j == firstKeyIndexForPool_[i])
      {
        min_dataset_valuesize_ = tmp_valuesize;
        max_dataset_valuesize_ = tmp_valuesize;
      }
      else
      {
        if (tmp_valuesize < min_dataset_valuesize_)
        {
          min_dataset_valuesize_ = tmp_valuesize;
        }
        if (tmp_valuesize > max_dataset_valuesize_)
        {
          max_dataset_valuesize_ = tmp_valuesize;
        }
      }
    }
  }
  avg_dataset_valuesize_ /= static_cast<double>(reqs_.size());

  // TMPDEBUG
  std::ostringstream oss;
  oss << "first key: " << reqs_[0].key << "; valuesize: " << (*reqs_[0].sizeBegin);
  Util::dumpNormalMsg(kClassName, oss.str());
}

void WorkloadGenerator::generateFirstKeyIndexForPool() {
  auto sumProb = std::accumulate(config_.keyPoolDistribution.begin(),
                                 config_.keyPoolDistribution.end(), 0.);
  auto accumProb = 0.;
  firstKeyIndexForPool_.push_back(0);
  for (auto prob : config_.keyPoolDistribution) {
    accumProb += prob;
    firstKeyIndexForPool_.push_back(
        facebook::cachelib::util::narrow_cast<uint32_t>(config_.numKeys * accumProb / sumProb));
  }
}

void WorkloadGenerator::generateKeyDistributions() {
  // We are trying to generate a gaussian distribution for each pool's part
  // in the overall cache ops. To keep the amount of memory finite, we only
  // generate a max of 4 billion op traces across all the pools and replay
  // the same when we need longer traces.
  std::chrono::seconds duration{0};
  for (uint64_t i = 0; i < config_.opPoolDistribution.size(); i++) {
    auto left = firstKeyIndexForPool_[i];
    auto right = firstKeyIndexForPool_[i + 1] - 1;
    size_t idx = workloadIdx(i);

    size_t numOpsForPool = std::min<size_t>(
        facebook::cachelib::util::narrow_cast<size_t>(config_.numOps * config_.numThreads *
                                  config_.opPoolDistribution[i]),
        std::numeric_limits<uint32_t>::max());
    // Siyuan: disable unnecessary outputs
    //std::cout << folly::sformat("Generating {:.2f}M sampled accesses",
    //                            numOpsForPool / 1e6)
    //          << std::endl;
    keyGenForPool_.push_back(std::uniform_int_distribution<uint32_t>(
        0, facebook::cachelib::util::narrow_cast<uint32_t>(numOpsForPool) - 1));
    keyIndicesForPool_.push_back(std::vector<uint32_t>(numOpsForPool));

    duration += covered::executeParallel(
        [&, this](size_t start, size_t end, size_t local_thread_idx) {
          //std::mt19937_64 gen(folly::Random::rand64());
          // Siyuan: use global_thread_idx as the deterministic seed to ensure that multiple clients generate different sets of requests/workload-items
          // Siyuan: we need this->config_.numThreads + 1, as Parallel may create an extra thread to generate remaining requests
          uint32_t global_thread_idx = this->client_idx_ * (this->config_.numThreads + 1) + local_thread_idx;
          std::mt19937_64 gen(global_thread_idx);
          auto popDist = workloadDist_[idx].getPopDist(left, right); // FastDiscreteDistribution
          for (uint64_t j = start; j < end; j++) {
            keyIndicesForPool_[i][j] =
                facebook::cachelib::util::narrow_cast<uint32_t>((*popDist)(gen));
          }
        },
        config_.numThreads, numOpsForPool);
  }

  // Siyuan: disable unnecessary outputs
  //std::cout << folly::sformat("Generated access patterns in {:.2f} mins",
  //                            duration.count() / 60.)
  //          << std::endl;
}

} // namespace covered

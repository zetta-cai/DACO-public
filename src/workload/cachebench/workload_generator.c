#include "workload/cachebench/workload_generator.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

#include "common/util.h"

namespace covered {

const std::string WorkloadGenerator::kClassName("WorkloadGenerator");

WorkloadGenerator::WorkloadGenerator(const StressorConfig& config, const uint32_t& client_idx, const uint32_t& perclient_workercnt, const bool& is_zipf_generator, const float& zipf_alpha)
    : config_{config}, client_idx_(client_idx), perclient_workercnt_(perclient_workercnt) {
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

    is_zipf_generator_ = is_zipf_generator;
    zipf_alpha_ = zipf_alpha;
  }

  if (config_.numKeys > std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument(folly::sformat(
        "Too many keys specified: {}. Maximum allowed is 4 Billion.",
        config_.numKeys));
  }

  generateReqs(); // Generate dataset items including key sizes, key chars, and value sizes
  generateKeyDistributions(); // Generate workload items
}

// gen is passed by each per-client worker to get different requests
const facebook::cachelib::cachebench::Request& WorkloadGenerator::getReq(const uint32_t& local_client_worker_idx, uint8_t poolId,
                                         std::mt19937_64& gen,
                                         std::optional<uint64_t>) {
  const uint32_t perclient_workercnt = perworkerKeyIndicesForPool_.size();
  XDCHECK_LT(local_client_worker_idx, perclient_workercnt);
  XDCHECK_LT(local_client_worker_idx, perclient_workercnt);

  const uint32_t poolcnt = perworkerKeyIndicesForPool_[local_client_worker_idx].size();
  XDCHECK_LT(poolId, poolcnt);
  XDCHECK_LT(poolId, poolcnt);

  size_t indice = perworkerKeyIndicesForPool_[local_client_worker_idx][poolId][perworkerKeyGenForPool_[local_client_worker_idx][poolId](gen)];
  auto op =
      static_cast<facebook::cachelib::cachebench::OpType>(workloadDist_[workloadIdx(poolId)].sampleOpDist(gen));
  reqs_[indice].setOp(op);
  return reqs_[indice];
}

const facebook::cachelib::cachebench::Request& WorkloadGenerator::getDatasetReq(uint32_t itemidx)
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

// Siyuan: quick operations for warmup speedup

void WorkloadGenerator::quickDatasetGet(const std::string& key, uint32_t& value_size) const
{
  // Lookup key
  std::unordered_map<std::string, uint32_t>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
  if (tmp_iter == dataset_lookup_table_.end())
  {
    // Must exist
    std::ostringstream oss;
    oss << "key " << key << " should exist in dataset_lookup_table_ (lookup table size: " << dataset_lookup_table_.size() << "; dataset size: " << reqs_.size() << ")";
    Util::dumpErrorMsg(kClassName, oss.str());
    exit(1);
  }

  // Get value size
  const uint32_t reqs_index = tmp_iter->second;
  assert(reqs_index < reqs_.size());
  const facebook::cachelib::cachebench::Request& tmp_facebook_req = reqs_[reqs_index];
  value_size = static_cast<uint32_t>(*(tmp_facebook_req.sizeBegin));

  return;
}

void WorkloadGenerator::quickDatasetPut(const std::string& key, const uint32_t& value_size)
{
  // Lookup key
  std::unordered_map<std::string, uint32_t>::const_iterator tmp_iter = dataset_lookup_table_.find(key);
  assert(tmp_iter != dataset_lookup_table_.end()); // Must exist

  // Put value size
  const uint32_t reqs_index = tmp_iter->second;
  assert(reqs_index < reqs_.size());
  facebook::cachelib::cachebench::Request& tmp_facebook_req = reqs_[reqs_index];
  *(tmp_facebook_req.sizeBegin) = value_size;

  return;
}

void WorkloadGenerator::quickDatasetDel(const std::string& key)
{
  quickDatasetPut(key, 0); // Use value size of 0 to simulate deletion (NOT affect cache stable performance in evaluation, as quick operations are ONLY used for warmup speedup and Facebook CDN is read-only trace without delete operations)

  return;
}

// Siyuan: For dynamic workload patterns

const std::vector<uint32_t>& WorkloadGenerator::getRankedKeyIndicesConstRef(const uint32_t local_client_worker_idx, const uint8_t poolId)
{
  // Get the const reference of current client worker's ranked key indices
  assert(local_client_worker_idx < perworker_perpool_ranked_key_indices_.size());
  return perworker_perpool_ranked_key_indices_[local_client_worker_idx][poolId];
}

void WorkloadGenerator::generateKeys() {
  uint8_t pid = 0;
  auto fn = [pid, this](size_t start, size_t end, size_t local_thread_idx) -> void {
    // All keys are printable lower case english alphabet.
    std::uniform_int_distribution<char> charDis('a', 'z');
    //std::mt19937_64 gen(folly::Random::rand64());
    // Siyuan: use Util::DATASET_KVPAIR_GENERATION_SEED as the deterministic seed to ensure that multiple clients generate the same set of key-value pairs
    if (start < end) // Siyuan: avoid effect of src/workload/cachebench/parallel.h::34
    {
      assert(local_thread_idx == 0); // Siyuan: local_thread_idx MUST be 0
    }
    std::mt19937_64 gen(Util::DATASET_KVPAIR_GENERATION_SEED + local_thread_idx);
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
    // Siyuan: use 1 thread to generate dataset keys to avoid perclient_workercnt/dataset_loadercnt affecting dataset
    size_t tmp_num_threads = 1;
    keyGenDuration += covered::executeParallel(
        fn, tmp_num_threads, numKeysForPool, firstKeyIndexForPool_[i]);
    //keyGenDuration += covered::executeParallel(
    //    fn, config_.numThreads, numKeysForPool, firstKeyIndexForPool_[i]);
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
  generateFirstKeyIndexForPool(); // Siyuan: we only use 1 pool by default (key index ranges from 0 to numkeys - 1)
  generateKeys(); // Siyuan: follow Facebook distribution to generate key sizes and fill with random characteristics
  //std::mt19937_64 gen(folly::Random::rand64());
  // Siyuan: Util::DATASET_KVPAIR_GENERATION_SEED as the deterministic seed to ensure that multiple clients generate the same set of key-value pairs
  std::mt19937_64 gen(Util::DATASET_KVPAIR_GENERATION_SEED);
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

      // Siyuan: update dataset lookup table to support quick operations for warmup speedup
      std::unordered_map<std::string, uint32_t>::iterator tmp_iter = dataset_lookup_table_.find(keys_[j]);
      assert(tmp_iter == dataset_lookup_table_.end()); // Must NOT exist
      dataset_lookup_table_.insert(std::pair(keys_[j], j));

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
  oss << "dataset: first key: " << reqs_[0].key << "; valuesize: " << (*reqs_[0].sizeBegin);
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
  for (uint32_t local_client_worker_idx = 0; local_client_worker_idx < perclient_workercnt_; local_client_worker_idx++)
  {
    std::vector<std::vector<uint32_t>> tmp_key_indices_for_pool;
    std::vector<std::uniform_int_distribution<uint32_t>> tmp_key_gen_for_pool;

    // Siyuan: For dynamic workload patterns
    std::vector<std::vector<uint32_t>> tmp_ranked_key_indices_for_pool;
    
    for (uint64_t i = 0; i < config_.opPoolDistribution.size(); i++) {
      auto left = firstKeyIndexForPool_[i];
      auto right = firstKeyIndexForPool_[i + 1] - 1;
      size_t idx = workloadIdx(i); // Siyuan: must be 0 as workloadDist_.size() is 1 due to a single pool

      size_t numOpsForPool = std::min<size_t>(
          facebook::cachelib::util::narrow_cast<size_t>(config_.numOps * config_.numThreads *
                                    config_.opPoolDistribution[i]),
          std::numeric_limits<uint32_t>::max());

      // Siyuan: disable unnecessary outputs
      //std::cout << folly::sformat("Generating {:.2f}M sampled accesses",
      //                            numOpsForPool / 1e6)
      //          << std::endl;

      tmp_key_gen_for_pool.push_back(std::uniform_int_distribution<uint32_t>(
          0, facebook::cachelib::util::narrow_cast<uint32_t>(numOpsForPool) - 1)); // Siyuan: used for randomly selecting one workload item from all workload items online

      // Siyuan: generate all workload items
      if (!is_zipf_generator_) // Use default workload generator of Facebook CDN
      {
        tmp_key_indices_for_pool.push_back(std::vector<uint32_t>(numOpsForPool));

        // Siyuan: use 1 thread to generate workload indices to avoid perclient_workercnt/dataset_loadercnt affecting workloads
        size_t tmp_num_threads = 1;
        duration += covered::executeParallel(
            [&, this](size_t start, size_t end, size_t local_thread_idx) {
              //std::mt19937_64 gen(folly::Random::rand64());
              // Siyuan: use global_thread_idx as the deterministic seed to ensure that multiple clients generate different sets of requests/workload-items
              // (OBSOLETE) Siyuan: we need this->config_.numThreads + 1, as Parallel may create an extra thread to generate remaining requests
              //uint32_t global_thread_idx = this->client_idx_ * (this->config_.numThreads + 1) + local_thread_idx;
              if (start < end) // Siyuan: avoid effect of src/workload/cachebench/parallel.h::34
              {
                assert(local_thread_idx == 0); // Siyuan: local_thread_idx MUST be 0
              }
              // NOTE: pre-generate workload items for each client worker to fix memory issue of large-scale exp by single-node simulator -> NOT affect previous evaluation results of other experiments due to perclient_workercnt = 1 and local_client_workeridx = 0 by default (i.e., global_client_worker_idx = clientidx * perclient_workercnt + local_client_worker_idx = clientidx)
              uint32_t global_client_worker_idx = this->client_idx_ * this->perclient_workercnt_ + local_client_worker_idx;
              // uint32_t global_thread_idx = this->client_idx_ + local_thread_idx;
              uint32_t global_thread_idx = global_client_worker_idx + local_thread_idx;
              // (OBSOLETE: homogeneous cache access patterns is a WRONG assumption -> we should ONLY follow homogeneous workload distribution yet still with heterogeneous cache access patterns) NOTE: we use WORKLOAD_KVPAIR_GENERATION_SEED to generate workload items with homogeneous cache access patterns
              //uint32_t global_thread_idx = Util::WORKLOAD_KVPAIR_GENERATION_SEED + local_thread_idx;
              std::mt19937_64 gen(global_thread_idx);
              auto popDist = workloadDist_[idx].getPopDist(left, right); // FastDiscreteDistribution
              for (uint64_t j = start; j < end; j++) {
                tmp_key_indices_for_pool[i][j] =
                    facebook::cachelib::util::narrow_cast<uint32_t>((*popDist)(gen));
              }
            },
            tmp_num_threads, numOpsForPool);
            //config_.numThreads, numOpsForPool);
        
        // (2) Siyuan: Identify object ranks from default workload distribution for dynamic workload patterns
        std::vector<uint32_t> descending_sorted_key_indices;
        getDescendingSortedKeyIndices_(tmp_key_indices_for_pool[i], descending_sorted_key_indices);
        tmp_ranked_key_indices_for_pool.push_back(descending_sorted_key_indices);
      } // End of !is_zipf_generator_
      else // Use zipf workload generator to generate "synthetic" workloads
      {
        assert(zipf_alpha_ > 0.0);

        // (1) Siyuan: generate default workload items
        std::vector<uint32_t> tmp_default_key_indices(numOpsForPool);
        size_t tmp_num_threads = 1;
        duration += covered::executeParallel(
            [&, this](size_t start, size_t end, size_t local_thread_idx) {
              //std::mt19937_64 gen(folly::Random::rand64());
              // Siyuan: use global_thread_idx as the deterministic seed to ensure that multiple clients generate different sets of requests/workload-items
              // (OBSOLETE) Siyuan: we need this->config_.numThreads + 1, as Parallel may create an extra thread to generate remaining requests
              //uint32_t global_thread_idx = this->client_idx_ * (this->config_.numThreads + 1) + local_thread_idx;
              if (start < end) // Siyuan: avoid effect of src/workload/cachebench/parallel.h::34
              {
                assert(local_thread_idx == 0); // Siyuan: local_thread_idx MUST be 0
              }
              // NOTE: pre-generate workload items for each client worker to fix memory issue of large-scale exp by single-node simulator -> NOT affect previous evaluation results of other experiments due to perclient_workercnt = 1 and local_client_workeridx = 0 by default (i.e., global_client_worker_idx = clientidx * perclient_workercnt + local_client_worker_idx = clientidx)
              uint32_t global_client_worker_idx = this->client_idx_ * this->perclient_workercnt_ + local_client_worker_idx;
              // uint32_t global_thread_idx = this->client_idx_ + local_thread_idx;
              uint32_t global_thread_idx = global_client_worker_idx + local_thread_idx;
              // (OBSOLETE: homogeneous cache access patterns is a WRONG assumption -> we should ONLY follow homogeneous workload distribution yet still with heterogeneous cache access patterns) NOTE: we use WORKLOAD_KVPAIR_GENERATION_SEED to generate workload items with homogeneous cache access patterns
              //uint32_t global_thread_idx = Util::WORKLOAD_KVPAIR_GENERATION_SEED + local_thread_idx;
              std::mt19937_64 gen(global_thread_idx);
              auto popDist = workloadDist_[idx].getPopDist(left, right); // FastDiscreteDistribution
              for (uint64_t j = start; j < end; j++) {
                tmp_default_key_indices[j] =
                    facebook::cachelib::util::narrow_cast<uint32_t>((*popDist)(gen));
              }
            },
            tmp_num_threads, numOpsForPool);

        // (2) Siyuan: Identify object ranks from default workload distribution
        std::vector<uint32_t> descending_sorted_key_indices;
        getDescendingSortedKeyIndices_(tmp_default_key_indices, descending_sorted_key_indices);
        tmp_ranked_key_indices_for_pool.push_back(descending_sorted_key_indices);

        // (3) Siyuan: generate probs based on Zipf's law mentioned in CacheLib paper
        const uint32_t tmp_rank_cnt = descending_sorted_key_indices.size(); // E.g., descending_sorted_key_indices[0] = A for the rank 1
        std::vector<double> tmp_probs(tmp_rank_cnt, 0.0);
        double tmp_sum_prob = 0.0;
        for (uint32_t tmp_rank_idx = 1; tmp_rank_idx <= tmp_rank_cnt; tmp_rank_idx++)
        {
          tmp_probs[tmp_rank_idx - 1] = 1.0 / pow(tmp_rank_idx, zipf_alpha_);
          tmp_sum_prob += tmp_probs[tmp_rank_idx - 1];
        }
        for (uint32_t tmp_rank_idx = 0; tmp_rank_idx < tmp_rank_cnt; tmp_rank_idx++)
        {
          tmp_probs[tmp_rank_idx] /= tmp_sum_prob;
        }
        
        // (4) Siyuan: Use Zipf's law to generate workload items based on the descending_sorted_key_indices
        // Use client idx as random seed to generate workload items for the current client
        std::mt19937_64 tmp_randgen(this->client_idx_);
        std::discrete_distribution<uint32_t> tmp_dist(tmp_probs.begin(), tmp_probs.end());
        tmp_key_indices_for_pool.push_back(std::vector<uint32_t>(numOpsForPool));
        for (uint32_t tmp_i = 0; tmp_i < numOpsForPool; tmp_i++)
        {
            uint32_t tmp_rank_index_minus_one = tmp_dist(tmp_randgen); // From 0 (the most popular) to rank_cnt - 1 (the least popular)
            uint32_t tmp_key_index = descending_sorted_key_indices[tmp_rank_index_minus_one]; // E.g., get A for rank 1 at descending_sorted_key_indices[0]
            assert(tmp_key_index < reqs_.size());

            tmp_key_indices_for_pool[i][tmp_i] = tmp_key_index;
        }

      } // End of else for if(!is_zipf_generator_)
    } // End of for loop for each pool

    perworkerKeyIndicesForPool_.push_back(tmp_key_indices_for_pool);
    perworkerKeyGenForPool_.push_back(tmp_key_gen_for_pool);

    // Siyuan: For dynamic workload patterns
    perworker_perpool_ranked_key_indices_.push_back(tmp_ranked_key_indices_for_pool);
  } // End of loop for each local client worker

  // Siyuan: disable unnecessary outputs
  //std::cout << folly::sformat("Generated access patterns in {:.2f} mins",
  //                            duration.count() / 60.)
  //          << std::endl;
}

void WorkloadGenerator::getDescendingSortedKeyIndices_(const std::vector<uint32_t>& key_indices, std::vector<uint32_t>& descending_sorted_key_indices)
{
  std::map<uint32_t, uint32_t> tmp_keyindex_freq_map;
  for (uint32_t tmp_i = 0; tmp_i < key_indices.size(); tmp_i++)
  {
    const uint32_t tmp_keyindex = key_indices[tmp_i];
    if (tmp_keyindex_freq_map.find(tmp_keyindex) == tmp_keyindex_freq_map.end())
    {
      tmp_keyindex_freq_map.insert(std::pair(tmp_keyindex, 1));
    }
    else
    {
      tmp_keyindex_freq_map[tmp_keyindex]++;
    }
  }

  std::vector<std::pair<uint32_t, uint32_t>> tmp_keyindex_freq_vec;
  for (std::map<uint32_t, uint32_t>::iterator tmp_iter = tmp_keyindex_freq_map.begin(); tmp_iter != tmp_keyindex_freq_map.end(); tmp_iter++)
  {
    tmp_keyindex_freq_vec.push_back(std::pair(tmp_iter->first, tmp_iter->second));
  }
  sort(tmp_keyindex_freq_vec.begin(), tmp_keyindex_freq_vec.end(), descendingSortByValue);

  descending_sorted_key_indices.clear();
  for (uint32_t tmp_i = 0; tmp_i < tmp_keyindex_freq_vec.size(); tmp_i++)
  {
    descending_sorted_key_indices.push_back(tmp_keyindex_freq_vec[tmp_i].first);
  }

  return;
}

bool WorkloadGenerator::descendingSortByValue(std::pair<uint32_t, uint32_t>& a, std::pair<uint32_t, uint32_t>& b)
{
  const uint32_t a_freq = a.second;
  const uint32_t b_freq = b.second;
  return a_freq > b_freq;
}

} // namespace covered

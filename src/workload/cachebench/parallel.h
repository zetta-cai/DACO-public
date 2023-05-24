/*
 * Parallel: refer to lib/CacheLib/cachelib/cachebench/util/Parallel.h.
 *
 * Hack to set deterministic seeds for random generators under multi-client benchmark.
 * 
 * By Siyuan Sheng (2023.04.21).
 */

#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <vector>

namespace covered {

inline std::chrono::seconds executeParallel(
    std::function<void(size_t start, size_t end, size_t thread_idx)> fn,
    size_t numThreads,
    size_t count,
    size_t offset = 0) {
  numThreads = std::max(numThreads, 1UL);
  auto startTime = std::chrono::steady_clock::now();
  const size_t perThread = count / numThreads;
  std::vector<std::thread> processingThreads;
  for (size_t i = 0; i < numThreads; i++) {
    processingThreads.emplace_back([i, perThread, offset, &fn]() {
      size_t blockStart = offset + i * perThread;
      size_t blockEnd = blockStart + perThread;
      fn(blockStart, blockEnd, i);
    });
  }
  fn(offset + perThread * numThreads, offset + count, numThreads);
  for (auto& t : processingThreads) {
    t.join();
  }

  return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - startTime);
}

inline std::chrono::seconds executeParallel(std::function<void()> fn,
                                            size_t numThreads) {
  numThreads = std::max(numThreads, 1UL);
  auto startTime = std::chrono::steady_clock::now();
  std::vector<std::thread> processingThreads;
  for (size_t i = 0; i < numThreads; i++) {
    processingThreads.emplace_back([&fn]() { fn(); });
  }
  for (auto& t : processingThreads) {
    t.join();
  }

  return std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - startTime);
}
} // namespace covered
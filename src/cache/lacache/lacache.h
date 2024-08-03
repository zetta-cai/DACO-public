/*
 * LACache: refer to include/cache_common.hpp::PBSQueue and src/cache_la.cpp::LACacheSet in lib/lacache/Simulator/Delayed-Source-Code/caching/ (follow most variable names in original implementation for better understanding).
 *
 * Optimize some impractical tricks in original implementation, e.g., maintaining LRU list of entries yet never used when eviction; enumerating all cached objects to find that with the smallest rank score.
 * 
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2024.08.03).
 */

#ifndef LACACHE_H
#define	LACACHE_H

#include <list> // std::list
#include <numeric> // std::accumulate
#include <random> // std::random and srd::mt19937_64
#include <unordered_map> // std::unordered_map

#include "common/key.h"
#include "common/value.h"

namespace covered {
	class InTimes
	{
	public:
		InTimes();
		~InTimes();

		// Records a packet arrival corresponding to this flow.
		void recordArrivTimes(double itime);

		// Returns the payoff for this flow.
		double getLambda();

		uint64_t getSizeForCapacity() const;

		InTimes& operator=(const InTimes& other);
	private:
		static const std::string kClassName;

		static const uint64_t L_;

		std::list<double> CTimes_; // Siyuan: track the most recent L_ access times for the given object

		double num_windows_; // Siyuan: unused???
		double cumulative_itimes_; // Siyuan: unused???
	};

	class CacheInfo
	{
	public:
		CacheInfo();
		CacheInfo(const Key& key, const Value& value, const uint64_t& miss_latency);
		~CacheInfo();

		Key getKey() const;
		Value getValue() const;
		void setValue(const Value& value);
		uint64_t getMissLatency() const;
		bool isUse2() const;

		uint64_t getSizeForCapacity() const;

		CacheInfo& operator=(const CacheInfo& other);
	private:
		static const std::string kClassName;

		Key key_;
		Value value_;
		// Siyuan: for cooperative caching, LA-Cache should maintain miss latency for each object, instead of a fixed value for the whole cache
		uint64_t miss_latency_; // In units of us
		bool use2_; // True only if miss_latency_ <= 1000000
	};

	class ArrivalInfo
	{
	public:
		ArrivalInfo();
		ArrivalInfo(const uint64_t& lrt, const InTimes& inter_times, const double& lambda);
		~ArrivalInfo();

		uint64_t getLrt() const;
		void setLrt(const uint64_t& lrt);

		double getLambda() const;

		void updateForNewInterTime(const double& inter_time); // Update inter_times_ and lambda

		uint64_t getSizeForCapacity() const;

		ArrivalInfo& operator=(const ArrivalInfo& other);
	private:
		static const std::string kClassName;

		uint64_t lrt_; // Last arrival time
		InTimes inter_times_; // Inter-arrival times
		double lambda_; // Calculated lambda for rank score calculation
	};

	class LACache
	{
	private:
		// typedef std::multimap<double, CacheInfo>::iterator cachedobj_map_iter_t;
		// typedef std::multimap<double, CacheInfo>::const_iterator cachedobj_map_const_iter_t;
		// typedef std::unordered_map<Key, cachedobj_map_iter_t, KeyHasher>::iterator lookup_map_iter_t;

		typedef std::list<CacheInfo>::iterator cachedobj_list_iter_t;
		typedef std::list<CacheInfo>::const_iterator cachedobj_list_const_iter_t;
		typedef std::unordered_map<Key, cachedobj_list_iter_t, KeyHasher>::iterator lookup_map_iter_t;
		typedef std::unordered_map<Key, cachedobj_list_iter_t, KeyHasher>::const_iterator lookup_map_const_iter_t;

		typedef std::unordered_map<Key, ArrivalInfo, KeyHasher>::iterator allobj_map_iter_t;
		typedef std::unordered_map<Key, ArrivalInfo, KeyHasher>::const_iterator allobj_map_const_iter_t;
	public:
		LACache();
		~LACache();
		
		bool get(const Key& key, Value& value);
		bool update(const Key& key, const Value& value);

		bool needAdmit(const Key& key) const; // Refer to write() in src/cache_la.cpp::LACacheSet
		void admit(const Key& key, const Value& value, const uint64_t& miss_latency);
		bool getVictimKey(Key& key) const;
		bool evictWithGivenKey(const Key& key, Value& value); // Refer to popMin() in src/cache_common.cpp::PBSQueue
		
		bool exists(const Key& key) const;
		
		uint64_t getSizeForCapacity() const;
		
	private:
		static const std::string kClassName;

		static const double BWidth;
		static const int SAMPLE_SIZE;

		void updateArrivalInfo_(const Key& key); // Update arrival info for each cached/uncached object (refer to updateFreqs_() in src/cache_la.cpp::LACacheSet)

		// void updateEvictRule_(const Key& key); // Update rank score and lookup table for each cached object

		double calculateRankScore_(const CacheInfo& cache_info, const ArrivalInfo& arrival_info) const; // Refer to update_evict() in src/cache_common.cpp::PBSQueue

		// In units of bytes
		uint64_t size_;

		// In units of packets
		uint64_t timer_;

		// For random sampling
		std::mt19937_64 randgen_;

		// For cached objects (OBSOLETE due to time-varying rank scores)
		// std::multimap<double, CacheInfo> evictrule_entry_map_; // Store key-value pair for each rank score (ordered by rank scores in ascending order)
		// std::unordered_map<Key, cachedobj_map_iter_t, KeyHasher> positions_; // Use cachedobj_map_iter_t to locate the corresponding entry in evictrule_entry_map_

		// For cached objects
		std::list<CacheInfo> entries_; // Store key-value pairs
		std::unordered_map<Key, cachedobj_list_iter_t, KeyHasher> positions_; // Use cachedobj_list_iter_t to locate the corresponding entry in entries_

		// For all objects (NOTE: LA-Cache only admits object if its lambda > 0.0)
		std::unordered_map<Key, ArrivalInfo, KeyHasher> perkey_arrivalinfo_; // Store arrival information for each cached/uncached object
	};
} // namespace covered

#endif	/* LACACHE_H */
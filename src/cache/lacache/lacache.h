/*
 * LACache: refer to include/cache_common.hpp::PBSQueue and src/cache_la.cpp::LACacheSet in lib/lacache/Simulator/Delayed-Source-Code/caching/ (follow most variable names in original implementation for better understanding).
 *
 * Optimize some tricks in original implementation, e.g., maintaining LRU list of entries yet never used when eviction; enumerating all cached objects to find that with the smallest rank score.
 * 
 * Hack to support required interfaces and cache size in units of bytes for capacity constraint.
 * 
 * By Siyuan Sheng (2024.08.03).
 */

#ifndef LACACHE_H
#define	LACACHE_H

#include <map> // std::multimap
#include <unordered_map> // std::unordered_map

#include "common/key.h"
#include "common/value.h"

namespace covered {
	// TODO: END HERE
	class InTimes{
		private:
			double num_windows_ = 0;
			double cumulative_itimes_ = 0;

		public:
			uint64_t L = 20;
			std::list<double> CTimes;
			/**
			 * Records a packet arrival corresponding to this flow.
			 */
			void recordArrivTimes(double itime) {
				//num_windows_++;
				//cumulative_itimes_ += itime;
				CTimes.push_back(itime);
				if(CTimes.size() > L){
					CTimes.pop_front();
				}
			}
			/**
			 * Returns the payoff for this flow.
			 */
			double getLambda() {
				double Lamb = 0.00000001;
				if(CTimes.size() >= 3){
					double Sum = std::accumulate(CTimes.begin(),CTimes.end(),0);
					Lamb = 1.0 / (Sum / (double)CTimes.size());
				}
				return Lamb;
			}
		};

	class ArrivalInfo
	{
	public:
	private:
		uint64_t lrt_; // Last arrival time
	};

	class LACache {
		private:
			typedef std::pair<Key, Value> key_value_pair_t;
			typedef std::multimap<double, key_value_pair_t>::iterator multimap_iterator_t;
		public:
			LruCache();
			~LruCache();
			
			bool get(const Key& key, Value& value);
			bool update(const Key& key, const Value& value);

			void admit(const Key& key, const Value& value);
			bool getVictimKey(Key& key) const;
			bool evictWithGivenKey(const Key& key, Value& value);
			
			bool exists(const Key& key) const;
			
			uint64_t getSizeForCapacity() const;
			
		private:
			static const std::string kClassName;

			// In units of bytes
			uint64_t size_;

			// For cached objects
			std::multimap<double, key_value_pair_t> evictrule_entry_map_; // Store key-value pair for each rank score (ordered by rank scores in ascending order)
			std::unordered_map<Key, multimap_iterator_t, KeyHasher> positions_; // Use multimap_iterator_t to locate the corresponding entry in evictrule_entry_map_

			// For uncached objects (NOTE: LA-Cache only admits object if its lambda > 0.0)
			std::unordered_map<std::string,uint64_t> LRTs;
			std::unordered_map<std::string,InTimes> InterTimes;
			std::unordered_map<std::string,double> Lambdas;
	};
} // namespace covered

#endif	/* LRUCACHE_H */
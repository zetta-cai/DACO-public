#include "cache/lacache/lacache.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
	// InTimes
	
	const std::string InTimes::kClassName("InTimes");
	const uint64_t InTimes::L_ = 20;

	InTimes::InTimes()
	{
		CTimes_.clear();
		num_windows_ = 0;
		cumulative_itimes_ = 0;
	}

	InTimes::~InTimes() {}

	void InTimes::recordArrivTimes(double itime)
	{
		//num_windows_++;
		//cumulative_itimes_ += itime;
		CTimes_.push_back(itime);
		if (CTimes_.size() > L_)
		{
			CTimes_.pop_front();
		}

		return;
	}

	double InTimes::getLambda()
	{
		double Lamb = 0.00000001;
		if (CTimes_.size() >= 3)
		{
			double Sum = std::accumulate(CTimes_.begin(), CTimes_.end(), 0);
			Lamb = 1.0 / (Sum / (double)CTimes_.size());
		}
		return Lamb;
	}

	uint64_t InTimes::getSizeForCapacity() const
	{
		// NOTE: NOT count num_windows_ and cumulative_itimes_ due to unused???
		return sizeof(double) * CTimes_.size();
	}

	InTimes& InTimes::operator=(const InTimes& other)
	{
		if (this != &other)
		{
			CTimes_ = other.CTimes_; // Deep copy
			num_windows_ = other.num_windows_;
			cumulative_itimes_ = other.cumulative_itimes_;
		}
		return *this;
	}

	// CacheInfo

	const std::string CacheInfo::kClassName("CacheInfo");

	CacheInfo::CacheInfo() : key_(), value_(), miss_latency_(0), use2_(false)
	{}

	CacheInfo::CacheInfo(const Key& key, const Value& value, const uint64_t& miss_latency) : key_(key), value_(value), miss_latency_(miss_latency), use2_(false)
	{
		if(miss_latency <= 1000000) // 1s
		{
			use2_ = true;
		}
	}

	CacheInfo::~CacheInfo() {}

	Key CacheInfo::getKey() const
	{
		return key_;
	}

	Value CacheInfo::getValue() const
	{
		return value_;
	}

	void CacheInfo::setValue(const Value& value)
	{
		value_ = value;
		return;
	}

	uint64_t CacheInfo::getMissLatency() const
	{
		return miss_latency_;
	}

	bool CacheInfo::isUse2() const
	{
		return use2_;
	}

	uint64_t CacheInfo::getSizeForCapacity() const
	{
		return key_.getKeyLength() + value_.getValuesize() + sizeof(uint64_t) + sizeof(bool);
	}

	CacheInfo& CacheInfo::operator=(const CacheInfo& other)
	{
		if (this != &other)
		{
			key_ = other.key_;
			value_ = other.value_;
			miss_latency_ = other.miss_latency_;
		}
		return *this;
	}

	// ArrivalInfo

	const std::string ArrivalInfo::kClassName("ArrivalInfo");

	ArrivalInfo::ArrivalInfo() : lrt_(0), inter_times_(), lambda_(0)
	{
	}

	ArrivalInfo::ArrivalInfo(const uint64_t& lrt, const InTimes& inter_times, const double& lambda) : lrt_(lrt), inter_times_(inter_times), lambda_(lambda)
	{
	}

	ArrivalInfo::~ArrivalInfo() {}

	uint64_t ArrivalInfo::getLrt() const
	{
		return lrt_;
	}

	void ArrivalInfo::setLrt(const uint64_t& lrt)
	{
		lrt_ = lrt;
		return;
	}

	double ArrivalInfo::getLambda() const
	{
		return lambda_;
	}

	void ArrivalInfo::updateForNewInterTime(const double& inter_time)
	{
		// Update inter-arrival times in the arrival info
		inter_times_.recordArrivTimes(inter_time);

		// Update lambda in the arrival info
		lambda_ = inter_times_.getLambda();

		return;
	}

	uint64_t ArrivalInfo::getSizeForCapacity() const
	{
		return sizeof(uint64_t) + inter_times_.getSizeForCapacity() + sizeof(double);
	}

	ArrivalInfo& ArrivalInfo::operator=(const ArrivalInfo& other)
	{
		if (this != &other)
		{
			lrt_ = other.lrt_;
			inter_times_ = other.inter_times_;
			lambda_ = other.lambda_;
		}
		return *this;
	}

	// LACache

	const std::string LACache::kClassName("LACache");
	const double LACache::BWidth = 104857600.0;
	const int LACache::SAMPLE_SIZE = 100;

	LACache::LACache() : size_(0), timer_(0), randgen_ptr_(NULL)
	{
		randgen_ptr_ = new std::mt19937_64(0);
		assert(randgen_ptr_ != NULL);

		// evictrule_entry_map_.clear();

		entries_.clear();
		positions_.clear();
		perkey_arrivalinfo_.clear();

		size_ += sizeof(uint64_t); // For timer_
	}

	LACache::~LACache()
	{
		assert(randgen_ptr_ != NULL);
		delete randgen_ptr_;
		randgen_ptr_ = NULL;
	}

	bool LACache::get(const Key& key, Value& value)
	{
		bool is_local_cached = false;

		lookup_map_iter_t lookup_map_iter = positions_.find(key);
		if (lookup_map_iter == positions_.end()) // locally uncached object
		{
			is_local_cached = false;

			updateArrivalInfo_(key); // Update arrival info
		}
		else
		{
			// cachedobj_map_iter_t cachedobj_map_iter = lookup_map_iter->second;
			// assert(cachedobj_map_iter != evictrule_entry_map_.end());
			// const CacheInfo& cache_info_const_ref = cachedobj_map_iter->second;

			cachedobj_list_iter_t cachedobj_list_iter = lookup_map_iter->second;
			assert(cachedobj_list_iter != entries_.end());
			const CacheInfo& cache_info_const_ref = *cachedobj_list_iter;

			// Key check
			if (cache_info_const_ref.getKey() != key)
			{
				std::ostringstream oss;
				oss << "get() for key " << key.getKeyDebugstr() << " failed due to mismatched key of cache_info_const_ref.getKey() " << cache_info_const_ref.getKey().getKeyDebugstr();
				Util::dumpErrorMsg(kClassName, oss.str());
				exit(1);
			}

			// Get value
			value = cache_info_const_ref.getValue();
			is_local_cached = true;

			updateArrivalInfo_(key); // Update arrival info

			// // NOTE: cachedobj_map_iter and cache_info_const_ref become invalid!!!
			// updateEvictRule_(key); // Update rank score and lookup table
		}

		return is_local_cached;
	}

	bool LACache::needAdmit(const Key& key) const
	{
		bool need_admit = false;

		lookup_map_const_iter_t lookup_map_const_iter = positions_.find(key);
		if (lookup_map_const_iter == positions_.end()) // Not cached yet
		{
			allobj_map_const_iter_t allobj_map_const_iter = perkey_arrivalinfo_.find(key);
			if (allobj_map_const_iter != perkey_arrivalinfo_.end()) // Non-first arrival access
			{
				const ArrivalInfo& arrival_info_const_ref = allobj_map_const_iter->second;
				if (arrival_info_const_ref.getLambda() > 0.0)
				{
					need_admit = true;
				}
			}	
		}

		return need_admit;
	}

	bool LACache::update(const Key& key, const Value& value)
	{
		bool is_local_cached = false;

		lookup_map_iter_t lookup_map_iter = positions_.find(key);
		if (lookup_map_iter != positions_.end()) // Already cached
		{
			is_local_cached = true;

			// cachedobj_map_iter_t cachedobj_map_iter = lookup_map_iter->second;
			// assert(cachedobj_map_iter != evictrule_entry_map_.end());
			// CacheInfo& cache_info_ref = cachedobj_map_iter->second;

			cachedobj_list_iter_t cachedobj_list_iter = lookup_map_iter->second;
			assert(cachedobj_list_iter != entries_.end());
			CacheInfo& cache_info_ref = *cachedobj_list_iter;
			const uint64_t cacheinfo_size_before_update = cache_info_ref.getSizeForCapacity();

			// Update the object with new value
			cache_info_ref.setValue(value);

			// Update cache size usage
			const uint64_t cacheinfo_size_after_update = cache_info_ref.getSizeForCapacity();
			if (cacheinfo_size_after_update > cacheinfo_size_before_update)
			{
				size_ = Util::uint64Add(size_, cacheinfo_size_after_update - cacheinfo_size_before_update);
			}
			else if (cacheinfo_size_after_update < cacheinfo_size_before_update)
			{
				size_ = Util::uint64Minus(size_, cacheinfo_size_before_update - cacheinfo_size_after_update);
			}

			updateArrivalInfo_(key); // Update arrival info

			// // NOTE: cachedobj_map_iter and cache_info_ref become invalid!!!
			// updateEvictRule_(key); // Update rank score and lookup table
		}
		else // Not cached yet
		{
			is_local_cached = false;

			updateArrivalInfo_(key); // Update arrival info
		}

		return is_local_cached;
	}

	void LACache::admit(const Key& key, const Value& value, const uint64_t& miss_latency)
	{
		lookup_map_iter_t lookup_map_iter = positions_.find(key);
		if (lookup_map_iter != positions_.end()) // Already cached
		{
			uint32_t list_index = static_cast<uint32_t>(std::distance(entries_.begin(), lookup_map_iter->second));

			std::ostringstream oss;
			oss << "key " << key.getKeyDebugstr() << " already exists in entries_ (list index: " << list_index << "; list size: " << entries_.size() << ") for admit()";
			Util::dumpWarnMsg(kClassName, oss.str());
			return;
		}
		else // No previous list and map entry
		{
			// // Insert CacheInfo with rank score = 0
			// cachedobj_map_iter_t cachedobj_map_iter = evictrule_entry_map_.insert(std::pair<double, CacheInfo>(0, CacheInfo(key, value, miss_latency)));
			// assert(cachedobj_map_iter != evictrule_entry_map_.end());

			// Insert CacheInfo
			entries_.push_back(CacheInfo(key, value, miss_latency));
			cachedobj_list_iter_t cachedobj_list_iter = std::prev(entries_.end());
			assert(cachedobj_list_iter != entries_.end());
			size_ = Util::uint64Add(size_, cachedobj_list_iter->getSizeForCapacity());

			// Update lookup table
			// lookup_map_iter = positions_.insert(std::pair<Key, cachedobj_map_iter_t>(key, cachedobj_map_iter)).first;
			lookup_map_iter = positions_.insert(std::pair<Key, cachedobj_list_iter_t>(key, cachedobj_list_iter)).first;
			assert(lookup_map_iter != positions_.end());
			size_ = Util::uint64Add(size_, key.getKeyLength() + sizeof(cachedobj_list_iter_t));

			// NOTE: NO need to update arrival info here, which has been done for the get/put request triggerring the admission

			// // NOTE: cachedobj_map_iter becomes invalid!!!
			// updateEvictRule_(key); // Update rank score and lookup table
		}

		return;
	}

	bool LACache::getVictimKey(Key& key) const
	{
		bool has_victim_key = false;
		if (entries_.size() > 0)
		{
			// OBSOLETE due to time-varying rank scores (changed with timer_)
			// // Select victim by rank score
			// cachedobj_map_const_iter_t victim_const_iter = evictrule_entry_map_.begin();
			// assert(victim_const_iter != evictrule_entry_map_.end());
			// key = victim_const_iter->second.getKey();

			// NOTE: follow existing recency-related caching studies (e.g., LHD and LRB) to perform eviction with random sampling

			// Randomly sample 100 cached objects
			const std::list<CacheInfo>* sampled_cacheinfos_ptr = NULL;
			std::list<CacheInfo> sampled_cacheinfos;
			sampled_cacheinfos.clear();
			if (entries_.size() <= SAMPLE_SIZE)
			{
				sampled_cacheinfos_ptr = &entries_;
			}
			else
			{
				// Sample without replacement
				// NOTE: NO need to resize sampled_cacheinfos, as std::back_inserter will choose the back() iterator to insert the sampled cacheinfos
				std::sample(entries_.begin(), entries_.end(), std::back_inserter(sampled_cacheinfos), SAMPLE_SIZE, *randgen_ptr_); // Not sure why use randgen_ cannot pass compilation

				sampled_cacheinfos_ptr = &sampled_cacheinfos;
			}
			assert(sampled_cacheinfos_ptr != NULL);
			assert(sampled_cacheinfos_ptr->size() <= SAMPLE_SIZE);

			// Calculate rank scores for sampled cache infos and select that with the smallest rank score as victim
			double min_rank_score = 0;
			Key min_key;
			for (cachedobj_list_const_iter_t tmp_iter = sampled_cacheinfos_ptr->begin(); tmp_iter != sampled_cacheinfos_ptr->end(); tmp_iter++)
			{
				const Key tmp_key = tmp_iter->getKey();

				// Cached object must exist arrival info
				allobj_map_const_iter_t tmp_allobj_map_const_iter = perkey_arrivalinfo_.find(tmp_key);
				assert(tmp_allobj_map_const_iter != perkey_arrivalinfo_.end());
				const ArrivalInfo& tmp_arrival_info_const_ref = tmp_allobj_map_const_iter->second;

				// Calculate rank score
				double tmp_rank_score = calculateRankScore_(*tmp_iter, tmp_arrival_info_const_ref);

				if (tmp_iter == sampled_cacheinfos_ptr->begin() || tmp_rank_score < min_rank_score)
				{
					min_rank_score = tmp_rank_score;
					min_key = tmp_key;
				}
			}

			key = min_key;
			has_victim_key = true;
		}
		return has_victim_key;
	}
    
	bool LACache::evictWithGivenKey(const Key& key, Value& value)
	{
		bool is_evict = false;
		
		lookup_map_iter_t lookup_map_iter = positions_.find(key);
		if (lookup_map_iter != positions_.end()) // Key exists
		{
			// Must exist cache info
			cachedobj_list_iter_t cachedobj_list_iter = lookup_map_iter->second;
			assert(cachedobj_list_iter != entries_.end());
			const uint64_t cacheinfo_size = cachedobj_list_iter->getSizeForCapacity();

			// Remove cache info
			entries_.erase(cachedobj_list_iter);
			size_ = Util::uint64Minus(size_, cacheinfo_size);

			// Remove lookup table
			positions_.erase(lookup_map_iter);
			size_ = Util::uint64Minus(size_, key.getKeyLength() + sizeof(cachedobj_list_iter_t));

			// NOTE: NO need to remove arrival info!

			is_evict = true;
		}

		return is_evict;
	}

	bool LACache::exists(const Key& key) const
	{
		return positions_.find(key) != positions_.end();
	}

	uint64_t LACache::getSizeForCapacity() const
	{
		return size_;
	}

	void LACache::updateArrivalInfo_(const Key& key)
	{
		allobj_map_iter_t allobj_map_iter = perkey_arrivalinfo_.find(key);

		if (allobj_map_iter != perkey_arrivalinfo_.end()) // Not the first access (already track the arrival info)
		{
			ArrivalInfo& tmp_arrival_info_ref = allobj_map_iter->second;
			const uint64_t tmp_arrival_info_size_before_update = tmp_arrival_info_ref.getSizeForCapacity();

			// Calculate inter-arrival time for current access
			const uint64_t tmp_lrt = tmp_arrival_info_ref.getLrt();
            double tmp_intertime = (timer_ - tmp_lrt) / 1.0;
			assert(tmp_intertime > 0);

			// Update inter-arrival times and lambda in the arrival info
			tmp_arrival_info_ref.updateForNewInterTime(tmp_intertime);

			// Update last arrival time
			tmp_arrival_info_ref.setLrt(timer_);

			// Update cache size usage
			const uint64_t tmp_arrival_info_size_after_update = tmp_arrival_info_ref.getSizeForCapacity();
			if (tmp_arrival_info_size_after_update > tmp_arrival_info_size_before_update)
			{
				size_ = Util::uint64Add(size_, tmp_arrival_info_size_after_update - tmp_arrival_info_size_before_update);
			}
			else if (tmp_arrival_info_size_after_update < tmp_arrival_info_size_before_update)
			{
				size_ = Util::uint64Minus(size_, tmp_arrival_info_size_before_update - tmp_arrival_info_size_after_update);
			}
        }
		else // The first access (NOT track arrival info yet)
		{
			ArrivalInfo tmp_arrival_info(timer_, InTimes(), 0.0);
			perkey_arrivalinfo_.insert(std::pair<Key, ArrivalInfo>(key, tmp_arrival_info));

			// Update cache size usage
			size_ = Util::uint64Add(size_, key.getKeyLength() + tmp_arrival_info.getSizeForCapacity());
		}

		timer_ += 1;

		return;
	}

	// void LACache::updateEvictRule_(const Key& key)
	// {
	// 	// NOTE: only invoke updateEvictRule_() for cached objects
	// 	lookup_map_iter_t lookup_map_iter = positions_.find(key);
	// 	assert(lookup_map_iter != positions_.end());
	// 	cachedobj_map_iter_t cachedobj_map_iter = lookup_map_iter->second;
	// 	assert(cachedobj_map_iter != evictrule_entry_map_.end());
	// 	const CacheInfo& cache_info_const_ref = cachedobj_map_iter->second;

	// 	// NOTE: cached object MUST have arrival info, as LA-Cache admits object only if its lambda > 0.0
	// 	allobj_map_iter_t allobj_map_iter = perkey_arrivalinfo_.find(key);
	// 	assert(allobj_map_iter != perkey_arrivalinfo_.end());
	// 	const ArrivalInfo& arrival_info_const_ref = allobj_map_iter->second;

	// 	// Calculate rank score
	// 	double rank_score = calculateRankScore_(cache_info_const_ref, arrival_info_const_ref);

	// 	// Replace old rank score with new rank score (NOT change cache size)
	// 	cachedobj_map_iter_t new_cachedobj_map_iter = evictrule_entry_map_.insert(std::pair<double, CacheInfo>(rank_score, cache_info_const_ref));
	// 	assert(new_cachedobj_map_iter != evictrule_entry_map_.end());
	// 	evictrule_entry_map_.erase(cachedobj_map_iter);

	// 	// Update lookup table (NOT change cache size)
	// 	lookup_map_iter->second = new_cachedobj_map_iter;

	// 	return;
	// }

	double LACache::calculateRankScore_(const CacheInfo& cache_info, const ArrivalInfo& arrival_info) const
	{
		// Calculate lambda_i in paper
		double lrt = timer_ - arrival_info.getLrt() + 1.0;
		double size = (cache_info.getKey().getKeyLength() + cache_info.getValue().getValuesize()) + 1.0;
		double glambda = arrival_info.getLambda();
		if (cache_info.isUse2() && lrt >= 12.0 * 1.0 / glambda)
		{
			glambda = 1.0 / lrt;
		}

		// Calculate lambda_i * L_i in paper
		double LT = glambda * (cache_info.getMissLatency() + size * 1000 / BWidth);

		// Calculate rank score f_i in paper
		double rank_score = LT * (LT + 1) / (LT + 2) / 1.0 / size;
		//if (cache_info.isUse2()e2) {rank_score = rank_score / lrt / 1.0;}

		return rank_score;
	}
}
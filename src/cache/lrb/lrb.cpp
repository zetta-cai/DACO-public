//
// Created by zhenyus on 1/16/19.
//

#include <algorithm>
#include <chrono>

#include <webcachesim/utils.h>
//#include "utils.h"

//using namespace chrono;
//using namespace std;

#include "cache/lrb/lrb.h"
#include "common/util.h"

namespace covered
{

// MetaExtra

MetaExtra::MetaExtra(const uint32_t &distance, LRBCache* lrb_cache_ptr) {
    assert(lrb_cache_ptr != NULL);
    lrb_cache_ptr_ = lrb_cache_ptr;

    _past_distances = std::vector<uint32_t>(1, distance);
    for (uint8_t i = 0; i < lrb_cache_ptr_->n_edc_feature; ++i) {
        uint32_t _distance_idx = std::min(uint32_t(distance / lrb_cache_ptr_->edc_windows[i]), lrb_cache_ptr_->max_hash_edc_idx);
        _edc[i] = lrb_cache_ptr_->hash_edc[_distance_idx] + 1;
    }
}

// Siyuan: copy assignment constructor
MetaExtra::MetaExtra(const MetaExtra& other)
{
    assert(other.lrb_cache_ptr_ != NULL);
    lrb_cache_ptr_ = other.lrb_cache_ptr_;

    _past_distances = other._past_distances;
    for (uint8_t i = 0; i < lrb_cache_ptr_->n_edc_feature; ++i) {
        _edc[i] = other._edc[i];
    }
    _past_distance_idx = other._past_distance_idx;
}

void MetaExtra::update(const uint32_t &distance) {
    uint8_t distance_idx = _past_distance_idx % lrb_cache_ptr_->max_n_past_distances;
    if (_past_distances.size() < lrb_cache_ptr_->max_n_past_distances)
        _past_distances.emplace_back(distance);
    else
        _past_distances[distance_idx] = distance;
    assert(_past_distances.size() <= lrb_cache_ptr_->max_n_past_distances);
    _past_distance_idx = _past_distance_idx + (uint8_t) 1;
    if (_past_distance_idx >= lrb_cache_ptr_->max_n_past_distances * 2)
        _past_distance_idx -= lrb_cache_ptr_->max_n_past_distances;
    for (uint8_t i = 0; i < lrb_cache_ptr_->n_edc_feature; ++i) {
        uint32_t _distance_idx = std::min(uint32_t(distance / lrb_cache_ptr_->edc_windows[i]), lrb_cache_ptr_->max_hash_edc_idx);
        _edc[i] = _edc[i] * lrb_cache_ptr_->hash_edc[_distance_idx] + 1;
    }
}

// Siyuan: for correct cache size usage calculation
uint64_t MetaExtra::getSizeForCapacity() const
{
    uint64_t total_size = 10 * sizeof(float) + _past_distances.size() * sizeof(uint32_t) + sizeof(uint8_t); // _edc + _past_distances + _past_distance_idx
    return total_size;
}

// Meta

Meta::Meta()
{
    lrb_cache_ptr_ = NULL;
    _key = 0;
    _size = 0;
    _past_timestamp = 0;
    _extra_features = NULL;
    _extra = nullptr;
    _sample_times.clear();
    _is_keybased_meta = false;
    _keystr = "";
}

#ifdef EVICTION_LOGGING
Meta::Meta(const uint64_t &key, const uint64_t &size, const uint64_t &past_timestamp,
        const std::vector<uint16_t> &extra_features, const uint64_t &future_timestamp, LRBCache* lrb_cache_ptr, const bool& is_keybased_meta, const std::string& keystr, const std::string& valuestr) {
    assert(lrb_cache_ptr != NULL);
    lrb_cache_ptr_ = lrb_cache_ptr;

    _key = key;
    _size = size;
    _past_timestamp = past_timestamp;
    _extra_features = new uint16_t[lrb_cache_ptr_->max_n_extra_feature];
    assert(_extra_features != NULL);
    for (int i = 0; i < lrb_cache_ptr_->n_extra_fields; ++i)
        _extra_features[i] = extra_features[i];
    _future_timestamp = future_timestamp;

    _is_keybased_meta = is_keybased_meta;
    _keystr = keystr;
    _valuestr = valuestr;
}
#else
Meta::Meta(const uint64_t &key, const uint64_t &size, const uint64_t &past_timestamp,
        const std::vector<uint16_t> &extra_features, LRBCache* lrb_cache_ptr, const bool& is_keybased_meta, const std::string& keystr) {
    assert(lrb_cache_ptr != NULL);
    lrb_cache_ptr_ = lrb_cache_ptr;

    _key = key;
    _size = size;
    _past_timestamp = past_timestamp;
    _extra_features = new uint16_t[lrb_cache_ptr_->max_n_extra_feature];
    assert(_extra_features != NULL);
    for (int i = 0; i < lrb_cache_ptr_->n_extra_fields; ++i)
        _extra_features[i] = extra_features[i];
    
    _is_keybased_meta = is_keybased_meta;
    _keystr = keystr;
}
#endif

// Siyuan: copy assignment constructor
Meta::Meta(const Meta& other)
{
    *this = other;
}

const Meta& Meta::operator=(const Meta& other)
{
    lrb_cache_ptr_ = other.lrb_cache_ptr_;

    _key = other._key;
    _size = other._size;
    _past_timestamp = other._past_timestamp;
    if (lrb_cache_ptr_ != NULL)
    {
        _extra_features = new uint16_t[lrb_cache_ptr_->max_n_extra_feature];
        assert(_extra_features != NULL);
        for (int i = 0; i < lrb_cache_ptr_->n_extra_fields; ++i)
            _extra_features[i] = other._extra_features[i];
    }
    else
    {
        assert(other.lrb_cache_ptr_ == NULL);
        assert(other._extra_features == NULL);
        _extra_features = NULL;
    }
    
    if (_extra != nullptr)
    {
        delete _extra;
        _extra = nullptr;
    }
    if (other._extra != nullptr)
    {
        _extra = new MetaExtra(*other._extra);
        assert(_extra != nullptr);
    }

    _sample_times = other._sample_times;

#ifdef EVICTION_LOGGING
    _eviction_sample_times = other._eviction_sample_times;
    _future_timestamp = other._future_timestamp;
#endif

    _is_keybased_meta = other._is_keybased_meta;
    _keystr = other._keystr;

    return *this;
}

Meta::~Meta()
{
    if (_extra_features != NULL)
    {
        delete [] _extra_features;
        _extra_features = NULL;
    }

    if (_extra != nullptr)
    {
        delete _extra;
        _extra = nullptr;
    }
}

// Siyuan: for key-value caching
bool Meta::operator==(const Meta& other)
{
    assert(_is_keybased_meta == other._is_keybased_meta);
    if (_is_keybased_meta)
    {
        return _keystr == other._keystr;
    }
    else
    {
        return _key == other._key;
    }
}
bool Meta::operator==(const SimpleRequest& other)
{
    assert(_is_keybased_meta == other.is_keybased_req);
    if (_is_keybased_meta)
    {
        return _keystr == other.keystr;
    }
    else
    {
        return _key == other.id;
    }
}

void Meta::emplace_sample(uint32_t &sample_t) {
    _sample_times.emplace_back(sample_t);
}

#ifdef EVICTION_LOGGING
void Meta::emplace_eviction_sample(uint32_t &sample_t) {
    _eviction_sample_times.emplace_back(sample_t);
}
#endif

void Meta::free() {
    delete _extra;
    _extra = nullptr;
}

#ifdef EVICTION_LOGGING
void Meta::update(const uint32_t &past_timestamp, const uint32_t &future_timestamp) {
    //distance
    uint32_t _distance = past_timestamp - _past_timestamp;
    assert(_distance);
    if (!_extra) {
        _extra = new MetaExtra(_distance, lrb_cache_ptr_);
    } else
        _extra->update(_distance);
    //timestamp
    _past_timestamp = past_timestamp;
    _future_timestamp = future_timestamp;
}

#else
void Meta::update(const uint32_t &past_timestamp) {
    //distance
    uint32_t _distance = past_timestamp - _past_timestamp;
    assert(_distance);
    if (!_extra) {
        _extra = new MetaExtra(_distance, lrb_cache_ptr_);
    } else
        _extra->update(_distance);
    //timestamp
    _past_timestamp = past_timestamp;
}
#endif

int Meta::feature_overhead() {
    int ret = sizeof(Meta);
    if (_extra)
        ret += sizeof(MetaExtra) - sizeof(_sample_times) + _extra->_past_distances.capacity() * sizeof(uint32_t);
    return ret;
}

int Meta::sample_overhead() {
    return sizeof(_sample_times) + sizeof(uint32_t) * _sample_times.capacity();
}

// Siyuan: for correct cache size usage calculation
uint64_t Meta::getSizeForCapacity() const
{
    uint64_t total_size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t) * lrb_cache_ptr_->n_extra_fields + _sample_times.size() * sizeof(uint32_t) + _keystr.length(); // _size + _past_timestamp + _extra_features + _sample_times + key
    if (_extra != nullptr)
    {
        total_size += _extra->getSizeForCapacity();
    }
    return total_size;
}

// InCacheMeta

InCacheMeta::InCacheMeta() : Meta(), p_last_request()
{
    valuestr = "";
}

#ifdef EVICTION_LOGGING
InCacheMeta::InCacheMeta(const uint64_t &key,
            const uint64_t &size,
            const uint64_t &past_timestamp,
            const std::vector<uint16_t> &extra_features,
            const uint64_t &future_timestamp,
            const std::list<KeyT>::const_iterator &it,
            LRBCache* lrb_cache_ptr) :
        Meta(key, size, past_timestamp, extra_features, future_timestamp, lrb_cache_ptr) {
    p_last_request = it;
};
#else
InCacheMeta::InCacheMeta(const uint64_t &key,
            const uint64_t &size,
            const uint64_t &past_timestamp,
            const std::vector<uint16_t> &extra_features,
            //const std::list<int64_t>::const_iterator &it,
            const std::list<std::string>::const_iterator &it, // Siyuan: for key-value caching
            LRBCache* lrb_cache_ptr,
            const bool& is_keybased_meta, const std::string& keystr, const std::string& valuestr) // Siyuan: for key-value caching
        : Meta(key, size, past_timestamp, extra_features, lrb_cache_ptr, is_keybased_meta, keystr) {
    p_last_request = it;
    this->valuestr = valuestr;
};
#endif

InCacheMeta::InCacheMeta(const Meta &meta, const std::list<std::string>::const_iterator &it, const std::string& valuestr) : Meta(meta) { // Siyuan: for key-value caching
    p_last_request = it;
    this->valuestr = valuestr;
};

// Siyuan: copy assignment operator and constructor
InCacheMeta::InCacheMeta(const InCacheMeta &other) {
    *this = other;
};

const InCacheMeta& InCacheMeta::operator=(const InCacheMeta& other)
{
    Meta::operator=(other);
    p_last_request = other.p_last_request;
    valuestr = other.valuestr;
    
    return *this;
}

// Siyuan: for correct cache size usage calculation
uint64_t InCacheMeta::getSizeForCapacity() const
{
    uint64_t total_size = Meta::getSizeForCapacity();
    total_size += sizeof(std::list<std::string>::const_iterator) + valuestr.length(); // p_last_request + valuestr
    return total_size;
}

// InCacheLRUQueue

std::list<std::string>::const_iterator InCacheLRUQueue::request(std::string key) { // Siyuan: for key-value caching
    dq.emplace_front(key);
    return dq.cbegin();
}

std::list<std::string>::const_iterator InCacheLRUQueue::re_request(std::list<std::string>::const_iterator it) { // Siyuan: for key-value caching
    if (it != dq.cbegin()) {
        dq.emplace_front(*it);
        dq.erase(it);
    }
    return dq.cbegin();
}

// TrainingData

TrainingData::TrainingData(LRBCache* lrb_cache_ptr) {
    assert(lrb_cache_ptr != NULL);
    lrb_cache_ptr_ = lrb_cache_ptr;

    labels.reserve(lrb_cache_ptr_->batch_size);
    indptr.reserve(lrb_cache_ptr_->batch_size + 1);
    indptr.emplace_back(0);
    indices.reserve(lrb_cache_ptr_->batch_size * lrb_cache_ptr_->n_feature);
    data.reserve(lrb_cache_ptr_->batch_size * lrb_cache_ptr_->n_feature);
}

void TrainingData::emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval) { // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
    int32_t counter = indptr.back();

    indices.emplace_back(0);
    data.emplace_back(sample_timestamp - meta._past_timestamp);
    ++counter;

    uint32_t this_past_distance = 0;
    int j = 0;
    uint8_t n_within = 0;
    if (meta._extra) {
        for (; j < meta._extra->_past_distance_idx && j < lrb_cache_ptr_->max_n_past_distances; ++j) {
            uint8_t past_distance_idx = (meta._extra->_past_distance_idx - 1 - j) % lrb_cache_ptr_->max_n_past_distances;
            const uint32_t &past_distance = meta._extra->_past_distances[past_distance_idx];
            this_past_distance += past_distance;
            indices.emplace_back(j + 1);
            data.emplace_back(past_distance);
            if (this_past_distance < lrb_cache_ptr_->memory_window) {
                ++n_within;
            }
        }
    }

    counter += j;

    indices.emplace_back(lrb_cache_ptr_->max_n_past_timestamps);
    data.push_back(meta._size);
    ++counter;

    for (int k = 0; k < lrb_cache_ptr_->n_extra_fields; ++k) {
        indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + k + 1);
        assert(meta._extra_features != NULL); // Siyuan: poiner verification
        data.push_back(meta._extra_features[k]);
    }
    counter += lrb_cache_ptr_->n_extra_fields;

    indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 1);
    data.push_back(n_within);
    ++counter;

    if (meta._extra) {
        for (int k = 0; k < lrb_cache_ptr_->n_edc_feature; ++k) {
            indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 2 + k);
            uint32_t _distance_idx = std::min(
                    uint32_t(sample_timestamp - meta._past_timestamp) / lrb_cache_ptr_->edc_windows[k],
                    lrb_cache_ptr_->max_hash_edc_idx);
            data.push_back(meta._extra->_edc[k] * lrb_cache_ptr_->hash_edc[_distance_idx]);
        }
    } else {
        for (int k = 0; k < lrb_cache_ptr_->n_edc_feature; ++k) {
            indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 2 + k);
            uint32_t _distance_idx = std::min(
                    uint32_t(sample_timestamp - meta._past_timestamp) / lrb_cache_ptr_->edc_windows[k],
                    lrb_cache_ptr_->max_hash_edc_idx);
            data.push_back(lrb_cache_ptr_->hash_edc[_distance_idx]);
        }
    }

    counter += lrb_cache_ptr_->n_edc_feature;

    labels.push_back(log1p(future_interval));
    indptr.push_back(counter);


#ifdef EVICTION_LOGGING
    assert(lrb_cache_ptr_ != NULL);
    if ((lrb_cache_ptr_->current_seq >= lrb_cache_ptr_->n_logging_start) && !lrb_cache_ptr_->start_train_logging && (indptr.size() == 2)) {
        lrb_cache_ptr_->start_train_logging = true;
    }

    if (lrb_cache_ptr_->start_train_logging) {
//            training_and_prediction_logic_timestamps.emplace_back(lrb_cache_ptr_->current_seq / 65536);
        int i = indptr.size() - 2;
        int current_idx = indptr[i];
        for (int p = 0; p < lrb_cache_ptr_->n_feature; ++p) {
            if (p == indices[current_idx]) {
                lrb_cache_ptr_->trainings_and_predictions.emplace_back(data[current_idx]);
                if (current_idx + 1 < indptr[i + 1])
                    ++current_idx;
            } else
                lrb_cache_ptr_->trainings_and_predictions.emplace_back(NAN);
        }
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(future_interval);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(NAN);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(sample_timestamp);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(0);
        //lrb_cache_ptr_->trainings_and_predictions.emplace_back(key); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
    }
#endif
}

void TrainingData::clear() {
    labels.clear();
    indptr.resize(1);
    indices.clear();
    data.clear();
}

uint64_t TrainingData::getSizeForcapacity() const
{
    uint64_t total_size = labels.size() * sizeof(float) + indptr.size() * sizeof(int32_t) + indices.size() * sizeof(int32_t) + data.size() * sizeof(double);
    return total_size;
}

#ifdef EVICTION_LOGGING
// LRBEvictionTrainingData

LRBEvictionTrainingData::LRBEvictionTrainingData(LRBCache* lrb_cache_ptr) {
    assert(lrb_cache_ptr != NULL);
    lrb_cache_ptr_ = lrb_cache_ptr;

    labels.reserve(lrb_cache_ptr_->batch_size);
    indptr.reserve(lrb_cache_ptr_->batch_size + 1);
    indptr.emplace_back(0);
    indices.reserve(lrb_cache_ptr_->batch_size * lrb_cache_ptr_->n_feature);
    data.reserve(lrb_cache_ptr_->batch_size * lrb_cache_ptr_->n_feature);
}

void LRBEvictionTrainingData::emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval) { // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
    int32_t counter = indptr.back();

    indices.emplace_back(0);
    data.emplace_back(sample_timestamp - meta._past_timestamp);
    ++counter;

    uint32_t this_past_distance = 0;
    int j = 0;
    uint8_t n_within = 0;
    if (meta._extra) {
        for (; j < meta._extra->_past_distance_idx && j < lrb_cache_ptr_->max_n_past_distances; ++j) {
            uint8_t past_distance_idx = (meta._extra->_past_distance_idx - 1 - j) % lrb_cache_ptr_->max_n_past_distances;
            const uint32_t &past_distance = meta._extra->_past_distances[past_distance_idx];
            this_past_distance += past_distance;
            indices.emplace_back(j + 1);
            data.emplace_back(past_distance);
            if (this_past_distance < lrb_cache_ptr_->memory_window) {
                ++n_within;
            }
        }
    }

    counter += j;

    indices.emplace_back(lrb_cache_ptr_->max_n_past_timestamps);
    data.push_back(meta._size);
    ++counter;

    for (int k = 0; k < lrb_cache_ptr_->n_extra_fields; ++k) {
        indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + k + 1);
        assert(meta._extra_features != NULL); // Siyuan: poiner verification
        data.push_back(meta._extra_features[k]);
    }
    counter += lrb_cache_ptr_->n_extra_fields;

    indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 1);
    data.push_back(n_within);
    ++counter;

    if (meta._extra) {
        for (int k = 0; k < lrb_cache_ptr_->n_edc_feature; ++k) {
            indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 2 + k);
            uint32_t _distance_idx = std::min(
                    uint32_t(sample_timestamp - meta._past_timestamp) / lrb_cache_ptr_->edc_windows[k],
                    lrb_cache_ptr_->max_hash_edc_idx);
            data.push_back(meta._extra->_edc[k] * lrb_cache_ptr_->hash_edc[_distance_idx]);
        }
    } else {
        for (int k = 0; k < lrb_cache_ptr_->n_edc_feature; ++k) {
            indices.push_back(lrb_cache_ptr_->max_n_past_timestamps + lrb_cache_ptr_->n_extra_fields + 2 + k);
            uint32_t _distance_idx = std::min(
                    uint32_t(sample_timestamp - meta._past_timestamp) / lrb_cache_ptr_->edc_windows[k],
                    lrb_cache_ptr_->max_hash_edc_idx);
            data.push_back(lrb_cache_ptr_->hash_edc[_distance_idx]);
        }
    }

    counter += lrb_cache_ptr_->n_edc_feature;

    labels.push_back(log1p(future_interval));
    indptr.push_back(counter);


    if (lrb_cache_ptr_->start_train_logging) {
//            assert(lrb_cache_ptr_ != NULL);
//            training_and_prediction_logic_timestamps.emplace_back(lrb_cache_ptr_->current_seq / 65536);
        int i = indptr.size() - 2;
        int current_idx = indptr[i];
        for (int p = 0; p < lrb_cache_ptr_->n_feature; ++p) {
            if (p == indices[current_idx]) {
                lrb_cache_ptr_->trainings_and_predictions.emplace_back(data[current_idx]);
                if (current_idx + 1 < indptr[i + 1])
                    ++current_idx;
            } else
                lrb_cache_ptr_->trainings_and_predictions.emplace_back(NAN);
        }
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(future_interval);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(NAN);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(sample_timestamp);
        lrb_cache_ptr_->trainings_and_predictions.emplace_back(2);
        //lrb_cache_ptr_->trainings_and_predictions.emplace_back(key); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
    }
}

void LRBEvictionTrainingData::clear() {
    labels.clear();
    indptr.resize(1);
    indices.clear();
    data.clear();
}
#endif

// LRBCache

// Siyuan: constructor
LRBCache::LRBCache(const uint32_t& dataset_keycnt) : Cache(), n_edc_feature(10), max_n_extra_feature(4)
{
    current_seq = -1;
    max_n_past_timestamps = 32;
    max_n_past_distances = 31;
    base_edc_window = 10;
    edc_windows.clear();
    hash_edc.clear();
    max_hash_edc_idx = 0;
    memory_window = 67108864;
    n_extra_fields = 0;
    //batch_size = 131072;
    batch_size = 10 * dataset_keycnt; // Siyuan: set batch size as 10*dataset_keycnt (the same as warmup reqcnt) to avoid too frequent training for warmup speedup
    n_feature = 0;
#ifdef EVICTION_LOGGING
    future_timestamps.clear();
    n_logging_start = 0;
    trainings_and_predictions.clear();
    start_train_logging = false;
    range_log = 1000000;
#endif

    training_data = NULL;
#ifdef EVICTION_LOGGING
    eviction_training_data = NULL;
#endif
    booster = nullptr;
}

// Siyuan: free heap memory
LRBCache::~LRBCache()
{
    if (training_data != NULL)
    {
        delete training_data;
        training_data = NULL;
    }

#ifdef EVICTION_LOGGING
    if (eviction_training_data != NULL)
    {
        delete eviction_training_data;
        eviction_training_data = NULL;
    }
#endif

    if (booster) LGBM_BoosterFree(booster);
}

void LRBCache::init_with_params(const std::map<std::string, std::string> &params) {
    //set params
    for (auto &it: params) {
        if (it.first == "sample_rate") {
            sample_rate = stoul(it.second);
        } else if (it.first == "memory_window") {
            memory_window = stoull(it.second);
        } else if (it.first == "max_n_past_timestamps") {
            max_n_past_timestamps = (uint8_t) stoi(it.second);
        } else if (it.first == "batch_size") {
            batch_size = stoull(it.second);
        } else if (it.first == "n_extra_fields") {
            n_extra_fields = stoull(it.second);
        } else if (it.first == "num_iterations") {
            training_params["num_iterations"] = it.second;
        } else if (it.first == "learning_rate") {
            training_params["learning_rate"] = it.second;
        } else if (it.first == "num_threads") {
            training_params["num_threads"] = it.second;
        } else if (it.first == "num_leaves") {
            training_params["num_leaves"] = it.second;
        } else if (it.first == "byte_million_req") {
            byte_million_req = stoull(it.second);
#ifdef EVICTION_LOGGING
            } else if (it.first == "n_early_stop") {
                n_early_stop = stoll((it.second));
            } else if (it.first == "n_req") {
                n_req = stoull(it.second);
            } else if (it.first == "dburi") {
                dburi = it.second;
            } else if (it.first == "task_id") {
                task_id = it.second;
            } else if (it.first == "belady_boundary") {
                belady_boundary = stoll(it.second);
            } else if (it.first == "range_log") {
                range_log = stoi(it.second);
#endif
        } else if (it.first == "n_edc_feature") {
            if (stoull(it.second) != n_edc_feature) {
                std::cerr << "error: cannot change n_edc_feature because of const" << std::endl;
                abort();
            }
//                n_edc_feature = stoull(it.second);
        } else if (it.first == "objective") {
            if (it.second == "byte_miss_ratio")
                objective = byte_miss_ratio;
            else if (it.second == "object_miss_ratio")
                objective = object_miss_ratio;
            else {
                std::cerr << "error: unknown objective" << std::endl;
                exit(-1);
            }
        } else {
            std::cerr << "LRB unrecognized parameter: " << it.first << std::endl;
        }
    }

    // Siyuan: negative_candidate_queue will map forget timestamp to key
    //negative_candidate_queue = make_shared<sparse_hash_map<uint64_t, uint64_t>>(memory_window);
    negative_candidate_queue = std::make_shared<sparse_hash_map<uint64_t, std::string>>(memory_window); // Siyuan: for key-value caching
    max_n_past_distances = max_n_past_timestamps - 1;
    //init
    edc_windows = std::vector<uint32_t>(n_edc_feature);
    for (uint8_t i = 0; i < n_edc_feature; ++i) {
        edc_windows[i] = pow(2, base_edc_window + i);
    }
    set_hash_edc();

    //interval, distances, size, extra_features, n_past_intervals, edwt
    n_feature = max_n_past_timestamps + n_extra_fields + 2 + n_edc_feature;
    if (n_extra_fields) {
        if (n_extra_fields > max_n_extra_feature) {
            std::cerr << "error: only support <= " + std::to_string(max_n_extra_feature)
                    + " extra fields because of static allocation" << std::endl;
            abort();
        }
        std::string categorical_feature = std::to_string(max_n_past_timestamps + 1);
        for (uint i = 0; i < n_extra_fields - 1; ++i) {
            categorical_feature += "," + std::to_string(max_n_past_timestamps + 2 + i);
        }
        training_params["categorical_feature"] = categorical_feature;
    }
    inference_params = training_params;
    //can set number of threads, however the inference time will increase a lot (2x~3x) if use 1 thread
//        inference_params["num_threads"] = "4";
    training_data = new TrainingData(this);
    assert(training_data != NULL); // Siyuan: allocate heap memory for TrainingData
#ifdef EVICTION_LOGGING
    eviction_training_data = new LRBEvictionTrainingData(this);
    assert(eviction_training_data != NULL); // Siyuan: allocate heap memory for TrainingData
#endif

#ifdef EVICTION_LOGGING
    //logging the training and inference happened in the last 1 million
    if (n_early_stop < 0) {
        n_logging_start = n_req < range_log ? 0 : n_req - range_log;
    } else {
        n_logging_start = n_early_stop < range_log ? 0 : n_early_stop - range_log;
    }
#endif
}
    
void LRBCache::train() {
    ++n_retrain;
    auto timeBegin = std::chrono::system_clock::now();
    if (booster) LGBM_BoosterFree(booster);
    // create training dataset
    DatasetHandle trainData;
    LGBM_DatasetCreateFromCSR(
            static_cast<void *>(training_data->indptr.data()),
            C_API_DTYPE_INT32,
            training_data->indices.data(),
            static_cast<void *>(training_data->data.data()),
            C_API_DTYPE_FLOAT64,
            training_data->indptr.size(),
            training_data->data.size(),
            n_feature,  //remove future t
            training_params,
            nullptr,
            &trainData);

    LGBM_DatasetSetField(trainData,
                        "label",
                        static_cast<void *>(training_data->labels.data()),
                        training_data->labels.size(),
                        C_API_DTYPE_FLOAT32);

    // init booster
    LGBM_BoosterCreate(trainData, training_params, &booster);
    // train
    for (int i = 0; i < stoi(training_params["num_iterations"]); i++) {
        int isFinished;
        LGBM_BoosterUpdateOneIter(booster, &isFinished);
        if (isFinished) {
            break;
        }
    }

    int64_t len;
    std::vector<double> result(training_data->indptr.size() - 1);
    LGBM_BoosterPredictForCSR(booster,
                            static_cast<void *>(training_data->indptr.data()),
                            C_API_DTYPE_INT32,
                            training_data->indices.data(),
                            static_cast<void *>(training_data->data.data()),
                            C_API_DTYPE_FLOAT64,
                            training_data->indptr.size(),
                            training_data->data.size(),
                            n_feature,  //remove future t
                            C_API_PREDICT_NORMAL,
                            0,
                            training_params,
                            &len,
                            result.data());


    double se = 0;
    for (int i = 0; i < result.size(); ++i) {
        auto diff = result[i] - training_data->labels[i];
        se += diff * diff;
    }
    training_loss = training_loss * 0.99 + se / batch_size * 0.01;

    LGBM_DatasetFree(trainData);
    training_time = 0.95 * training_time +
                    0.05 * std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timeBegin).count();
}

void LRBCache::sample() {
    // start sampling once cache filled up
    auto rand_idx = _distribution(_generator);
    auto n_in = static_cast<uint32_t>(in_cache_metas.size());
    auto n_out = static_cast<uint32_t>(out_cache_metas.size());
    std::bernoulli_distribution distribution_from_in(static_cast<double>(n_in) / (n_in + n_out));
    auto is_from_in = distribution_from_in(_generator);
    if (is_from_in == true) {
        uint32_t pos = rand_idx % n_in;
        auto &meta = in_cache_metas[pos];
        uint64_t original_in_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for correct cache size calculation
        meta.emplace_sample(current_seq); // Siyuan: this will insert a sample time
        // Siyuan: for correct cache size calculation
        uint64_t current_in_cache_meta_size = meta.getSizeForCapacity();
        assert(current_in_cache_meta_size > original_in_cache_meta_size);
        _currentSize += (current_in_cache_meta_size - original_in_cache_meta_size);
    } else {
        uint32_t pos = rand_idx % n_out;
        auto &meta = out_cache_metas[pos];
        uint64_t original_in_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for correct cache size calculation
        meta.emplace_sample(current_seq);
        // Siyuan: for correct cache size calculation
        uint64_t current_in_cache_meta_size = meta.getSizeForCapacity();
        assert(current_in_cache_meta_size > original_in_cache_meta_size);
        _currentSize += (current_in_cache_meta_size - original_in_cache_meta_size);
    }
}


void LRBCache::update_stat_periodic() {
//    uint64_t feature_overhead = 0;
//    uint64_t sample_overhead = 0;
//    for (auto &m: in_cache_metas) {
//        feature_overhead += m.feature_overhead();
//        sample_overhead += m.sample_overhead();
//    }
//    for (auto &m: out_cache_metas) {
//        feature_overhead += m.feature_overhead();
//        sample_overhead += m.sample_overhead();
//    }
    float percent_beyond;
    if (0 == obj_distribution[0] && 0 == obj_distribution[1]) {
        percent_beyond = 0;
    } else {
        percent_beyond = static_cast<float>(obj_distribution[1])/(obj_distribution[0] + obj_distribution[1]);
    }
    obj_distribution[0] = obj_distribution[1] = 0;
    segment_percent_beyond.emplace_back(percent_beyond);
    segment_n_retrain.emplace_back(n_retrain);
    segment_n_in.emplace_back(in_cache_metas.size());
    segment_n_out.emplace_back(out_cache_metas.size());

    float positive_example_ratio;
    if (0 == training_data_distribution[0] && 0 == training_data_distribution[1]) {
        positive_example_ratio = 0;
    } else {
        positive_example_ratio = static_cast<float>(training_data_distribution[1])/(training_data_distribution[0] + training_data_distribution[1]);
    }
    training_data_distribution[0] = training_data_distribution[1] = 0;
    segment_positive_example_ratio.emplace_back(positive_example_ratio);

    n_retrain = 0;
    std::cerr
            << "in/out metadata: " << in_cache_metas.size() << " / " << out_cache_metas.size() << std::endl
            //    cerr << "feature overhead: "<<feature_overhead<<std::endl;
            << "memory_window: " << memory_window << std::endl
//            << "percent_beyond: " << percent_beyond << std::endl
//            << "feature overhead per entry: " << static_cast<double>(feature_overhead) / key_map.size() << std::endl
//            //    cerr << "sample overhead: "<<sample_overhead<<std::endl;
//            << "sample overhead per entry: " << static_cast<double>(sample_overhead) / key_map.size() << std::endl
            << "n_training: " << training_data->labels.size() << std::endl
            //            << "training loss: " << training_loss << std::endl
            << "training_time: " << training_time << " ms" << std::endl
            << "inference_time: " << inference_time << " us" << std::endl;
    assert(in_cache_metas.size() + out_cache_metas.size() == key_map.size());
}

// Siyuan: remove "static" from set_hash_edc for thread-scalability
void LRBCache::set_hash_edc() {
    max_hash_edc_idx = (uint64_t) (memory_window / pow(2, base_edc_window)) - 1;
    hash_edc = std::vector<double>(max_hash_edc_idx + 1);
    for (int i = 0; i < hash_edc.size(); ++i)
        hash_edc[i] = pow(0.5, i);
}

uint32_t LRBCache::getInCacheMetadataCnt() const
{
    return in_cache_metas.size();
}

bool LRBCache::access(SimpleRequest &req, const bool& is_update) {
    bool ret;
    ++current_seq;

#ifdef EVICTION_LOGGING
    {
        AnnotatedRequest *_req = (AnnotatedRequest *) &req;
        auto it = future_timestamps.find(_req->_id);
        if (it == future_timestamps.end()) {
            future_timestamps.insert({_req->_id, _req->_next_seq});
        } else {
            it->second = _req->_next_seq;
        }
    }
#endif
    forget();

    //first update the metadata: insert/update, which can trigger pending data.mature
    //auto it = key_map.find(req.id);
    assert(req.is_keybased_req); // Siyuan: for key-value caching
    auto it = key_map.find(req.keystr); // Siyuan: for key-value caching
    if (it != key_map.end()) {
        auto list_idx = it->second.list_idx;
        auto list_pos = it->second.list_pos;
        Meta &meta = list_idx ? out_cache_metas[list_pos] : in_cache_metas[list_pos];
        //update past timestamps
        assert(meta == req); // Siyuan: for key-value caching
        uint64_t original_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for correct cache size usage calculation
        //assert(meta._key == req.id);
        uint64_t last_timestamp = meta._past_timestamp;
        uint64_t forget_timestamp = last_timestamp % memory_window;
        //if the key in out_metadata, it must also in forget table
        // Siyuan: the following assertion made by LRB original code may fail in some rare cases, but NOT affect correctness and cache performance
        //assert((!list_idx) ||
        //    (negative_candidate_queue->find(forget_timestamp) !=
        //        negative_candidate_queue->end()));
        if (list_idx == 1 && negative_candidate_queue->find(forget_timestamp) == negative_candidate_queue->end())
        {
            std::ostringstream oss;
            oss << "key " << req.keystr << " is not found in negative_candidate_queue (last_timestamp: " << last_timestamp << "; forget_timestamp: " << forget_timestamp << "; " << "current_seq: " << current_seq << ")";
            Util::dumpInfoMsg("LRBCache", oss.str());
        }
        //re-request
        if (!meta._sample_times.empty()) {
            uint64_t original_training_data_size = training_data->getSizeForcapacity(); // Siyuan: for correct cache size usage calculation
            //mature
            for (auto &sample_time: meta._sample_times) {
                //don't use label within the first forget window because the data is not static
                uint32_t future_distance = current_seq - sample_time;
                //training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                ++training_data_distribution[1];
            }
            //batch_size ~>= batch_size
            if (training_data->labels.size() >= batch_size) {
                train();
                training_data->clear();
            }
            meta._sample_times.clear(); // Siyuan: this will clear sample times decrease cache size usage
            meta._sample_times.shrink_to_fit();
            // Siyuan: for correct cache size usage calculation
            uint64_t current_training_data_size = training_data->getSizeForcapacity();
            if (current_training_data_size > original_training_data_size)
            {
                _currentSize += (current_training_data_size - original_training_data_size);
            }
            else
            {
                decreaseCurrentSize(original_training_data_size - current_training_data_size);
            }
        }

#ifdef EVICTION_LOGGING
        if (!meta._eviction_sample_times.empty()) {
            //mature
            for (auto &sample_time: meta._eviction_sample_times) {
                //don't use label within the first forget window because the data is not static
                uint32_t future_distance = req.seq - sample_time;
                //eviction_training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                eviction_training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                //training
                if (eviction_training_data->labels.size() == batch_size) {
                    eviction_training_data->clear();
                }
            }
            meta._eviction_sample_times.clear(); // Siyuan: this will clear sample times decrease cache size usage
            meta._eviction_sample_times.shrink_to_fit();
        }
#endif

        //make this update after update training, otherwise the last timestamp will change
#ifdef EVICTION_LOGGING
        AnnotatedRequest *_req = (AnnotatedRequest *) &req;
        meta.update(current_seq, _req->_next_seq); // Siyuan: this will create MetaExtra or insert new extra feature if features not full, which may increase cache size usage
#else
        meta.update(current_seq); // Siyuan: this will create MetaExtra or insert new extra feature if features not full, which may increase cache size usage
#endif
        if (list_idx) {
            negative_candidate_queue->erase(forget_timestamp);
            //negative_candidate_queue->insert({current_seq % memory_window, req.id});
            assert(req.is_keybased_req); // Siyuan: for key-value caching
            negative_candidate_queue->insert({current_seq % memory_window, req.keystr}); // Siyuan: for key-value caching
            assert(negative_candidate_queue->find(current_seq % memory_window) !=
                negative_candidate_queue->end());
        } else {
            auto *p = dynamic_cast<InCacheMeta *>(&meta);
            p->p_last_request = in_cache_lru_queue.re_request(p->p_last_request);

            if (!is_update)
            {
                req.valuestr = p->valuestr; // Siyuan: get value if key is cached
            }
            else
            {
                p->valuestr = req.valuestr; // Siyuan: update value if key is cached
            }
        }
        //update negative_candidate_queue
        ret = !list_idx;

        // Siyuan: for correct cache size usage calculation
        uint64_t current_cache_meta_size = meta.getSizeForCapacity();
        if (current_cache_meta_size > original_cache_meta_size)
        {
            _currentSize += (current_cache_meta_size - original_cache_meta_size);
        }
        else
        {
            decreaseCurrentSize(original_cache_meta_size - current_cache_meta_size);
        }
    } else {
        ret = false;
    }

    //sampling happens late to prevent immediate re-request
    if (is_sampling) {
        sample();
    }

    return ret;
}

void LRBCache::forget() {
    /*
    * forget happens exactly after the beginning of each time, without doing any other operations. For example, an
    * object is request at time 0 with memory window = 5, and will be forgotten exactly at the start of time 5.
    * */
    //remove item from forget table, which is not going to be affect from update
    auto it = negative_candidate_queue->find(current_seq % memory_window);
    if (it != negative_candidate_queue->end()) {
        auto forget_key = it->second;
        auto pos = key_map.find(forget_key)->second.list_pos;
        // Forget only happens at list 1
        assert(key_map.find(forget_key)->second.list_idx);
//        auto pos = meta_it->second.list_pos;
//        bool meta_id = meta_it->second.list_idx;
        auto &meta = out_cache_metas[pos];

        //timeout mature
        if (!meta._sample_times.empty()) {
            uint64_t original_training_data_size = training_data->getSizeForcapacity(); // Siyuan: for correct cache size usage calculation
            //mature
            //todo: potential to overfill
            uint32_t future_distance = memory_window * 2;
            for (auto &sample_time: meta._sample_times) {
                //don't use label within the first forget window because the data is not static
                //training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                ++training_data_distribution[0];
            }
            //batch_size ~>= batch_size
            if (training_data->labels.size() >= batch_size) {
                train();
                training_data->clear();
            }
            uint64_t original_out_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for correct cache size calculation
            meta._sample_times.clear(); // Siyuan: this will clear sample times to reduce cache size usage of out-cache object-level meta
            meta._sample_times.shrink_to_fit();
            // Siyuan: for correct cache size calculation
            uint64_t current_training_data_size = training_data->getSizeForcapacity();
            if (current_training_data_size > original_training_data_size)
            {
                _currentSize += (current_training_data_size - original_training_data_size);
            }
            else
            {
                decreaseCurrentSize(original_training_data_size - current_training_data_size);
            }
            uint64_t current_out_cache_meta_size = meta.getSizeForCapacity();
            assert(original_out_cache_meta_size > current_out_cache_meta_size); // NOT: meta._sample_times is NOT empty
            decreaseCurrentSize(original_out_cache_meta_size - current_out_cache_meta_size);
        }

#ifdef EVICTION_LOGGING
        //timeout mature
        if (!meta._eviction_sample_times.empty()) {
            //mature
            //todo: potential to overfill
            uint32_t future_distance = memory_window * 2;
            for (auto &sample_time: meta._eviction_sample_times) {
                //don't use label within the first forget window because the data is not static
                //eviction_training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                eviction_training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                //training
                if (eviction_training_data->labels.size() == batch_size) {
                    eviction_training_data->clear();
                }
            }
            meta._eviction_sample_times.clear();
            meta._eviction_sample_times.shrink_to_fit();
        }
#endif

        //assert(meta._key == forget_key);
        assert(meta._keystr == forget_key); // Siyuan: for key-value caching
        remove_from_outcache_metas(meta, pos, forget_key);
    }
}

void LRBCache::admit(const SimpleRequest &req) {
    const uint64_t &size = req.size;
    // object feasible to store?
    if (size > _cacheSize) {
        //LOG("L", _cacheSize, req.id, size); // Siyuan: should log req.keystr
        return;
    }

    //auto it = key_map.find(req.id);
    assert(req.is_keybased_req); // Siyuan: for key-value caching
    auto it = key_map.find(req.keystr); // Siyuan: for key-value caching
    if (it == key_map.end()) {
        //fresh insert
        //key_map.insert({req.id, {0, (uint32_t) in_cache_metas.size()}});
        key_map.insert({req.keystr, {0, (uint32_t) in_cache_metas.size()}}); // Siyuan: for key-value caching
        //auto lru_it = in_cache_lru_queue.request(req.id);
        auto lru_it = in_cache_lru_queue.request(req.keystr); // Siyuan: for key-value caching
#ifdef EVICTION_LOGGING
        AnnotatedRequest *_req = (AnnotatedRequest *) &req;
        //in_cache_metas.emplace_back(req.id, req.size, current_seq, req.extra_features, _req->_next_seq, lru_it);
        in_cache_metas.emplace_back(req.id, req.size, current_seq, req.extra_features, _req->_next_seq, lru_it, this, req.is_keybased_req, req.keystr, req.valuestr); // Siyuan: for key-value caching
#else
        //in_cache_metas.emplace_back(req.id, req.size, current_seq, req.extra_features, lru_it);
        // Siyuan: for key-value caching
        InCacheMeta tmp_in_cache_meta(req.id, req.size, current_seq, req.extra_features, lru_it, this, req.is_keybased_req, req.keystr, req.valuestr);
        in_cache_metas.push_back(tmp_in_cache_meta);
#endif
        _currentSize += (req.keystr.length() + sizeof(KeyMapEntryT)); // Siyuan: key and list index/pos in lookup table
        _currentSize += (req.keystr.length()); // Siyuan: key in LRU list
        //_currentSize += size;
        _currentSize += tmp_in_cache_meta.getSizeForCapacity(); // Siyuan: in-cache object-level metadata w/ key-value size
        //this must be a fresh insert
////        negative_candidate_queue.insert({(current_seq + memory_window)%memory_window, req.id});
//          assert(req.is_keybased_req); // Siyuan: for key-value caching
//          negative_candidate_queue.insert({(current_seq + memory_window)%memory_window, req.keystr}); // Siyuan: for key-value caching
        if (_currentSize <= _cacheSize)
            return;
    } else {
        assert(it->second.list_idx == 1); // Siyuan: must be in out_cache

        //bring list 1 to list 0
        //first move meta data, then modify hash table
        uint32_t tail0_pos = in_cache_metas.size();
        auto &meta = out_cache_metas[it->second.list_pos];
        uint64_t original_out_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for key-value caching
        assert(meta == req); // Siyuan: for key-value caching
        auto forget_timestamp = meta._past_timestamp % memory_window;
        negative_candidate_queue->erase(forget_timestamp);
        //auto it_lru = in_cache_lru_queue.request(req.id);
        auto it_lru = in_cache_lru_queue.request(req.keystr); // Siyuan: for key-value caching
        //in_cache_metas.emplace_back(out_cache_metas[it->second.list_pos], it_lru);
        // Siyuan: for key-value caching
        InCacheMeta tmp_in_cache_meta(out_cache_metas[it->second.list_pos], it_lru, req.valuestr);
        in_cache_metas.push_back(tmp_in_cache_meta);
        uint32_t tail1_pos = out_cache_metas.size() - 1;
        if (it->second.list_pos != tail1_pos) {
            //swap tail
            out_cache_metas[it->second.list_pos] = out_cache_metas[tail1_pos];
            //key_map.find(out_cache_metas[tail1_pos]._key)->second.list_pos = it->second.list_pos;
            assert(out_cache_metas[tail1_pos]._is_keybased_meta); // Siyuan: for key-value caching
            key_map.find(out_cache_metas[tail1_pos]._keystr)->second.list_pos = it->second.list_pos; // Siyuan: for key-value caching
        }
        out_cache_metas.pop_back();
        it->second = {0, tail0_pos};
        //_currentSize += size;
        // Siyuan: increased cache size usage includes delta from out-cache to in-cache object-level metadata + key in LRU list
        assert(tmp_in_cache_meta.getSizeForCapacity() >= original_out_cache_meta_size);
        _currentSize += (tmp_in_cache_meta.getSizeForCapacity() - original_out_cache_meta_size + req.keystr.length());
        decreaseCurrentSize(sizeof(uint64_t) + req.keystr.length()); // Siyuan: forget timestamp and key in negative_candidate_queue
    }
    if (_currentSize > _cacheSize) {
        //start sampling once cache is filled up
        is_sampling = true;
    }

    // (Siyuan) NOTE: eviction will be triggered by EdgeWrapper outside LRB local cache
    // // check more eviction needed?
    // while (_currentSize > _cacheSize) {
    //     evict();
    // }
}


//std::pair<uint64_t, uint32_t> LRBCache::rank() {
std::pair<std::string, uint32_t> LRBCache::rank() { // Siyuan: for key-value caching
    {
        //if not trained yet, or in_cache_lru past memory window, use LRU
        auto &candidate_key = in_cache_lru_queue.dq.back();
        auto it = key_map.find(candidate_key);
        auto pos = it->second.list_pos;
        auto &meta = in_cache_metas[pos];
        if ((!booster) || (memory_window <= current_seq - meta._past_timestamp)) {
            //this use LRU force eviction, consider sampled a beyond boundary object
            if (booster) {
                ++obj_distribution[1];
            }
            //return {meta._key, pos};
            return {meta._keystr, pos}; // Siyuan: for key-value caching
        }
    }


    int32_t indptr[sample_rate + 1];
    indptr[0] = 0;
    int32_t indices[sample_rate * n_feature];
    double data[sample_rate * n_feature];
    int32_t past_timestamps[sample_rate];
    uint32_t sizes[sample_rate];

    //unordered_set<uint64_t> key_set;
    //uint64_t keys[sample_rate];
    std::unordered_set<std::string> key_set; // Siyuan: for key-value caching
    std::string keys[sample_rate]; // Siyuan: for key-value caching
    uint32_t poses[sample_rate];
    //next_past_timestamp, next_size = next_indptr - 1

    unsigned int idx_feature = 0;
    unsigned int idx_row = 0;

    auto n_new_sample = sample_rate - idx_row;
    while (idx_row != sample_rate) {
        uint32_t pos = _distribution(_generator) % in_cache_metas.size();
        auto &meta = in_cache_metas[pos];
        assert(meta._is_keybased_meta); // Siyuan: for key-value caching
        //if (key_set.find(meta._key) != key_set.end()) {
        if (key_set.find(meta._keystr) != key_set.end()) { // Siyuan: for key-value caching
            continue;
        } else {
            // key_set.insert(meta._key);
            key_set.insert(meta._keystr); // Siyuan: for key-value caching
        }
#ifdef EVICTION_LOGGING
        meta.emplace_eviction_sample(current_seq);
#endif

        //keys[idx_row] = meta._key;
        keys[idx_row] = meta._keystr; // Siyuan: for key-value caching
        poses[idx_row] = pos;
        //fill in past_interval
        indices[idx_feature] = 0;
        data[idx_feature++] = current_seq - meta._past_timestamp;
        past_timestamps[idx_row] = meta._past_timestamp;

        uint8_t j = 0;
        uint32_t this_past_distance = 0;
        uint8_t n_within = 0;
        if (meta._extra) {
            for (j = 0; j < meta._extra->_past_distance_idx && j < max_n_past_distances; ++j) {
                uint8_t past_distance_idx = (meta._extra->_past_distance_idx - 1 - j) % max_n_past_distances;
                uint32_t &past_distance = meta._extra->_past_distances[past_distance_idx];
                this_past_distance += past_distance;
                indices[idx_feature] = j + 1;
                data[idx_feature++] = past_distance;
                if (this_past_distance < memory_window) {
                    ++n_within;
                }
//                } else
//                    break;
            }
        }

        indices[idx_feature] = max_n_past_timestamps;
        data[idx_feature++] = meta._size;
        sizes[idx_row] = meta._size;

        for (uint k = 0; k < n_extra_fields; ++k) {
            indices[idx_feature] = max_n_past_timestamps + k + 1;
            data[idx_feature++] = meta._extra_features[k];
        }

        indices[idx_feature] = max_n_past_timestamps + n_extra_fields + 1;
        data[idx_feature++] = n_within;

        for (uint8_t k = 0; k < n_edc_feature; ++k) {
            indices[idx_feature] = max_n_past_timestamps + n_extra_fields + 2 + k;
            uint32_t _distance_idx = std::min(uint32_t(current_seq - meta._past_timestamp) / edc_windows[k],
                                        max_hash_edc_idx);
            if (meta._extra)
                data[idx_feature++] = meta._extra->_edc[k] * hash_edc[_distance_idx];
            else
                data[idx_feature++] = hash_edc[_distance_idx];
        }
        //remove future t
        indptr[++idx_row] = idx_feature;
    }

    int64_t len;
    double scores[sample_rate];
    std::chrono::system_clock::time_point timeBegin;
    //sample to measure inference time
    if (!(current_seq % 10000))
        timeBegin = std::chrono::system_clock::now();
    LGBM_BoosterPredictForCSR(booster,
                            static_cast<void *>(indptr),
                            C_API_DTYPE_INT32,
                            indices,
                            static_cast<void *>(data),
                            C_API_DTYPE_FLOAT64,
                            idx_row + 1,
                            idx_feature,
                            n_feature,  //remove future t
                            C_API_PREDICT_NORMAL,
                            0,
                            inference_params,
                            &len,
                            scores);
    if (!(current_seq % 10000))
        inference_time = 0.95 * inference_time +
                        0.05 *
                        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timeBegin).count();
//    for (int i = 0; i < n_sample; ++i)
//        result[i] -= (t - past_timestamps[i]);
    for (int i = sample_rate - n_new_sample; i < sample_rate; ++i) {
        //only monitor at the end of change interval
        if (scores[i] >= log1p(memory_window)) {
            ++obj_distribution[1];
        } else {
            ++obj_distribution[0];
        }
    }

    if (objective == object_miss_ratio) {
        for (uint32_t i = 0; i < sample_rate; ++i)
            scores[i] *= sizes[i];
    }

    std::vector<int> index(sample_rate, 0);
    for (int i = 0; i < index.size(); ++i) {
        index[i] = i;
    }

    std::sort(index.begin(), index.end(),
        [&](const int &a, const int &b) {
            return (scores[a] > scores[b]);
        }
    );

#ifdef EVICTION_LOGGING
    {
        if (start_train_logging) {
//            training_and_prediction_logic_timestamps.emplace_back(current_seq / 65536);
            for (int i = 0; i < sample_rate; ++i) {
                int current_idx = indptr[i];
                for (int p = 0; p < n_feature; ++p) {
                    if (p == indices[current_idx]) {
                        trainings_and_predictions.emplace_back(data[current_idx]);
                        if (current_idx + 1 < indptr[i + 1])
                            ++current_idx;
                    } else
                        trainings_and_predictions.emplace_back(NAN);
                }
                uint32_t future_interval = future_timestamps.find(keys[i])->second - current_seq;
                future_interval = min(2 * memory_window, future_interval);
                trainings_and_predictions.emplace_back(future_interval);
                trainings_and_predictions.emplace_back(result[i]);
                trainings_and_predictions.emplace_back(current_seq);
                trainings_and_predictions.emplace_back(1);
                //trainings_and_predictions.emplace_back(keys[i]); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
            }
        }
    }
#endif

    return {keys[index[0]], poses[index[0]]};
}

// Siyuan: get victim key before eviction
std::string LRBCache::getVictimKey()
{
    auto epair = rank();
    //uint64_t &key = epair.first;
    std::string &key = epair.first; // Siyuan: for key-value caching

    return key;
}

bool LRBCache::evict(const std::string& keystr, std::string& valuestr) {
    // (Siyuan) NOTE: victim will be found by getVictimKey() before evict()
    // auto epair = rank();
    // //uint64_t &key = epair.first;
    // std::string &keystr = epair.first; // Siyuan: for key-value caching
    // uint32_t &old_pos = epair.second;

    // Siyuan: evict for the given victim
    sparse_hash_map<std::string, KeyMapEntryT>::const_iterator key_map_const_iter = key_map.find(keystr);
    if (key_map_const_iter == key_map.end()) // No such key
    {
        std::ostringstream oss;
        oss << "evict() key " << keystr << " not found in key_map!";
        Util::dumpWarnMsg("LRBCache", oss.str());
        return false;
    }
    else if (key_map_const_iter->second.list_idx == 1) // Key in out-cache
    {
        std::ostringstream oss;
        oss << "evict() key " << keystr << " not found in in_cache_metas!";
        Util::dumpWarnMsg("LRBCache", oss.str());
        return false;
    }
    const uint32_t &old_pos = key_map_const_iter->second.list_pos;

#ifdef EVICTION_LOGGING
    {
        auto it = future_timestamps.find(key);
        unsigned int decision_qulity =
                static_cast<double>(it->second - current_seq) / (_cacheSize * 1e6 / byte_million_req);
        decision_qulity = min((unsigned int) 255, decision_qulity);
        eviction_qualities.emplace_back(decision_qulity);
        eviction_logic_timestamps.emplace_back(current_seq / 65536);
    }
#endif

    auto &meta = in_cache_metas[old_pos];
    uint64_t victim_in_cache_meta_size = meta.getSizeForCapacity();
    valuestr = static_cast<InCacheMeta>(meta).valuestr; // Siyuan: get victim value
    if (memory_window <= current_seq - meta._past_timestamp) {
        //must be the tail of lru
        if (!meta._sample_times.empty()) {
            uint64_t original_training_data_size = training_data->getSizeForcapacity(); // Siyuan: for correct cache size usage calculation
            //mature
            uint32_t future_distance = current_seq - meta._past_timestamp + memory_window;
            for (auto &sample_time: meta._sample_times) {
                //don't use label within the first forget window because the data is not static
                //training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                ++training_data_distribution[0];
            }
            //batch_size ~>= batch_size
            if (training_data->labels.size() >= batch_size) {
                train();
                training_data->clear();
            }
            meta._sample_times.clear(); // Siyuan: victim_in_cache_meta_size already contains cache size usage of sample times
            meta._sample_times.shrink_to_fit();
            // Siyuan: for correct cache size usage calculation
            uint64_t current_training_data_size = training_data->getSizeForcapacity();
            if (current_training_data_size > original_training_data_size)
            {
                _currentSize += (current_training_data_size - original_training_data_size);
            }
            else
            {
                decreaseCurrentSize(original_training_data_size - current_training_data_size);
            }
        }

#ifdef EVICTION_LOGGING
        //must be the tail of lru
        if (!meta._eviction_sample_times.empty()) {
            //mature
            uint32_t future_distance = current_seq - meta._past_timestamp + memory_window;
            for (auto &sample_time: meta._eviction_sample_times) {
                //don't use label within the first forget window because the data is not static
                //eviction_training_data->emplace_back(meta, sample_time, future_distance, meta._key);
                eviction_training_data->emplace_back(meta, sample_time, future_distance); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference
                //training
                if (eviction_training_data->labels.size() == batch_size) {
                    eviction_training_data->clear();
                }
            }
            meta._eviction_sample_times.clear(); // Siyuan: victim_in_cache_meta_size already contains cache size usage of sample times
            meta._eviction_sample_times.shrink_to_fit();
        }
#endif

        in_cache_lru_queue.dq.erase(meta.p_last_request);
        meta.p_last_request = in_cache_lru_queue.dq.end();
        //above is suppose to be below, but to make sure the action is correct
//        in_cache_lru_queue.dq.pop_back();
        meta.free(); // Siyuan: victim_in_cache_meta_size already contains cache size usage of MetaExtra
        key_map.erase(keystr);

        uint32_t activate_tail_idx = in_cache_metas.size() - 1;
        if (old_pos != activate_tail_idx) {
            //move tail
            in_cache_metas[old_pos] = in_cache_metas[activate_tail_idx];
            //key_map.find(in_cache_metas[activate_tail_idx]._key)->second.list_pos = old_pos;
            assert(in_cache_metas[activate_tail_idx]._is_keybased_meta); // Siyuan: for key-value caching
            key_map.find(in_cache_metas[activate_tail_idx]._keystr)->second.list_pos = old_pos; // Siyuan: for key-value caching
        }
        in_cache_metas.pop_back();
        ++n_force_eviction;

        //_currentSize -= meta._size;
        decreaseCurrentSize(victim_in_cache_meta_size + keystr.length() + keystr.length() + sizeof(KeyMapEntryT)); // Siyuan: victim in-cache object-level metadata + key in LRU list + key and list index/pos in lookup table
    } else {
        //bring list 0 to list 1
        in_cache_lru_queue.dq.erase(meta.p_last_request);
        meta.p_last_request = in_cache_lru_queue.dq.end();
        //negative_candidate_queue->insert({meta._past_timestamp % memory_window, meta._key});
        assert(meta._is_keybased_meta); // Siyuan: for key-value caching
        negative_candidate_queue->insert({meta._past_timestamp % memory_window, meta._keystr}); // Siyuan: for key-value caching

        uint32_t new_pos = out_cache_metas.size();
        Meta tmp_out_cache_meta(in_cache_metas[old_pos]);
        out_cache_metas.push_back(tmp_out_cache_meta);
        uint32_t activate_tail_idx = in_cache_metas.size() - 1;
        if (old_pos != activate_tail_idx) {
            //move tail
            in_cache_metas[old_pos] = in_cache_metas[activate_tail_idx];
            //key_map.find(in_cache_metas[activate_tail_idx]._key)->second.list_pos = old_pos;
            assert(in_cache_metas[activate_tail_idx]._is_keybased_meta); // Siyuan: for key-value caching
            key_map.find(in_cache_metas[activate_tail_idx]._keystr)->second.list_pos = old_pos; // Siyuan: for key-value caching
        }
        in_cache_metas.pop_back();
        key_map.find(keystr)->second = {1, new_pos};

        //_currentSize -= meta._size;
        _currentSize += (sizeof(uint64_t) + keystr.length() + tmp_out_cache_meta.getSizeForCapacity()); // Siyuan: forget timestamp and key in negative_candidate_queue + out-cache object-level metadata
        decreaseCurrentSize(keystr.length() + victim_in_cache_meta_size); // Siyuan: key in LRU list + in-cache object-level metadata
    }

    return true;
}

//void LRBCache::remove_from_outcache_metas(Meta &meta, unsigned int &pos, const uint64_t &key) {
void LRBCache::remove_from_outcache_metas(Meta &meta, unsigned int &pos, const std::string &key) { // Siyuan: for key-value caching
    //free the actual content
    uint64_t original_out_cache_meta_size = meta.getSizeForCapacity(); // Siyuan: for correct cache size usage calculation
    meta.free();
    //TODO: can add a function to delete from a queue with (key, pos)
    //evict
    uint32_t tail_pos = out_cache_metas.size() - 1;
    if (pos != tail_pos) {
        //swap tail
        out_cache_metas[pos] = out_cache_metas[tail_pos];
        //key_map.find(out_cache_metas[tail_pos]._key)->second.list_pos = pos;
        assert(out_cache_metas[tail_pos]._is_keybased_meta); // Siyuan: for key-value caching
        key_map.find(out_cache_metas[tail_pos]._keystr)->second.list_pos = pos; // Siyuan: for key-value caching
    }
    out_cache_metas.pop_back();
    key_map.erase(key);
    negative_candidate_queue->erase(current_seq % memory_window);

    decreaseCurrentSize(original_out_cache_meta_size + key.length() + sizeof(KeyMapEntryT) + sizeof(uint64_t) + key.length()); // Siyuan: out-cache object-level metadata + key and list index/pos in lookup table + forget timestamp and key in negative_candidate_queue
}

// Siyuan: for correct cache size usage calculation
void LRBCache::decreaseCurrentSize(const uint64_t decreased_size)
{
    if (_currentSize > decreased_size)
    {
        _currentSize -= decreased_size;
    }
    else
    {
        _currentSize = 0;
    }
    return;
}

// Siyuan: for key-value caching
bool LRBCache::has(const std::string &keystr) {
//bool has(const uint64_t &id) {
    //auto it = key_map.find(id);
    auto it = key_map.find(keystr);
    if (it == key_map.end())
        return false;
    return !it->second.list_idx;
}

void LRBCache::update_stat(bsoncxx::v_noabi::builder::basic::document &doc) {
    int64_t feature_overhead = 0;
    int64_t sample_overhead = 0;
    for (auto &m: in_cache_metas) {
        feature_overhead += m.feature_overhead();
        sample_overhead += m.sample_overhead();
    }
    for (auto &m: out_cache_metas) {
        feature_overhead += m.feature_overhead();
        sample_overhead += m.sample_overhead();
    }

    doc.append(kvp("n_metadata", static_cast<int32_t>(key_map.size())));
    doc.append(kvp("feature_overhead", feature_overhead));
    doc.append(kvp("sample_overhead", sample_overhead));
    doc.append(kvp("n_force_eviction", n_force_eviction));

    int res;
    auto importances = std::vector<double>(n_feature, 0);

    if (booster) {
        res = LGBM_BoosterFeatureImportance(booster,
                                            0,
                                            1,
                                            importances.data());
        if (res == -1) {
            std::cerr << "error: get model importance fail" << std::endl;
            abort();
        }
    }

    doc.append(kvp("segment_n_in", [this](sub_array child) {
        for (const auto &element : segment_n_in)
            child.append(element);
    }));

    doc.append(kvp("segment_n_out", [this](sub_array child) {
        for (const auto &element : segment_n_out)
            child.append(element);
    }));

    doc.append(kvp("segment_n_retrain", [this](sub_array child) {
        for (const auto &element : segment_n_retrain)
            child.append(element);
    }));

    doc.append(kvp("model_importance", [importances](sub_array child) {
        for (const auto &element : importances)
            child.append(element);
    }));

    auto v = get_object_distribution_n_past_timestamps();
    doc.append(kvp("object_distribution_n_past_timestamps", [v](sub_array child) {
        for (const auto &element : v)
            child.append(element);
    }));

    doc.append(kvp("segment_percent_beyond", [this](sub_array child) {
        for (const auto &element : segment_percent_beyond)
            child.append(element);
    }));

    doc.append(kvp("segment_positive_example_ratio", [this](sub_array child) {
        for (const auto &element : segment_positive_example_ratio)
            child.append(element);
    }));
#ifdef EVICTION_LOGGING
    doc.append(kvp("near_bytes", [this](sub_array child) {
        for (const auto &element : near_bytes)
            child.append(element);
    }));
    doc.append(kvp("middle_bytes", [this](sub_array child) {
        for (const auto &element : middle_bytes)
            child.append(element);
    }));
    doc.append(kvp("far_bytes", [this](sub_array child) {
        for (const auto &element : far_bytes)
            child.append(element);
    }));
    try {
        mongocxx::client client = mongocxx::client{mongocxx::uri(dburi)};
        auto db = client[mongocxx::uri(dburi).database()];
        auto bucket = db.gridfs_bucket();

        auto uploader = bucket.open_upload_stream(task_id + ".evictions");
        for (auto &b: eviction_qualities)
            uploader.write((uint8_t *) (&b), sizeof(uint8_t));
        uploader.close();
        uploader = bucket.open_upload_stream(task_id + ".eviction_timestamps");
        for (auto &b: eviction_logic_timestamps)
            uploader.write((uint8_t *) (&b), sizeof(uint16_t));
        uploader.close();
        uploader = bucket.open_upload_stream(task_id + ".trainings_and_predictions");
        for (auto &b: trainings_and_predictions)
            uploader.write((uint8_t *) (&b), sizeof(float));
        uploader.close();
//            uploader = bucket.open_upload_stream(task_id + ".training_and_prediction_timestamps");
//            for (auto &b: training_and_prediction_logic_timestamps)
//                uploader.write((uint8_t *) (&b), sizeof(uint16_t));
//            uploader.close();
    } catch (const std::exception &xcp) {
        std::cerr << "error: db connection failed: " << xcp.what() << std::std::endl;
        //continue to upload the simulation summaries
//            abort();
    }
#endif
}

std::vector<int> LRBCache::get_object_distribution_n_past_timestamps() {
    std::vector<int> distribution(max_n_past_timestamps, 0);
    for (auto &meta: in_cache_metas) {
        if (nullptr == meta._extra) {
            ++distribution[0];
        } else {
            ++distribution[meta._extra->_past_distances.size()];
        }
    }
    for (auto &meta: out_cache_metas) {
        if (nullptr == meta._extra) {
            ++distribution[0];
        } else {
            ++distribution[meta._extra->_past_distances.size()];
        }
    }
    return distribution;
}

} // End of namespace covered
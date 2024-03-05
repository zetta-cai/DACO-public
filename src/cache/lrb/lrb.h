//
// Created by zhenyus on 1/16/19.
//

#ifndef WEBCACHESIM_LRB_H
#define WEBCACHESIM_LRB_H

#include <assert.h>
#include <cmath>
#include <fstream>
#include <list>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sparsepp/spp.h>
#include <LightGBM/c_api.h>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>

#include "cache/lrb/cache.h"
//#include <webcachesim/cache.h>
//using namespace webcachesim;
#include "cache/lrb/request.h"

//using namespace std;
using spp::sparse_hash_map;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::sub_array;

namespace covered {
    // Siyuan: forward declaration
    class LRBCache;

    // Siyuan: remove global variables for thread-scalability
    //uint32_t current_seq = -1;
    //uint8_t max_n_past_timestamps = 32;
    //uint8_t max_n_past_distances = 31;
    //uint8_t base_edc_window = 10;
    //const uint8_t n_edc_feature = 10;
    //std::vector<uint32_t> edc_windows;
    //std::vector<double> hash_edc;
    //uint32_t max_hash_edc_idx;
    //uint32_t memory_window = 67108864;
    //uint32_t n_extra_fields = 0;
    //uint32_t batch_size = 131072;
    //const uint max_n_extra_feature = 4;
    //uint32_t n_feature;
    //TODO: interval clock should tick by event instead of following incoming packet time
#ifdef EVICTION_LOGGING
    //std::unordered_map<uint64_t, uint32_t> future_timestamps;
    //int n_logging_start;
    //std::vector<float> trainings_and_predictions;
    //bool start_train_logging = false;
    //int range_log = 1000000;
#endif

struct MetaExtra {
private:
    LRBCache* lrb_cache_ptr_; // Siyuan: used for access original global variables
public:
    //vector overhead = 24 (8 pointer, 8 size, 8 allocation)
    //40 + 24 + 4*x + 1
    //65 byte at least
    //189 byte at most
    //not 1 hit wonder
    float _edc[10];
    std::vector<uint32_t> _past_distances;
    //the next index to put the distance
    uint8_t _past_distance_idx = 1;

    MetaExtra(const uint32_t &distance, LRBCache* lrb_cache_ptr);

    // Siyuan: copy assignment constructor
    MetaExtra(const MetaExtra& other);

    void update(const uint32_t &distance);

    // Siyuan: for correct cache size usage calculation
    uint64_t getSizeForCapacity() const;
};

class Meta {
private:
    LRBCache* lrb_cache_ptr_; // Siyuan: used for access original global variables
public:
    //25 byte
    uint64_t _key;
    uint32_t _size;
    uint32_t _past_timestamp;
    uint16_t* _extra_features;
    MetaExtra *_extra = nullptr;
    std::vector<uint32_t> _sample_times;
#ifdef EVICTION_LOGGING
    std::vector<uint32_t> _eviction_sample_times;
    uint32_t _future_timestamp;
#endif

    // Siyuan: for key-value caching
    bool _is_keybased_meta;
    std::string _keystr;

    Meta();

#ifdef EVICTION_LOGGING
    Meta(const uint64_t &key, const uint64_t &size, const uint64_t &past_timestamp,
            const std::vector<uint16_t> &extra_features, const uint64_t &future_timestamp, LRBCache* lrb_cache_ptr, const bool& is_keybased_meta, const std::string& keystr); // Siyuan: for key-value caching
#else
    Meta(const uint64_t &key, const uint64_t &size, const uint64_t &past_timestamp,
            const std::vector<uint16_t> &extra_features, LRBCache* lrb_cache_ptr, const bool& is_keybased_meta, const std::string& keystr); // Siyuan: for key-value caching
#endif

    // Siyuan: copy assignment operator and constructor
    Meta(const Meta& other);
    const Meta& operator=(const Meta& other);

    virtual ~Meta();

    // Siyuan: for key-value caching
    bool operator==(const Meta& other);
    bool operator==(const SimpleRequest& other);

    void emplace_sample(uint32_t &sample_t);

#ifdef EVICTION_LOGGING
    void emplace_eviction_sample(uint32_t &sample_t);
#endif

    void free();

#ifdef EVICTION_LOGGING
    void update(const uint32_t &past_timestamp, const uint32_t &future_timestamp);
#else
    void update(const uint32_t &past_timestamp);
#endif

    int feature_overhead();

    int sample_overhead();

    // Siyuan: for correct cache size usage calculation
    uint64_t getSizeForCapacity() const;
};


class InCacheMeta : public Meta {
public:
    //pointer to lru0
    //std::list<int64_t>::const_iterator p_last_request;
    std::list<std::string>::const_iterator p_last_request; // Siyuan: for key-value caching
    //any change to functions?

    // Siyuan: for key-value caching
    std::string valuestr;

    InCacheMeta();

#ifdef EVICTION_LOGGING
    InCacheMeta(const uint64_t &key,
                const uint64_t &size,
                const uint64_t &past_timestamp,
                const std::vector<uint16_t> &extra_features,
                const uint64_t &future_timestamp,
                const std::list<KeyT>::const_iterator &it,
                LRBCache* lrb_cache_ptr);
#else
    InCacheMeta(const uint64_t &key,
                const uint64_t &size,
                const uint64_t &past_timestamp,
                const std::vector<uint16_t> &extra_features,
                //const std::list<int64_t>::const_iterator &it,
                const std::list<std::string>::const_iterator &it,
                LRBCache* lrb_cache_ptr,
                const bool& is_keybased_meta, const std::string& keystr, const std::string& valuestr); // Siyuan: for key-value caching
#endif

    //InCacheMeta(const Meta &meta, const std::list<int64_t>::const_iterator &it) : Meta(meta);
    InCacheMeta(const Meta &meta, const std::list<std::string>::const_iterator &it, const std::string& valuestr); // Siyuan: for key-value caching

    // Siyuan: copy assignment operator and constructor
    InCacheMeta(const InCacheMeta &other);
    const InCacheMeta& operator=(const InCacheMeta& other);

    // Siyuan: for correct cache size usage calculation
    uint64_t getSizeForCapacity() const;
};

class InCacheLRUQueue {
public:
    //std::list<int64_t> dq;
    std::list<std::string> dq; // Siyuan: for key-value caching

    //size?
    //the hashtable (location information is maintained outside, and assume it is always correct)
    //std::list<int64_t>::const_iterator request(int64_t key);
    std::list<std::string>::const_iterator request(std::string key); // Siyuan: for key-value caching

    //std::list<int64_t>::const_iterator re_request(std::list<int64_t>::const_iterator it);
    std::list<std::string>::const_iterator re_request(std::list<std::string>::const_iterator it); // Siyuan: for key-value caching
};

class TrainingData {
private:
    LRBCache* lrb_cache_ptr_; // Siyuan: used for access original global variables
public:
    std::vector<float> labels;
    std::vector<int32_t> indptr;
    std::vector<int32_t> indices;
    std::vector<double> data;

    TrainingData(LRBCache* lrb_cache_ptr);

    //void emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval, const uint64_t &key);
    void emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference

    void clear();

    uint64_t getSizeForcapacity() const;
};

#ifdef EVICTION_LOGGING
class LRBEvictionTrainingData {
private:
    LRBCache* lrb_cache_ptr_; // Siyuan: used for access original global variables
public:
    std::vector<float> labels;
    std::vector<int32_t> indptr;
    std::vector<int32_t> indices;
    std::vector<double> data;

    LRBEvictionTrainingData(LRBCache* lrb_cache_ptr);

    //void emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval, const uint64_t &key);
    void emplace_back(Meta &meta, uint32_t &sample_timestamp, uint32_t &future_interval); // Siyuan: LRB maps object-level metadata into next access time, yet key is NOT input for inference

    void clear();
};
#endif


struct KeyMapEntryT {
    unsigned int list_idx: 1;
    unsigned int list_pos: 31;
};

class LRBCache : public Cache {
public:
    // Siyuan: move global variables here for thread-scalability
    uint32_t current_seq = -1;
    uint8_t max_n_past_timestamps = 32;
    uint8_t max_n_past_distances = 31;
    uint8_t base_edc_window = 10;
    const uint8_t n_edc_feature = 10;
    std::vector<uint32_t> edc_windows;
    std::vector<double> hash_edc;
    uint32_t max_hash_edc_idx;
    uint32_t memory_window = 67108864;
    uint32_t n_extra_fields = 0;
    uint32_t batch_size = 131072;
    const uint max_n_extra_feature = 4;
    uint32_t n_feature;
#ifdef EVICTION_LOGGING
    std::unordered_map<uint64_t, uint32_t> future_timestamps;
    int n_logging_start;
    std::vector<float> trainings_and_predictions;
    bool start_train_logging = false;
    int range_log = 1000000;
#endif

    //key -> (0/1 list, idx)
    //sparse_hash_map<uint64_t, KeyMapEntryT> key_map;
    sparse_hash_map<std::string, KeyMapEntryT> key_map; // Siyuan: for key-value caching
//    std::vector<Meta> meta_holder[2];
    std::vector<InCacheMeta> in_cache_metas;
    std::vector<Meta> out_cache_metas;

    InCacheLRUQueue in_cache_lru_queue;
    //shared_ptr<sparse_hash_map<uint64_t, uint64_t>> negative_candidate_queue;
    std::shared_ptr<sparse_hash_map<uint64_t, std::string>> negative_candidate_queue; // Siyuan: for key-value caching
    TrainingData *training_data;
#ifdef EVICTION_LOGGING
    LRBEvictionTrainingData *eviction_training_data;
#endif

    // sample_size: use n_memorize keys + random choose (sample_rate - n_memorize) keys
    uint sample_rate = 64;

    double training_loss = 0;
    int32_t n_force_eviction = 0;

    double training_time = 0;
    double inference_time = 0;

    BoosterHandle booster = nullptr;

    std::unordered_map<std::string, std::string> training_params = {
            //don't use alias here. C api may not recongize
            {"boosting",         "gbdt"},
            {"objective",        "regression"},
            {"num_iterations",   "32"},
            {"num_leaves",       "32"},
            {"num_threads",      "4"},
            {"feature_fraction", "0.8"},
            {"bagging_freq",     "5"},
            {"bagging_fraction", "0.8"},
            {"learning_rate",    "0.1"},
            {"verbosity",        "0"},
    };

    std::unordered_map<std::string, std::string> inference_params;

    enum ObjectiveT : uint8_t {
        byte_miss_ratio = 0, object_miss_ratio = 1
    };
    ObjectiveT objective = byte_miss_ratio;

    std::default_random_engine _generator = std::default_random_engine();
    std::uniform_int_distribution<std::size_t> _distribution = std::uniform_int_distribution<std::size_t>();

    std::vector<int> segment_n_in;
    std::vector<int> segment_n_out;
    uint32_t obj_distribution[2];
    uint32_t training_data_distribution[2];  //1: pos, 0: neg
    std::vector<float> segment_positive_example_ratio;
    std::vector<double> segment_percent_beyond;
    int n_retrain = 0;
    std::vector<int> segment_n_retrain;
    bool is_sampling = false;

    uint64_t byte_million_req;
#ifdef EVICTION_LOGGING
    std::vector<uint8_t> eviction_qualities;
    std::vector<uint16_t> eviction_logic_timestamps;
    uint32_t n_req;
    int64_t n_early_stop = -1;
//    std::vector<uint16_t> training_and_prediction_logic_timestamps;
    std::string task_id;
    std::string dburi;
    uint64_t belady_boundary;
    std::vector<int64_t> near_bytes;
    std::vector<int64_t> middle_bytes;
    std::vector<int64_t> far_bytes;
#endif

    LRBCache(const uint32_t& dataset_keycnt);
    ~LRBCache();

    void init_with_params(const std::map<std::string, std::string> &params);

    uint32_t getInCacheMetadataCnt() const; // Siyuan: used to check if with cached objects or not

    bool access(SimpleRequest &req, const bool& is_update) override;

    void admit(const SimpleRequest &req) override;

    std::string getVictimKey(); // Siyuan: get victim key before eviction

    bool evict(const std::string& keystr, std::string& valuestr); // Siyuan: evict for the given victim (return whether evict successfully)

    void forget();

    //sample, rank the 1st and return
    //std::pair<uint64_t, uint32_t> rank();
    std::pair<std::string, uint32_t> rank(); // Siyuan: for key-value caching

    void train();

    void sample();

    void update_stat_periodic() override;

    // Siyuan: remove "static" from set_hash_edc for thread-scalability
    void set_hash_edc();

    //void remove_from_outcache_metas(Meta &meta, unsigned int &pos, const uint64_t &key);
    void remove_from_outcache_metas(Meta &meta, unsigned int &pos, const std::string &key); // Siyuan: for key-value caching

    // Siyuan: for correct size usage calculation
    void decreaseCurrentSize(const uint64_t decreased_size);

    // Siyuan: for key-value caching
    bool has(const std::string &keystr);

    void update_stat(bsoncxx::v_noabi::builder::basic::document &doc) override;

    std::vector<int> get_object_distribution_n_past_timestamps();
};

// Siyuan: NO need in LrbLocalCache wrapper
//static Factory<LRBCache> factoryLRB("LRB");

}
#endif //WEBCACHESIM_LRB_H

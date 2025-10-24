#include "cache/covered/hetero_key_level_metadata.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string HeteroKeyLevelMetadata::kClassName("HeteroKeyLevelMetadata");

    HeteroKeyLevelMetadata::HeteroKeyLevelMetadata() : KeyLevelMetadataBase()
    {
        redirected_frequency_ = 0;
        redirected_popularity_ = 0.0;
        p2p_redirected_reward_freq_ = 0;
        is_neighbor_cached_ = false;

        // NOTE: base class will set is_valid_ as false
    }

    HeteroKeyLevelMetadata::HeteroKeyLevelMetadata(const GroupId& group_id, const bool& is_neighbor_cached) : KeyLevelMetadataBase(group_id, is_neighbor_cached)
    {
        redirected_frequency_ = 0;
        redirected_popularity_ = 0.0;
        p2p_redirected_reward_freq_ = 0;
        is_neighbor_cached_ = is_neighbor_cached;

        is_valid_ = true;
    }

    HeteroKeyLevelMetadata::HeteroKeyLevelMetadata(const HeteroKeyLevelMetadata& other) : KeyLevelMetadataBase(other)
    {
        redirected_frequency_ = other.redirected_frequency_;
        redirected_popularity_ = other.redirected_popularity_;
        is_neighbor_cached_ = other.is_neighbor_cached_;
        p2p_redirected_reward_freq_ = other.p2p_redirected_reward_freq_;
        // NOTE: base class will set is_valid_ as other.is_valid_
    }

    HeteroKeyLevelMetadata::~HeteroKeyLevelMetadata() {}

    void HeteroKeyLevelMetadata::updateNoValueDynamicMetadata(const bool& is_redirected, const bool& is_global_cached, const Frequency& added_local_frequency, const Frequency& added_redirected_frequency)
    {
        checkValidity_();
        assert(is_global_cached == true); // Local cached objects MUST be global cached
        
        if (is_redirected)
        {
            redirected_frequency_ += 1; // By default +1
            p2p_redirected_reward_freq_ += added_redirected_frequency;
        }
        else
        {
            KeyLevelMetadataBase::updateNoValueDynamicMetadata_(added_local_frequency);
        }

        return;
    }

    void HeteroKeyLevelMetadata::updateRedirectedPopularity(const Popularity& redirected_popularity)
    {
        checkValidity_();
        redirected_popularity_ = redirected_popularity;

        return;
    }

    void HeteroKeyLevelMetadata::updateRedirectedPopularityReward(const Popularity& redirected_popularity)
    {
        checkValidity_();
        redirected_reward_popularity_ = redirected_popularity;

        return;
    }

    Frequency HeteroKeyLevelMetadata::getRedirectedFrequency() const
    {
        checkValidity_();
        return redirected_frequency_;
    }

    Frequency HeteroKeyLevelMetadata::getRedirectedRewardFreq() const
    {
        checkValidity_();
        return p2p_redirected_reward_freq_;
    }

    Popularity HeteroKeyLevelMetadata::getRedirectedPopularity() const
    {
        checkValidity_();
        return redirected_popularity_;
    }
    Popularity HeteroKeyLevelMetadata::getRedirectedPopularityReward() const
    {
        checkValidity_();
        return redirected_reward_popularity_;
    }
    void HeteroKeyLevelMetadata::enableIsNeighborCached()
    {
        checkValidity_();
        is_neighbor_cached_ = true;
        return;
    }

    void HeteroKeyLevelMetadata::disableIsNeighborCached()
    {
        checkValidity_();
        is_neighbor_cached_ = false;
        return;
    }

    bool HeteroKeyLevelMetadata::isNeighborCached() const
    {
        checkValidity_();
        bool is_neighbor_cached = is_neighbor_cached_;
        return is_neighbor_cached;
    }

    uint64_t HeteroKeyLevelMetadata::getSizeForCapacity()
    {
        uint64_t total_size = KeyLevelMetadataBase::getSizeForCapacity();
        total_size += sizeof(Frequency) + sizeof(Popularity);
        return total_size;
    }

    // Dump/load key-level metadata for local cached/uncached metadata of cache metadata in cache snapshot
    
    void HeteroKeyLevelMetadata::dumpKeyLevelMetadata(std::fstream* fs_ptr) const
    {
        checkValidity_();
        assert(fs_ptr != NULL);

        // Dump key-level metadata in base class
        KeyLevelMetadataBase::dumpKeyLevelMetadata(fs_ptr);

        // Dump redirected frequency
        fs_ptr->write((const char*)&redirected_frequency_, sizeof(Frequency));

        // Dump redirected popularity
        fs_ptr->write((const char*)&redirected_popularity_, sizeof(Popularity));

        // Dump is neighbor cached flag
        fs_ptr->write((const char*)&is_neighbor_cached_, sizeof(bool));

        return;
    }

    void HeteroKeyLevelMetadata::loadKeyLevelMetadata(std::fstream* fs_ptr)
    {
        assert(fs_ptr != NULL);

        // Load key-level metadata in base class
        KeyLevelMetadataBase::loadKeyLevelMetadata(fs_ptr);

        // Load redirected frequency
        fs_ptr->read((char*)&redirected_frequency_, sizeof(Frequency));

        // Load redirected popularity
        fs_ptr->read((char*)&redirected_popularity_, sizeof(Popularity));

        // Load is neighbor cached flag
        fs_ptr->read((char*)&is_neighbor_cached_, sizeof(bool));

        is_valid_ = true;

        return;
    }
}
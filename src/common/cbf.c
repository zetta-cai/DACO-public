#include "common/cbf.h"

#include <cmath> // log
#include <limits.h> // INT_MAX

#include "common/util.h"

namespace covered
{
    const std::string CBF::kClassName = "CBF";

    CBF::CBF(const uint64_t& entries, const double& error)
    {
        // Follow CBF theory to calculate # of counters and hashes
        const double num = log(error);
        const double denom = 0.480453013918201; // ln(2)^2
        const double bpe = -num / denom;
        int hashes = (int)ceil(0.693147180559945 * bpe); // ln(2)
        int counter_num = (int)fmin(ceil(bpe * entries), INT_MAX);

        // Verify # of counters and hashes
        if (counter_num < 1)
        {
            std::ostringstream oss;
            oss << "CBF has too few counters " << counter_num << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        assert(hashes >= 1);
        if (hashes > 10)
        {
            std::ostringstream oss;
            oss << "CBF has too many hashes " << hashes << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        if (counter_num < hashes)
        {
            std::ostringstream oss;
            oss << "CBF has too few counters " << counter_num << "! make it into 2 * " << hashes;
            counter_num = hashes * 2;
        }

        hash_wrappers_.resize(hashes, NULL);
        for (int i = 0; i < hashes; i++)
        {
            hash_wrappers_[i] = HashWrapperBase::getHashWrapperByHashName(Util::MMH3_HASH_NAME, i);
            assert(hash_wrappers_[i] != NULL);
        }
        cbf_.resize(counter_num, 0);
    }

    CBF::~CBF()
    {
        for (uint32_t i = 0; i < hash_wrappers_.size(); i++)
        {
            assert(hash_wrappers_[i] != NULL);
            delete hash_wrappers_[i];
            hash_wrappers_[i] = NULL;
        }
        hash_wrappers_.clear();
        cbf_.clear();
    }

    uint32_t CBF::estimate(const Key& key)
    {
        return access_(key, false);
    }

    uint32_t CBF::update(const Key& key)
    {
        return access_(key, true);
    }

    void CBF::decay()
    {
        for (uint32_t i = 0; i < cbf_.size(); i++)
        {
            cbf_[i] = cbf_[i] >> 1; // divided by 2
        }
        return;
    }

    uint64_t CBF::getSizeForCapacity() const
    {
        return cbf_.size() * sizeof(uint32_t);
    }

    uint32_t CBF::access_(const Key& key, const bool& is_update)
    {
        // Calculate hash values
        assert(cbf_.size() != 0);
        std::vector<uint32_t> hash_values;
        for (uint32_t i = 0; i < hash_wrappers_.size(); i++)
        {
            assert(hash_wrappers_[i] != NULL);
            hash_values.push_back(hash_wrappers_[i]->hash(key) % cbf_.size());
        }

        uint32_t min_count = UINT_MAX;
        for (uint32_t i = 0; i < hash_values.size(); i++)
        {
            uint32_t tmp_hash_value = hash_values[i];
            assert(tmp_hash_value < cbf_.size());
            if (cbf_[tmp_hash_value] < min_count)
            {
                min_count = cbf_[tmp_hash_value];
            }
        }

        if (is_update && min_count < UINT_MAX) {
            for (uint32_t i = 0; i < hash_values.size(); i++)
            {
                uint32_t tmp_hash_value = hash_values[i];
                assert(tmp_hash_value < cbf_.size());
                cbf_[tmp_hash_value]++;
            }
            min_count++;
        }

        return min_count;
    }
}
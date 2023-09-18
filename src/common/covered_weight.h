/*
 * CoveredWeight: a static class to manage cache-related weights used by different modules for COVERED (thread safe).
 * 
 * By Siyuan Sheng (2023.09.18).
 */

#ifndef COVERED_WEIGHT_H
#define COVERED_WEIGHT_H

#include <string>

#include "common/covered_common_header.h"
#include "concurrency/rwlock.h"

namespace covered
{
    class WeightInfo
    {
    public:
        WeightInfo();
        WeightInfo(const Weight& local_hit_weight, const Weight& cooperative_hit_weight);
        ~WeightInfo();

        Weight getLocalHitWeight() const;
        Weight getCooperativeHitWeight() const;

        const WeightInfo& operator=(const WeightInfo& other);
    private:
        static const std::string kClassName;

        Weight local_hit_weight_; // w1
        Weight cooperative_hit_weight_; // w2
    };

    class CoveredWeight
    {
    public:
        // NOTE: CoveredWeight is a static class with NO need for constructors and deconstructors

        static WeightInfo getWeightInfo();
        static void setWeightInfo(const WeightInfo& weight_info);
    private:
        static const std::string kClassName;

        static Rwlock rwlock_for_covered_weight_;
        static bool is_valid_;
        static WeightInfo weight_info_;
    };
}

#endif
/*
 * Different custom function parameters of baselines for cache module.
 *
 * By Siyuan Sheng (2024.01.25).
 */

#ifndef BASIC_CACHE_CUSTOM_FUNC_PARAM_H
#define BASIC_CACHE_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "cache/cache_custom_func_param_base.h"
#include "common/key.h"
#include "core/popularity/collected_popularity.h"
#include "core/victim/victim_cacheinfo.h"

namespace covered
{
    // GetLocalVictimVtimeFuncParam for BestGuess

    class GetLocalVictimVtimeFuncParam : public CacheCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // get victim vtime of the current edge node

        GetLocalVictimVtimeFuncParam();
        virtual ~GetLocalVictimVtimeFuncParam();

        uint64_t getLocalVictimVtime() const;
        void setLocalVictimVtime(const uint64_t& local_victim_vtime);
    private:
        static const std::string kClassName;

        uint64_t local_victim_vtime_;
    };

    // UpdateNeighborVictimVtimeParam for BestGuess

    class UpdateNeighborVictimVtimeParam : public CacheCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Update victim vtime of the given neighbor edge node

        UpdateNeighborVictimVtimeParam(const uint32_t& neighbor_edge_idx, const uint64_t& neighbor_victim_vtime);
        virtual ~UpdateNeighborVictimVtimeParam();

        uint32_t getNeighborEdgeIdx() const;
        uint64_t getNeighborVictimVtime() const;
    private:
        static const std::string kClassName;

        const uint32_t neighbor_edge_idx_;
        const uint64_t neighbor_victim_vtime_;
    };

    // GetPlacementEdgeIdxParam for BestGuess

    class GetPlacementEdgeIdxParam : public CacheCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // Get placement edge idx for best-guess replacement

        GetPlacementEdgeIdxParam();
        virtual ~GetPlacementEdgeIdxParam();

        uint32_t getPlacementEdgeIdx() const;
        void setPlacementEdgeIdx(const uint32_t& placement_edge_idx);
    private:
        static const std::string kClassName;

        uint32_t placement_edge_idx_;
    };
}

#endif
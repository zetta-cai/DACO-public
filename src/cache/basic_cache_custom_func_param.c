#include "cache/basic_cache_custom_func_param.h"

namespace covered
{
    // GetLocalVictimVtimeFuncParam for BestGuess

    const std::string GetLocalVictimVtimeFuncParam::kClassName("GetLocalVictimVtimeFuncParam");

    const std::string GetLocalVictimVtimeFuncParam::FUNCNAME("get_local_victim_vtime");

    GetLocalVictimVtimeFuncParam::GetLocalVictimVtimeFuncParam() : CacheCustomFuncParamBase(false, false, Key(), false)
    {
        local_victim_vtime_ = 0;
    }

    GetLocalVictimVtimeFuncParam::~GetLocalVictimVtimeFuncParam()
    {
    }

    uint64_t& GetLocalVictimVtimeFuncParam::getLocalVictimVtimeRef()
    {
        return local_victim_vtime_;
    }

    // UpdateNeighborVictimVtimeParam for BestGuess

    const std::string UpdateNeighborVictimVtimeParam::kClassName("UpdateNeighborVictimVtimeParam");

    const std::string UpdateNeighborVictimVtimeParam::FUNCNAME("update_neighbor_victim_vtime");

    UpdateNeighborVictimVtimeParam::UpdateNeighborVictimVtimeParam(const uint32_t& neighbor_edge_idx, const uint64_t& neighbor_victim_vtime)
        : CacheCustomFuncParamBase(false, false, Key(), true), neighbor_edge_idx_(neighbor_edge_idx), neighbor_victim_vtime_(neighbor_victim_vtime)
    {
    }

    UpdateNeighborVictimVtimeParam::~UpdateNeighborVictimVtimeParam()
    {
    }

    uint32_t UpdateNeighborVictimVtimeParam::getNeighborEdgeIdx() const
    {
        return neighbor_edge_idx_;
    }

    uint64_t UpdateNeighborVictimVtimeParam::getNeighborVictimVtime() const
    {
        return neighbor_victim_vtime_;
    }

    // GetPlacementEdgeIdxParam for BestGuess

    const std::string GetPlacementEdgeIdxParam::kClassName("GetPlacementEdgeIdxParam");

    const std::string GetPlacementEdgeIdxParam::FUNCNAME("get_placement_edge_idx");

    GetPlacementEdgeIdxParam::GetPlacementEdgeIdxParam()
        : CacheCustomFuncParamBase(false, false, Key(), false)
    {
        placement_edge_idx_ = 0;
    }

    GetPlacementEdgeIdxParam::~GetPlacementEdgeIdxParam()
    {
    }

    uint32_t& GetPlacementEdgeIdxParam::getPlacementEdgeIdxRef()
    {
        return placement_edge_idx_;
    }

    const uint32_t& GetPlacementEdgeIdxParam::getPlacementEdgeIdxConstRef() const
    {
        return placement_edge_idx_;
    }
}
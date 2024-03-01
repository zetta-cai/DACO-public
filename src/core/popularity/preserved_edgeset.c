#include "core/popularity/preserved_edgeset.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string PreservedEdgeset::kClassName("PreservedEdgeset");

    PreservedEdgeset::PreservedEdgeset(const uint32_t& edgecnt) : preserved_bitmap_(edgecnt, false)
    {
        assert(preserved_bitmap_.size() > 0);
    }

    PreservedEdgeset::~PreservedEdgeset() {}

    bool PreservedEdgeset::isPreserved(const uint32_t edge_idx) const
    {
        assert(edge_idx < preserved_bitmap_.size());
        return preserved_bitmap_[edge_idx];
    }

    void PreservedEdgeset::preserveEdgesetForPlacement(const Edgeset& placement_edgeset)
    {
        for (std::unordered_set<uint32_t>::const_iterator placement_edgeset_const_iter = placement_edgeset.begin(); placement_edgeset_const_iter != placement_edgeset.end(); placement_edgeset_const_iter++)
        {
            const uint32_t tmp_edge_idx = *placement_edgeset_const_iter;
            assert(tmp_edge_idx < preserved_bitmap_.size());

            // (OBSOLETE) NOTE: there should NOT be duplicate placement on the same edge node
            //assert(preserved_bitmap_[tmp_edge_idx] == false);

            // NOTE: cache server worker and beacon server may perform cache placement for the same key simultaneously (triggered by local/remote directory lookup) and CoveredManager::placementCalculation_() is NOT atomic -> one placement decision may already preserve the edge node before another
            if (preserved_bitmap_[tmp_edge_idx] == true)
            {
                std::ostringstream oss;
                oss << "Edge node " << tmp_edge_idx << " is already preserved in preserved edgeset for placement " << placement_edgeset.toString() << ", which may be caused by occasional duplicate placement decision due to NOT strong atomicity of CoveredManager::placementCalculation_()";
                Util::dumpInfoMsg(kClassName, oss.str());
            }
            else
            {
                preserved_bitmap_[tmp_edge_idx] = true;
            }
        }
        
        return;
    }

    bool PreservedEdgeset::clearPreservedEdgeNode(const uint32_t edge_idx)
    {
        assert(edge_idx < preserved_bitmap_.size());

        if (preserved_bitmap_[edge_idx])
        {
            preserved_bitmap_[edge_idx] = false;
        }
        else
        {
            #ifdef ENABLE_FAST_PATH_PLACEMENT
            // NOTE: as fast-path placement is performed in sender edge node which is NOT awared by beacon edge node, beacon edge node may NOT preserve the sender edge idx
            #else
            std::ostringstream oss;
            oss << "Edge node " << edge_idx << " is NOT preserved in preserved edgeset";
            Util::dumpWarnMsg(kClassName, oss.str());
            #endif
        }

        bool is_empty = true;
        for (std::vector<bool>::const_iterator preserved_bitmap_const_iter = preserved_bitmap_.begin(); preserved_bitmap_const_iter != preserved_bitmap_.end(); preserved_bitmap_const_iter++)
        {
            if (*preserved_bitmap_const_iter == true)
            {
                is_empty = false;
            }
        }

        return is_empty;
    }

    uint64_t PreservedEdgeset::getSizeForCapacity() const
    {
        // NOTE: std::vector<bool> is just an impl trick, while the actual space cost is actually one bit for each edge node
        return (preserved_bitmap_.size() * sizeof(bool) - 1) / 8 + 1;
    }
}
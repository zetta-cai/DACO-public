#include "core/popularity/preserved_edgeset.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string PreservedEdgeset::kClassName("PreservedEdgeset");

    PreservedEdgeset::PreservedEdgeset(const uint32_t& edgecnt) : preserved_bitmap_(edgecnt, false) {}

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

            // NOTE: there should NOT be duplicate placement on the same edge node
            assert(preserved_bitmap_[tmp_edge_idx] == false);
            preserved_bitmap_[tmp_edge_idx] = true;
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
            std::ostringstream oss;
            oss << "Edge node " << edge_idx << " is NOT preserved in preserved edgeset";
            Util::dumpWarnMsg(kClassName, oss.str());
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
        return preserved_bitmap_.size() * sizeof(bool);
    }
}
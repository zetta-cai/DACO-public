/*
 * PreservedEdgeset: preserve edge nodes for non-blocking placement deployment.
 *
 * By Siyuan Sheng (2023.09.22).
 */

#ifndef PRESERVED_EDGESET_H
#define PRESERVED_EDGESET_H

#include <string>
#include <unordered_set>
#include <vector>

namespace covered
{
    class PreservedEdgeset
    {
    public:
        PreservedEdgeset(const uint32_t& edgecnt);
        ~PreservedEdgeset();

        bool isPreserved(const uint32_t edge_idx) const;

        void preserveEdgesetForPlacement(const std::unordered_set<uint32_t>& placement_edgeset);

        uint64_t getSizeForCapacity() const;
    private:
        static const std::string kClassName;

        std::vector<bool> preserved_bitmap_;
    };
}

#endif
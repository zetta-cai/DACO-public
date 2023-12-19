/*
 * SynchronizationServerItem: an item passed between cache/beacon server and synchronization server to update victim tracker for remote victim synchronization.
 * 
 * By Siyuan Sheng (2023.12.13).
 */

#ifndef SYNCHRONIZATION_SERVER_ITEM_H
#define SYNCHRONIZATION_SERVER_ITEM_H

#include <string>

#include "core/victim/victim_syncset.h"

namespace covered
{
    class SynchronizationServerItem
    {
    public:
        SynchronizationServerItem();
        SynchronizationServerItem(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset);
        ~SynchronizationServerItem();

        uint32_t getSourceEdgeIdx() const;
        const VictimSyncset& getNeighborVictimSyncsetRef() const;

        const SynchronizationServerItem& operator=(const SynchronizationServerItem& other);
    private:
        static const std::string kClassName;

        uint32_t source_edge_idx_;
        VictimSyncset neighbor_victim_syncset_;
    };
}

#endif
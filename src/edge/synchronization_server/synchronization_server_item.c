#include "edge/synchronization_server/synchronization_server_item.h"

namespace covered
{
    SynchronizationServerItem::SynchronizationServerItem()
    : source_edge_idx_(0), neighbor_victim_syncset_(VictimSyncset())
    {
    }

    SynchronizationServerItem::SynchronizationServerItem(const uint32_t& source_edge_idx, const VictimSyncset& neighbor_victim_syncset)
    : source_edge_idx_(source_edge_idx), neighbor_victim_syncset_(neighbor_victim_syncset)
    {
    }

    SynchronizationServerItem::~SynchronizationServerItem()
    {
    }

    uint32_t SynchronizationServerItem::getSourceEdgeIdx() const
    {
        return source_edge_idx_;
    }

    const VictimSyncset& SynchronizationServerItem::getNeighborVictimSyncsetRef() const
    {
        return neighbor_victim_syncset_;
    }

    const SynchronizationServerItem& SynchronizationServerItem::operator=(const SynchronizationServerItem& other)
    {
        source_edge_idx_ = other.source_edge_idx_;
        neighbor_victim_syncset_ = other.neighbor_victim_syncset_;
        return *this;
    }
}
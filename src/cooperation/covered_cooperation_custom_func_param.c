#include "cooperation/covered_cooperation_custom_func_param.h"

namespace covered
{
    // GetLocalBeaconedVictimsFromVictimSyncsetFuncParam

    const std::string GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::kClassName("GetLocalBeaconedVictimsFromVictimSyncsetFuncParam");

    const std::string GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::FUNCNAME("get_local_beaconed_victims_from_victim_syncset");

    GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::GetLocalBeaconedVictimsFromVictimSyncsetFuncParam(const VictimSyncset& victim_syncset_const_ref) : CooperationCustomFuncParamBase(), victim_syncset_const_ref_(victim_syncset_const_ref)
    {
        local_beaconed_victim_dirinfosets_.clear();
    }

    GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::~GetLocalBeaconedVictimsFromVictimSyncsetFuncParam()
    {
        local_beaconed_victim_dirinfosets_.clear();
    }

    const VictimSyncset& GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::getVictimSyncsetConstRef() const
    {
        return victim_syncset_const_ref_;
    }

    std::list<std::pair<Key, DirinfoSet>>& GetLocalBeaconedVictimsFromVictimSyncsetFuncParam::getLocalBeaconedVictimDirinfosetsRef()
    {
        return local_beaconed_victim_dirinfosets_;
    }

    // GetLocalBeaconedVictimsFromCacheinfosParam

    const std::string GetLocalBeaconedVictimsFromCacheinfosParam::kClassName("GetLocalBeaconedVictimsFromCacheinfosParam");

    const std::string GetLocalBeaconedVictimsFromCacheinfosParam::FUNCNAME("get_local_beaconed_victims_from_cacheinfos");

    GetLocalBeaconedVictimsFromCacheinfosParam::GetLocalBeaconedVictimsFromCacheinfosParam(const std::list<VictimCacheinfo>& victim_cacheinfos_const_ref) : CooperationCustomFuncParamBase(), victim_cacheinfos_const_ref_(victim_cacheinfos_const_ref)
    {
        local_beaconed_victim_dirinfosets_.clear();
    }

    GetLocalBeaconedVictimsFromCacheinfosParam::~GetLocalBeaconedVictimsFromCacheinfosParam()
    {
        local_beaconed_victim_dirinfosets_.clear();
    }

    const std::list<VictimCacheinfo>& GetLocalBeaconedVictimsFromCacheinfosParam::getVictimCacheinfosConstRef() const
    {
        return victim_cacheinfos_const_ref_;
    }

    std::list<std::pair<Key, DirinfoSet>>& GetLocalBeaconedVictimsFromCacheinfosParam::getLocalBeaconedVictimDirinfosetsRef()
    {
        return local_beaconed_victim_dirinfosets_;
    }
}
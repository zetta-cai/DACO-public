/*
 * Different custom function parameters of COVERED for cooperation module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef COVERED_COOPERATION_CUSTOM_FUNC_PARAM_H
#define COVERED_COOPERATION_CUSTOM_FUNC_PARAM_H

#include <list>
#include <string>

#include "common/key.h"
#include "common/value.h"
#include "cooperation/cooperation_custom_func_param_base.h"
#include "cooperation/directory/dirinfo_set.h"
#include "core/victim/victim_syncset.h"

namespace covered
{
    // GetLocalBeaconedVictimsFromVictimSyncsetFuncParam

    class GetLocalBeaconedVictimsFromVictimSyncsetFuncParam : public CooperationCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // get local-beaconed victim dirinfosets for neighbor-synced victims in victim syncset

        GetLocalBeaconedVictimsFromVictimSyncsetFuncParam(const VictimSyncset& victim_syncset_const_ref);
        virtual ~GetLocalBeaconedVictimsFromVictimSyncsetFuncParam();

        const VictimSyncset& getVictimSyncsetConstRef() const;
        std::list<std::pair<Key, DirinfoSet>>& getLocalBeaconedVictimDirinfosetsRef();
    private:
        static const std::string kClassName;

        const VictimSyncset& victim_syncset_const_ref_;
        std::list<std::pair<Key, DirinfoSet>> local_beaconed_victim_dirinfosets_;
    };

    // GetLocalBeaconedVictimsFromCacheinfosParam

    class GetLocalBeaconedVictimsFromCacheinfosParam : public CooperationCustomFuncParamBase
    {
    public:
        static const std::string FUNCNAME; // get local-beaconed victim dirinfosets for local-/neighbor-synced victims in victim cacheinfos

        GetLocalBeaconedVictimsFromCacheinfosParam(const std::list<VictimCacheinfo>& victim_cacheinfos_const_ref);
        virtual ~GetLocalBeaconedVictimsFromCacheinfosParam();

        const std::list<VictimCacheinfo>& getVictimCacheinfosConstRef() const;
        std::list<std::pair<Key, DirinfoSet>>& getLocalBeaconedVictimDirinfosetsRef();
    private:
        static const std::string kClassName;

        const std::list<VictimCacheinfo>& victim_cacheinfos_const_ref_;
        std::list<std::pair<Key, DirinfoSet>> local_beaconed_victim_dirinfosets_;
    };
}

#endif
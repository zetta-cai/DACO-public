#include "common/kv_list_helper.h"

namespace covered
{
    template<class K, class V>
    const std::string KVListHelper<K, V>::kClassName("KVListHelper");

    template<class K, class V>
    typename std::list<std::pair<K, V>>::iterator KVListHelper<K, V>::findVFromListForK(const K& k, std::list<std::pair<K, V>>& list)
    {
        typename std::list<std::pair<K, V>>::iterator iter = list.begin();
        for (; iter != list.end(); iter++)
        {
            if (iter->first == k)
            {
                break;
            }
        }
        return iter;
    }

    template<class K, class V>
    typename std::list<std::pair<K, V>>::const_iterator KVListHelper<K, V>::findVFromListForK(const K& k, const std::list<std::pair<K, V>>& list)
    {
        typename std::list<std::pair<K, V>>::const_iterator iter = list.begin();
        for (; iter != list.end(); iter++)
        {
            if (iter->first == k)
            {
                break;
            }
        }
        return iter;
    }
}
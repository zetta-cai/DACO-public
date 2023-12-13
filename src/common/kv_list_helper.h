/*
 * KVListHelper: a static class to support helper functions for a list of pairs.
 *
 * NOTE: This class is ONLY used to reduce hash and string comparison time cost for a limited number of pairs (e.g., victims in victim synchronization of COVERED); or std::unordered_map / std::map may be better otherwise.
 * 
 * By Siyuan Sheng (2023.12.13).
 */

#include <list>
#include <string>

namespace covered
{
    template<class K, class V>
    class KVListHelper
    {
    public:
        KVListHelper() = delete;
        ~KVListHelper() = delete;

        static typename std::list<std::pair<K, V>>::iterator findVFromListForK(const K& k, std::list<std::pair<K, V>>& list);
        static typename std::list<std::pair<K, V>>::const_iterator findVFromListForK(const K& k, const std::list<std::pair<K, V>>& list);
        
    private:
        static const std::string kClassName;
    };
}
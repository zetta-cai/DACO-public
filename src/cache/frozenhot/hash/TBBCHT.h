//#include <hash/FastHashBase.h> // Siyuan: lib/frozenhot/hash/FastHashBase.h
#include "cache/frozenhot/hash/FastHashBase.h" // Siyuan: src/cache/frozenhot/hash/FastHashBase.h for covered::FastHashAPI

#ifndef COVERED_TBBCHT_H
#define COVERED_TBBCHT_H

namespace covered
{
    template <typename TValue>
    class TbbCHT : public FashHashAPI<TValue>
    {
        typedef typename tbb::concurrent_hash_map<TKey, TValue> HashMap;
        typedef typename HashMap::const_accessor HashMapConstAccessor;
        //typedef typename HashMap::accessor HashMapAccessor;
        typedef typename HashMap::value_type HashMapValuePair;

    protected:
        std::unique_ptr<HashMap> m_map;
    
    public:
        bool insert(TKey idex, TValue value){
            HashMapConstAccessor const_accessor;
            HashMapValuePair hashMapValue(idex, value);
            if(m_map->insert(const_accessor, hashMapValue))
                return true;
            else
                return false;
        }

        bool find(TKey idx, TValue &value) {
            HashMapConstAccessor const_accessor;
            if(m_map->find(const_accessor, idx)){
                value = const_accessor->second;
                return true;
            } else {
                return false;
            }
        }

        // Siyuan: for in-place update (insert-after-remove)
        bool remove(TKey idx, TValue& value)
        {
            HashMapConstAccessor const_accessor;
            if(m_map->find(const_accessor, idx)){
                value = const_accessor->second;
                m_map.erase(idx);
                return true;
            } else {
                return false;
            }
        }

        TbbCHT(TKey size) {
            m_map.reset(new HashMap(size));
        }

        void clear() {
            m_map->clear();
        }
    };
} // namespace covered

#endif
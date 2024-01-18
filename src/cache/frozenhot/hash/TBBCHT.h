//#include <hash/FastHashBase.h> // Siyuan: lib/frozenhot/hash/FastHashBase.h
#include "cache/frozenhot/hash/FastHashBase.h" // Siyuan: src/cache/frozenhot/hash/FastHashBase.h for covered::FastHashAPI

#ifndef COVERED_TBBCHT_H
#define COVERED_TBBCHT_H

namespace covered
{
    template <typename TKey, typename TValue, class THash>
    class TbbCHT : public FashHashAPI<TKey, TValue, THash>
    {
        typedef typename tbb::concurrent_hash_map<TKey, TValue, THash> HashMap;
        typedef typename HashMap::const_accessor HashMapConstAccessor;
        typedef typename HashMap::accessor HashMapAccessor;
        typedef typename HashMap::value_type HashMapValuePair;

    protected:
        std::unique_ptr<HashMap> m_map;
    
    public:
        virtual bool insert(TKey idx, TValue value){
            HashMapConstAccessor const_accessor;
            HashMapValuePair hashMapValue(idx, value);
            if(m_map->insert(const_accessor, hashMapValue))
                return true;
            else
                return false;
        }

        virtual bool find(TKey idx, TValue &value) {
            HashMapConstAccessor const_accessor;
            if(m_map->find(const_accessor, idx)){
                value = const_accessor->second;
                return true;
            } else {
                return false;
            }
        }

        // Siyuan: additional required interfaces
        virtual bool remove(TKey idx, TValue& value)
        {
            HashMapAccessor accessor;
            if (m_map->find(accessor, idx)) // Siyuan: this will acquire a write lock for the key
            {
                value = accessor->second;
                m_map->erase(accessor); // Siyuan: destructor of accessor will release the write lock for the key
                return true;
            }
            else
            {
                return false;
            }
        }
        virtual bool update(TKey idx, TValue value, TValue& original_value)
        {
            HashMapAccessor accessor;
            if (m_map->find(accessor, idx)) // Siyuan: this will acquire a write lock for the key
            {
                original_value = accessor->second;
                accessor->second = value; // Siyuan: accessor::operation->() returns the pointer of the reference of the value at the corresponding hashmap node
                return true;
            }
            else
            {
                return false;
            }
        }

        //TbbCHT(TKey size) {
        TbbCHT(size_t size) { // Siyuan: fix impl error
            m_map.reset(new HashMap(size));
        }

        virtual void clear() {
            m_map->clear();
        }
    };
} // namespace covered

#endif
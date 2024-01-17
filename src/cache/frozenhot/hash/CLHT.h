//#include <hash/FastHashBase.h> // Siyuan: lib/frozenhot/hash/FastHashBase.h
#include "cache/frozenhot/hash/FastHashBase.h" // Siyuan: src/cache/frozenhot/hash/FastHashBase.h for covered::FastHashAPI
#include <clht.h> // Siyuan: lib/frozenhot/CLHT/include/clht.h

#ifndef COVERED_CLHT_H
#define COVERED_CLHT_H

namespace covered
{
    template <typename TValue>
    class CLHT :public FashHashAPI<TValue> {
    private:
	    clht_t* m_hashtable;
        uint32_t m_buckets;

    public:
	CLHT(int size, int exp) {
            int ex = 0;
            while (size != 0) {
                ex++;
                size >>= 1;
            }
	    m_buckets = (uint32_t)(1 << (ex + exp));
	    printf ("Number of buckets: %u\n", m_buckets);
            m_hashtable = clht_create(m_buckets);
	};

	~CLHT() { 
	    clht_gc_destroy(m_hashtable);
	}

    virtual void thread_init(int tid) {
	    clht_gc_thread_init(m_hashtable, (uint32_t)tid);
	}

	virtual bool insert(TKey idx, TValue value) {
           return clht_put(m_hashtable, (clht_addr_t)idx, (clht_val_t)(value.get())) != 0;
	};

	virtual bool find(TKey idx, TValue &value) {
	   clht_val_t v = clht_get(m_hashtable->ht, (clht_addr_t)idx);
	   if (v == 0) return false;
	   value.reset((std::string*)v, [](auto p) {});
	   return true;
	};

	// Siyuan: additional required interfaces
	virtual bool remove(TKey idx, TValue& value)
	{
		clht_val_t v = clht_remove(m_hashtable, (clht_addr_t)idx);
		if (v == 0 || v == false) return false;
		value.reset((std::string*)v, [](auto p) {});
		return true;
	}
	bool update(TKey idx, TValue value, TValue& original_value)
	{
		// NOTE: CLHT does NOT provide atomic semantics to update value of existing key
		// TODO: Use insert-after-remove to implement update, which may need additional locking for atomicity
		remove(idx, original_value);
		return insert(idx, value);
	}

	virtual void clear() {
	    clht_clear(m_hashtable->ht);
	};
    };

} // namespace covered

#endif
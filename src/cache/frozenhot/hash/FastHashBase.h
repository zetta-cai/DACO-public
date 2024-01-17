#ifndef COVERED_COMMON_WORKSPACE_H_
#define COVERED_COMMON_WORKSPACE_H_

namespace covered
{
    typedef unsigned TKey;

    template <typename TValue> 
    class FashHashAPI {
    public:
        virtual bool insert(TKey idx, TValue value) = 0;

        virtual bool find(TKey idx, TValue &value) = 0;

        virtual void thread_init(int tid) {}

        // Siyuan: additional required interfaces
        virtual bool remove(TKey idx, TValue& value) = 0;
        virtual bool update(TKey idx, TValue value, TValue& original_value) = 0;

        virtual void clear() = 0;
    };
    
} // namespace covered

#endif // COVERED_COMMON_WORKSPACE_H_
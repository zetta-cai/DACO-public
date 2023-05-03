/*
 * DynamicArray: encapsulate std::vector<char> as a dynamic array w/ array range checking.
 * 
 * By Siyuan Sheng (2023.05.02).
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <string>
#include <vector>

namespace covered
{
    class DynamicArray
    {
    public:
        DynamicArray();
        DynamicArray(const uint32_t& capacity);
        ~DynamicArray();

        const std::vector<char>& getBytes() const;
        void clear(const uint32_t& capacity);

        uint32_t getSize() const;
        uint32_t getCapacity() const;

        void write(uint32_t position, const char* data, uint32_t length);
        void arrayset(uint32_t position, int charval, uint32_t length);
        void read(uint32_t position, char* data, uint32_t length) const;
        void arraycpy(uint32_t position, DynamicArray& dstarray, uint32_t dstarray_position, uint32_t length) const;
    private:
        static const std::string kClassName;

        std::vector<char> bytes_;
    };
}

#endif
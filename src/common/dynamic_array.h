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

        std::vector<char>& getBytesRef();
        const std::vector<char>& getBytesConstRef() const;
        void clear(const uint32_t& capacity);

        uint32_t getSize() const;
        uint32_t getCapacity() const;
        std::string getBytesHexstr() const;

        void deserialize(uint32_t position, const char* data, uint32_t length); // Deserialize dynamic array from byte array
        void arrayset(uint32_t position, int charval, uint32_t length); // Memset current dynamic array
        void serialize(uint32_t position, char* data, uint32_t length) const; // Serialize dynamic array to byte array
        void arraycpy(uint32_t position, DynamicArray& dstarray, uint32_t dstarray_position, uint32_t length) const; // Copy current dynamic array to another

        void writeBinaryFile(uint32_t position, std::fstream* fs_ptr, uint32_t length) const; // Write dynamic array into a binary file
        void readBinaryFile(uint32_t position, std::fstream* fs_ptr, uint32_t length); // Read dynamic array from a binary file
    private:
        static const std::string kClassName;

        std::vector<char> bytes_;
    };
}

#endif
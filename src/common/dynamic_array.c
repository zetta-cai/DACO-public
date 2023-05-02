#include "common/dynamic_array.h"

#include <assert.h>
#include <cstring> // memcpy
#include <sstream>

#include "common/util.h"

namespace covered
{
    const std::string DynamicArray::kClassName("DynamicArray");

    DynamicArray::DynamicArray()
    {
        bytes_.resize(0);
        bytes_.reserve(0);
    }

    DynamicArray::DynamicArray(const uint32_t& capacity)
    {
        clear(capacity);
    }
    
    DynamicArray::~DynamicArray() {};

    const std::vector<char>& DynamicArray::getBytes() const
    {
        return bytes_;
    }

    void DynamicArray::clear(const uint32_t& capacity)
    {
        bytes_.resize(0);
        bytes_.reserve(capacity);
    }

    uint32_t DynamicArray::getSize() const
    {
        return bytes_.size();
    }
    
    uint32_t DynamicArray::getCapacity() const
    {
        return bytes_.capacity();
    }

    void DynamicArray::write(uint32_t position, const char* data, uint32_t length)
    {
        // Should NOT exceed capacity; otherwise, you need to use clear to increase capacity before write
        if (position + length > bytes_.capacity())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the capacity " << bytes_.capacity() << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        if (position + length > bytes_.size())
        {
            bytes_.resize(position + length);
        }

        assert(data != NULL);
        memcpy((void*)(bytes_.data() + position), (const void*)data, length);
        return;
    }

    void DynamicArray::read(uint32_t position, char* data, uint32_t length) const
    {
        // Should NOT exceed size; otherwise, you need to use write to increase size before read
        if (position + length > bytes_.size())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the size " << bytes_.size() << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(data != NULL);
        memcpy((void*)data, (const void*)(bytes_.data() + position), length);
        return;
    }

    void DynamicArray::arraycpy(uint32_t position, DynamicArray& dstarray, uint32_t dstarray_position, uint32_t length) const
    {
        // Should NOT exceed size; otherwise, you need to use write to increase size before read
        if (position + length > bytes_.size())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the size " << bytes_.size() << "!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        dstarray.write(dstarray_position, bytes_.data() + position, length);
        return;
    }
}
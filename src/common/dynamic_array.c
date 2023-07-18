#include "common/dynamic_array.h"

#include <assert.h>
#include <cstring> // memcpy memset
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

    const std::vector<char>& DynamicArray::getBytesRef() const
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

    std::string DynamicArray::getBytesHexstr() const
    {
        std::ostringstream oss;
        char default_fill = oss.fill();
        std::streamsize default_width = oss.width();
        for (uint32_t idx = 0; idx < bytes_.size(); idx++)
        {
            oss.fill('0');
            oss.width(2);
            oss << std::hex << static_cast<int>(bytes_[idx]);
            if (idx != bytes_.size() - 1)
            {
                oss << " ";
            }
        }
        oss.fill(default_fill);
        oss.width(default_width);
        return oss.str();
    }

    void DynamicArray::deserialize(uint32_t position, const char* data, uint32_t length)
    {
        // Should NOT exceed capacity; otherwise, you need to use clear() to increase capacity before deserialize()
        if (position + length > bytes_.capacity())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the capacity " << bytes_.capacity() << " for deserialize()!";
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

    void DynamicArray::arrayset(uint32_t position, int charval, uint32_t length)
    {
        // Should NOT exceed capacity; otherwise, you need to use clear() to increase capacity before arrayset()
        if (position + length > bytes_.capacity())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the capacity " << bytes_.capacity() << " for arrayset()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        if (position + length > bytes_.size())
        {
            bytes_.resize(position + length);
        }

        memset((void*)(bytes_.data() + position), charval, length);
        return;
    }

    void DynamicArray::serialize(uint32_t position, char* data, uint32_t length) const
    {
        // Should NOT exceed size; otherwise, you need to use deserialize()/arrayset() to increase size before serialize()
        if (position + length > bytes_.size())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the size " << bytes_.size() << " for serialize()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(data != NULL);
        memcpy((void*)data, (const void*)(bytes_.data() + position), length);
        return;
    }

    void DynamicArray::arraycpy(uint32_t position, DynamicArray& dstarray, uint32_t dstarray_position, uint32_t length) const
    {
        // Should NOT exceed size; otherwise, you need to use deserialize()/arrayset() to increase size before arraycpy()
        if (position + length > bytes_.size())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the size " << bytes_.size() << " for arraycpy()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        dstarray.deserialize(dstarray_position, bytes_.data() + position, length);
        return;
    }

    void DynamicArray::writeBinaryFile(uint32_t position, std::fstream* fs_ptr, uint32_t length) const
    {
        // Should NOT exceed size; otherwise, you need to use deserialize()/arrayset()/readBinaryFile() to increase size before writeBinaryFile()
        if (position + length > bytes_.size())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the size " << bytes_.size() << " for writeBinaryFile()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        assert(fs_ptr != NULL);
        fs_ptr->write((const char*)(bytes_.data() + position), length);
        return;
    }
    
    void DynamicArray::readBinaryFile(uint32_t position, std::fstream* fs_ptr, uint32_t length)
    {
        // Should NOT exceed capacity; otherwise, you need to use clear() to increase capacity before readBinaryFile()
        if (position + length > bytes_.capacity())
        {
            std::ostringstream oss;
            oss << "position " << position << " + length " << length << " = " << position + length << ", which exceeds the capacity " << bytes_.capacity() << " for readBinaryFile()!";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        if (position + length > bytes_.size())
        {
            bytes_.resize(position + length);
        }

        assert(fs_ptr != NULL);
        fs_ptr->read((char *)(bytes_.data() + position), length);
        return;
    }
}
#include "AnemoneRuntime/Storage/MemoryWriter.hxx"
#include "AnemoneRuntime/Diagnostics/Debug.hxx"

#include <algorithm>

namespace Anemone::Storage
{
    MemoryWriter::MemoryWriter(std::vector<std::byte>& buffer)
        : _buffer{buffer}
    {
    }

    MemoryWriter::~MemoryWriter() = default;

    size_t MemoryWriter::Write(std::span<std::byte const> data)
    {
        AE_ASSERT(this->_position <= this->_buffer.size());

        if (not data.empty())
        {
            // Get number of bytes from position to end of the buffer.
            size_t const available = this->_buffer.size() - this->_position;

            // Compute number of bytes available to overwrite.
            size_t const overwrite = std::min(data.size(), available);

            // And number of bytes to append.
            size_t const append = data.size() - overwrite;

            if (overwrite != 0)
            {
                std::copy_n(data.data(), overwrite, this->_buffer.data() + this->_position);
                this->_position += overwrite;
            }

            if (append != 0)
            {
                AE_ASSERT(this->_buffer.size() == this->_position);
                this->_buffer.insert(this->_buffer.end(), data.data() + overwrite, data.data() + data.size());
                this->_position += append;
            }
        }

        AE_ASSERT(this->_position <= this->_buffer.size());
        return data.size();
    }

    void MemoryWriter::Flush()
    {
    }

    void MemoryWriter::SetPosition(uint64_t position)
    {
        AE_ASSERT(this->_position <= this->_buffer.size());

        this->_position = position;

        if (this->_position > this->_buffer.size())
        {
            this->_buffer.resize(this->_position);
        }

        AE_ASSERT(this->_position <= this->_buffer.size());
    }

    uint64_t MemoryWriter::GetPosition() const
    {
        AE_ASSERT(this->_position <= this->_buffer.size());
        return this->_position;
    }
}

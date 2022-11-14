#pragma once
#ifndef INCLUDE_IMPLUSPLUS_DECODER_HPP
#define INCLUDE_IMPLUSPLUS_DECODER_HPP
#include <cstddef>
#include <stdexcept>

namespace impp
{
    class decoder {
    private:
        using memory_byte = uint8_t;
        const memory_byte* _readmem = nullptr;
        size_t _readsize = 0;
        size_t _readoffset = 0;

    public:
        static decoder create(const void* mem, size_t size)
        {
            return { mem, size };
        }

        decoder(const void* mem, size_t size) : _readmem(reinterpret_cast<const memory_byte*>(mem)), _readsize(size) {}
        decoder(const decoder&) = default;

        void read(void* mem, size_t size)
        {
            if(_readsize - _readoffset < size)
                throw std::runtime_error("decoder read<mem, size>: not enough bytes.");
            memcpy(mem, _readmem + _readoffset, size);
            _readoffset += size;
        }

        template <typename tval>
        const tval& read()
        {
            if(_readsize - _readoffset < sizeof(tval))
                throw std::runtime_error("decoder read<tval>: not enough bytes.");
            auto offset = _readoffset;
            _readoffset += sizeof(tval);
            return *reinterpret_cast<const tval*>(_readmem + offset);
        }

        size_t get_readable() const
        {
            return _readsize - _readoffset;
        }

        size_t get_read_offset() const
        {
            return _readoffset;
        }

        void proceed_reading(size_t size)
        {
            if (_readsize - _readoffset < size)
                throw std::runtime_error("decoder proceed_reading: not enough bytes.");
            _readoffset += size;
        }

        template <class tval>
        const tval* peek() const
        {
            return reinterpret_cast<const tval*>(_readmem + _readoffset);
        }

        void reset()
        {
            _readoffset = 0;
        }
    };
}
#endif //INCLUDE_IMPLUSPLUS_DECODER_HPP

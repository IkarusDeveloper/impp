#pragma once
#ifndef INCLUDE_IMPLUSPLUS_ENCODER_HPP
#define INCLUDE_IMPLUSPLUS_ENCODER_HPP
#include <cstddef>
#include <stdexcept>
#include <fstream>

namespace impp
{
    class encoder {

    private:
        std::ofstream _stream;
        size_t _writesize = 0;

    public:
        static encoder create(const std::string& filename)
        {
            return { filename };
        }

        encoder(const encoder&) = default;
        encoder(const std::string& filename)
        {
            _stream = std::ofstream(filename, std::ios::binary|std::ios::out);
        }

        bool is_open() const
        {
            return _stream.is_open();
        }

        template <class tval>
        void write(const tval& val)
        {
            write(&val, sizeof(tval));
        }

        template<class pixel>
        void write_pixels(const std::vector<pixel>& pixels)
        {
            write(pixels.data(), pixels.size() * sizeof(pixel));
        }

        void write(const void* mem, size_t size)
        {
            if (!_stream.good())
                throw std::runtime_error("encoder write<mem,size> : _stream corrupted");
            _stream.write(reinterpret_cast<const char*>(mem), size);
            _writesize += size;
        }

        void cancel_write(size_t size)
        {
            if(size > _writesize)
                throw std::runtime_error("cancel_write : size is too large");
            _writesize -= size;
            _stream.seekp(_writesize);
        }

        size_t get_writesize() const
        {
            return _writesize;
        }

        void reset()
        {
            _writesize = 0;
            _stream.seekp(0);
        }
    };
}
#endif //INCLUDE_IMPLUSPLUS_ENCODER_HPP

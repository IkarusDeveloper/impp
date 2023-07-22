#pragma once
#ifndef INCLUDE_IMPLUSPLUS_BMP_HPP
#define INCLUDE_IMPLUSPLUS_BMP_HPP
#include "image.hpp"
namespace impp
{
	namespace bmp
	{
#pragma pack(push, 1)
        struct bitmap_file_header {
            uint16_t  type;       // MUST BE BM
            uint32_t  size;       // SIZE IN BYTES OF BITMAP
            uint32_t  reserved;   // RESERVED (MUST BE 0)
            uint32_t  offbits;    // OFFSET IN BYTES OF RASTER DATA
        };

        struct bitmap_info_header {
            uint32_t  ihsize;           // THE SIZE OF bitmap_info_header
            int32_t   width;            // THE WIDTH OF THE BITMAP IN PIXELS
            int32_t   height;           // THE HEIGHT OF THE BITMAP IN PIXELS :
                                        //      - UNCOMPRESSED : h > 0 means BOTTOM-UP,  h < 0 means TOP-DOWN
                                        //      - COMPRESSED : MUST BE > 0
                                        //      - YUV : ALWAYS TOP-DOWN REGARDLESS SIGN AND NEGATIVE VALUES ARE TO GET CONVERTED INTO POSITIVE VALUES

            uint16_t  planes;           // THE NUMBER OF PLANES (MUST BE 1)
            uint16_t  bitcount;         // THE NUMBER OF BITS PER PIXEL:
                                        //      - 1 MONOCHROME PALETTE. NUMCOLORS 1
                                        //      - 4 4BIT PALETTIZED. NUMCOLORS 16
                                        //      - 8 8BIT PALETTIZED. NUMCOLORS 256
                                        //      - 16 16BIT RGB. NUMCOLORS 65536
                                        //      - 24 24BIT RGB. NUMCOLORS 16MILIONS

            uint32_t  compression;      // TYPE OF COMPRESSION:
                                        //      - 0 BI_RGB NO COMPRESSION
                                        //      - 1 BI_RLE8 8BIT RLE ENCODING
                                        //      - 2 BI_RLE4 4BIT RLE ENCODING

            uint32_t  compsize;         // COMPRESSED SIZE OF IMAGE (IT CAN BE 0 IF COMPRESSION IS 0)
            int32_t   xppm;             // HORIZONTAL RESOLUTION : PIXEL/METER
            int32_t   yppm;             // VERTICAL RESOLUTION : PIXEL/METER
            uint32_t  colorcount;       // NUMBER OF ACTUALLY USED COLOR
            uint32_t  colorimp;         // NUMBER OF IMPORTANTI COLORS (0 = ALL)
        };
#pragma pack(pop)

        enum bmp_bitcount
        {
            BMP_MONOCHROME_PALETTED = 1,
            BMP_4BIT_PALETTED = 4,
            BMP_8BIT_PALETTED = 8,
            BMP_16BIT_PALETTED = 16,
            BMP_16BIT_RGB = 16,
            BMP_24BIT_BGR = 24,
            BMP_32BIT_BGRA = 32,
        };

        enum bmp_uncompression
        {
            BMP_UNCOMPRESSED_RGB = 0,
            BMP_UNCOMPRESSED_BITFIELDS = 1,
        };

        enum bmp_compression
        {
            BMP_COMPRESSION_RGB = 0,
            BMP_COMPRESSION_RLE8 = 1,
            BMP_COMPRESSION_RLE4 = 2,
        };

        namespace detail
        {
            template<class pixel>
            void load_bitmap_from_memory(const void* data, size_t len, typename image<pixel>::size* width, typename image<pixel>::size* height, std::vector<pixel>* pixels)
            {
                auto decoder = decoder::create(data, len);
                const auto& fheader = decoder.read<bitmap_file_header>();
                const auto& iheader = decoder.read<bitmap_info_header>();

                // checking file header
                if(fheader.type != 19778) //BM LETTERS
                    throw std::runtime_error("invalid bitmap file header.type: it must be BM");
                if(fheader.reserved != 0) //MUST BE 0
                    throw std::runtime_error("invalid bitmap file header.reserved: it must be 0 (0x00000000)");
                if(fheader.offbits - sizeof(bitmap_file_header) - sizeof(bitmap_info_header) > decoder.get_readable())
                    throw std::runtime_error("invalid bitmap file header.offbits: exceeded image space");
                if(fheader.size != decoder.get_readable() + decoder.get_read_offset())
                    throw std::runtime_error("invalid bitmap file header.size: incorrect file size");

                // checking info header
                /*if(iheader.ihsize != sizeof(iheader))
                    throw std::runtime_error(std::string("invalid bitmap info header.ihsize: it must be ") + std::to_string(sizeof(iheader)));*/
                if(iheader.width <= 0)
                    throw std::runtime_error("invalid bitmap info header.width: it must be > 0");
                if(iheader.height < 0 && iheader.compression != 0)
                    throw std::runtime_error("invalid bitmap info header.height: it must be > 0 for compressed bitmaps");
                if(iheader.planes != 1)
                    throw std::runtime_error("invalid bitmap info header.planes: it must be 1");
                if(iheader.bitcount != BMP_MONOCHROME_PALETTED && 
                   iheader.bitcount != BMP_4BIT_PALETTED && 
                   iheader.bitcount != BMP_8BIT_PALETTED && 
                   iheader.bitcount != BMP_16BIT_RGB && 
                   iheader.bitcount != BMP_24BIT_BGR &&
                   iheader.bitcount != BMP_32BIT_BGRA)
                    throw std::runtime_error("invalid bitmap info header.bitcount: it must be one of the following values - 1, 4, 8, 16, 24");
                if(iheader.bitcount == BMP_16BIT_RGB || iheader.bitcount == BMP_32BIT_BGRA)
                    if(iheader.compression != BMP_UNCOMPRESSED_RGB && iheader.compression != BMP_UNCOMPRESSED_BITFIELDS)
                        throw std::runtime_error("invalid bitmap info header.compression: it must be one of the following values - 0,1 for bitmap using 16/32 bpp");
                if(iheader.bitcount == BMP_MONOCHROME_PALETTED || iheader.bitcount == BMP_4BIT_PALETTED || iheader.bitcount == BMP_8BIT_PALETTED)
                    if (iheader.compression != BMP_COMPRESSION_RGB && iheader.compression != BMP_COMPRESSION_RLE4 && iheader.compression != BMP_COMPRESSION_RLE8)
                        throw std::runtime_error("invalid bitmap info header.compression: it must be one of the following values - 0,1,2 for bitmap using 1/4/8 bpp");

               
            }
        }

        template<class pixel>
        image<pixel> load_memory(const void* data, size_t len)
        {
            using imagesize = typename image<pixel>::size;
            using imagepix = typename image<pixel>::pixelvec;

            imagepix pixels{};
            imagesize width = 0, height = 0;

            try
            {
                detail::load_bitmap_from_memory(data, len, &width, &height, &pixels);
                return image<pixel>::create(width, height, std::move(pixels));
            }

            catch(const std::runtime_error& error)
            {
                error::detail::on_error(error);
                return image<pixel>::null();
            }
        }

        template<class pixel>
        image<pixel> load(const std::string& filename)
        {
            using imagesize = typename image<pixel>::size;
            using imagepix = typename image<pixel>::pixelvec;

            std::ifstream f(filename, std::ios::binary | std::ios::ate);
            if (!f.is_open())
                return image<pixel>::null();

            size_t size = f.tellg();
            f.seekg(0);

            auto buffer = std::make_unique<uint8_t[]>(size);
            f.read((char*)buffer.get(), size);
            f.close();

            imagepix pixels{};
            imagesize width = 0, height = 0;

            try
            {
                detail::load_bitmap_from_memory(buffer.get(), size, &width, &height, &pixels);
                return image<pixel>::create(width, height, std::move(pixels));
            }

            catch (const std::runtime_error& error)
            {
                error::detail::on_error(error);
                return image<pixel>::null();
            }
        }
	}
}
#endif //INCLUDE_IMPLUSPLUS_BMP_HPP

/*
MIT License

Copyright (c) 2022 IkarusDeveloper

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#ifndef INCLUDE_TGA_IMPLEMENTATION_HPP
#define INCLUDE_TGA_IMPLEMENTATION_HPP
#include "image.hpp"

#include <string.h>
#include <stdint.h>
#include <fstream>
#include "pixel.hpp"

namespace impp
{
	namespace tga
	{
		enum TGAType {
			TGA_NONE = 0,
			TGA_UNCOMPRESSED_MAPPED = 1,
			TGA_UNCOMPRESSED_RGB = 2,
			TGA_RLE_RBG = 10,
		};

#pragma pack(push, 1)
		struct tga_header
		{
			uint8_t idlen;
			uint8_t colormap_type;
			uint8_t image_type;

			uint16_t colormap_origin;
			uint16_t colormap_len;
			uint8_t  colormap_entrysize;

			uint16_t xo;
			uint16_t yo;
			uint16_t width;
			uint16_t height;
			uint8_t  bits;
			uint8_t  imagedesc;
		};
#pragma pack(pop)

		namespace detail
		{
			template<class pixelfrom, class pixelto, class palette_type>
			void tga_load_compressed_paletted(palette_type* input, const uint8_t* colormap, pixelto* pxto, size_t size)
			{
				const auto* map_pixels = reinterpret_cast<const pixelfrom*>(colormap);
				for (size_t i = 0; i < size; i++, input++, pxto++)
					pxto->from(map_pixels[*input]);
			}

			template<class pixelfrom, class pixelto>
			bool tga_load_compressed_true_color(const uint8_t* input, pixelto* pxto, size_t size, size_t remain_size)
			{
				uint8_t blockhead;
				size_t i, j, pcount;

				for (i = 0; i < size; )
				{
					if(remain_size < sizeof(pixelfrom) + 1)
						return false;

					blockhead = *input++;
					remain_size--;
					pcount = static_cast<size_t>(blockhead & 0x7F) + 1;

					if (blockhead & 0x80)
					{
						const auto* pxfrom = reinterpret_cast<const pixelfrom*>(input);
						for (j = 0; j < pcount; j++, pxto++)
							pxto->from(*pxfrom);
						input += sizeof(pixelfrom);
						remain_size -= sizeof(pixelfrom);
						i += pcount;
					}
					else
					{
						if(remain_size < sizeof(pixelfrom) * pcount)
							return false;
						const auto* pxfrom = reinterpret_cast<const pixelfrom*>(input);
						for (j = 0; j < pcount; j++, pxfrom++, pxto++)
							pxto->from(*pxfrom);
						input += sizeof(pixelfrom) * pcount;
						remain_size -= sizeof(pixelfrom) * pcount;
						i += pcount;
					}
				}

				return true;
				//return remain_size == 0;
			}

			template<class pixelfrom, class pixelto>
			void tga_load_uncompressed_true_color(uint8_t* input, pixelto* pxto, size_t size)
			{
				const auto* pxfrom = reinterpret_cast<const pixelfrom*>(input);
				pixel_convert(pxfrom, pxto, size);
			}

			template<class pixel>
			inline bool tga_load_memory(const uint8_t* data, size_t size, typename image<pixel>::size* width, typename image<pixel>::size* height, typename image<pixel>::size* bpp, std::vector<pixel>* pixels, tga_header* pheader = nullptr)
			{
				using imagesize = image<pixel>::size;
				tga_header header = *reinterpret_cast<const tga_header*>(data);
				if(pheader)
					*pheader = header;

				data += sizeof(header) + header.idlen;

				const imagesize colormap_element_size = header.colormap_entrysize / 8;
				const imagesize colormap_size = header.colormap_type == 1 ? header.colormap_len * colormap_element_size : 0;
				const uint8_t* colormap = data;

				if (size < sizeof(header) + colormap_size)
					return false;

				const imagesize pixelcount = static_cast<imagesize>(header.width) * static_cast<imagesize>(header.height);
				const imagesize pixelsize = header.colormap_len == 0 ? (header.bits / 8) : colormap_element_size;
				const imagesize datasize = size - sizeof(header) - colormap_size;
				const imagesize imgsize = pixelcount * pixelsize;

				// handling mapped images
				if (header.colormap_type == 1){
					// ACTUALLY :
					// supported mapped images can only use 16bit or 8bit palette
					if (header.bits != 8 && header.bits != 16)
						return false;
					if (header.bits * pixelcount < datasize)
						return false;
					data += colormap_size;
				}

				// ACTUALLY :
				// supported images can only use 24bit or 32 bit pixels
				if(pixelsize != 3 && pixelsize != 4)
					return false;

				std::vector<pixel> temp_pixel(pixelcount);
				auto bytes = temp_pixel.data();

				switch (header.image_type)
				{
				case TGA_UNCOMPRESSED_MAPPED: // Uncompressed paletted
				{
					switch (pixelsize) {
					case 3:
						switch (header.bits)
						{
						case 8:  tga_load_compressed_paletted<pixel24bgr, pixel>((uint8_t*)data, colormap, bytes, pixelcount); break;
						case 16: tga_load_compressed_paletted<pixel24bgr, pixel>((uint16_t*)data, colormap, bytes, pixelcount); break;
						}
					case 4:
						switch (header.bits)
						{
						case 8:  tga_load_compressed_paletted<pixel32bgra, pixel>((uint8_t*)data, colormap, bytes, pixelcount); break;
						case 16: tga_load_compressed_paletted<pixel32bgra, pixel>((uint16_t*)data, colormap, bytes, pixelcount); break;
						}
					}
					break;
				}
				case TGA_UNCOMPRESSED_RGB: // Uncompressed TrueColor
				{
					// mismatching between remaining bytes and pixel space
					if (datasize < imgsize)
						return false;

					switch(pixelsize){
					case 3:
						tga_load_uncompressed_true_color<pixel24bgr, pixel>((uint8_t*)data, bytes, pixelcount);
						break;
					case 4:
						tga_load_uncompressed_true_color<pixel32bgra, pixel>((uint8_t*)data, bytes, pixelcount);
						break;
					}

					break;
				}

				case TGA_RLE_RBG: // Compressed TrueColor
				{
					switch (header.bits)
					{
					case 24:
						if(!tga_load_compressed_true_color<pixel24bgr, pixel>(data, bytes, pixelcount, datasize))
							return false;
						break;

					case 32:
						if(!tga_load_compressed_true_color<pixel32bgra, pixel>(data, bytes, pixelcount, datasize))
							return false;
						break;
					}

					break;
				}

				default:
					return false;
				}

				*width = header.width;
				*height = header.height;
				*bpp = static_cast<int>(pixelsize);
				*pixels = std::move(temp_pixel);
				return true;
			}

			template<class pixel>
			inline bool tga_load(const char* Filename, typename image<pixel>::size* width, typename image<pixel>::size* height, typename image<pixel>::size* bpp, std::vector<pixel>* bytes, tga_header* header = nullptr)
			{
				std::ifstream f(Filename, std::ios::binary | std::ios::ate);
				if (!f.is_open())
					return false;

				size_t size = f.tellg();
				f.seekg(0);

				auto buffer = std::make_unique<uint8_t[]>(size);
				f.read((char*)buffer.get(), size);
				f.close();

				return tga_load_memory(buffer.get(), size, width, height, bpp, bytes, header);
			}
		}

		template<class pixel>
		inline image<pixel> load(const std::string& filename) {
			typename image<pixel>::pixelvec pixels{};
			typename image<pixel>::size width = 0, height = 0, bpp = 0;
			if (!detail::tga_load(filename.c_str(), &width, &height, &bpp, &pixels) || (bpp != 3 && bpp != 4))
				return image<pixel>::null();
			return image<pixel>::create(width, height, std::move(pixels));
		}

		template<class pixel>
		inline image<pixel> load_memory(const void* memory, size_t size) {
			auto* data = reinterpret_cast<const uint8_t*>(memory);
			typename image<pixel>::size width = 0, height = 0, bpp = 0;
			typename image<pixel>::pixelvec pixels{};
			if (!detail::tga_load_memory(data, size, &width, &height, &bpp, &pixels) || (bpp != 3 && bpp != 4))
				return image<pixel>::null();
			return image<pixel>::create(width, height, std::move(pixels));
		}

		template<class imagetype>
		tga_header detect_header(const imagetype& source){
			using pixel = imagetype::pixel;

			tga_header header{};
			header.bits = pixel_is24bit<pixel> ? 24 : 32;
			header.colormap_type = 0;
			header.height = static_cast<uint16_t>(source.height);
			header.width = static_cast<uint16_t>(source.width);
			header.idlen = 0;
			header.imagedesc = pixel_is24bit<pixel> ? 0 : 8;
			header.image_type = TGA_UNCOMPRESSED_RGB;
			return header;
		}

		template<class pixel>
		inline bool save(const image<pixel>& source, const std::string& filename)
		{
			std::ofstream of(filename, std::ios::binary|std::ios::out);
			if (!of.is_open() || !of.good())
				return false;

			auto header = detect_header(source);
			of.write((const char*)&header, sizeof(header));

			if constexpr (std::is_same_v<pixel, pixel_bgr_cast<pixel>>)
				of.write((const char*)source.pixels.data(), sizeof(pixel) * source.pixels.size());

			else 
			{
				auto pxto = pixel_convert<pixel_bgr_cast<pixel>>(source.pixels);
				of.write((const char*)pxto.data(), sizeof(pixel) * pxto.size());
			}

			of.close();
			return true;
		}
	}
}

#endif //INCLUDE_TGA_IMPLEMENTATION_HPP

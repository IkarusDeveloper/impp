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
#ifndef INCLUDE_IMPLUSPLUS_TGA_HPP
#define INCLUDE_IMPLUSPLUS_TGA_HPP
#include "image.hpp"

#include <string.h>
#include <stdint.h>
#include <fstream>
#include "pixel.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "error.hpp"

namespace impp
{
	namespace tga
	{
		enum tga_type {
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
			template<pixel_type pixelfrom, pixel_type pixelto, class palette_type>
			void tga_load_paletted(decoder& decoder, const uint8_t* colormap, pixelto* pxto, size_t size)
			{
				const auto* map_pixels = reinterpret_cast<const pixelfrom*>(colormap);
				for (size_t i = 0; i < size; i++, pxto++)
					pxto->from(map_pixels[decoder.read<palette_type>()]);
			}

			template<pixel_type pixelfrom, pixel_type pixelto>
			void tga_load_compressed_true_color(decoder& decoder, pixelto* pxto, size_t size)
			{
				size_t i, j, pcount;

				for (i = 0; i < size; )
				{
					const auto& blockhead = decoder.read<uint8_t>();
					pcount = static_cast<size_t>(blockhead & 0x7F) + 1;

					if (blockhead & 0x80)
					{
						const auto& from = decoder.read<pixelfrom>();
						for (j = 0; j < pcount; j++, pxto++)
							pxto->from(from);
						i += pcount;
					}
					else
					{
						for (j = 0; j < pcount; j++, pxto++)
							pxto->from(decoder.read<pixelfrom>());
						i += pcount;
					}
				}
			}

			template<pixel_type pixelfrom, pixel_type pixelto>
			void tga_load_uncompressed_true_color(decoder& decoder, pixelto* pxto, size_t size)
			{
				const auto* pxfrom = decoder.peek<pixelfrom>();
				decoder.proceed_reading(size * sizeof(pixelfrom));
				pixel_convert(pxfrom, pxto, size);
			}

			template<pixel_type pixel, class imagesize = image<pixel>::size>
			inline bool tga_load_memory(const uint8_t* data, size_t size, imagesize* width, imagesize* height, imagesize* bpp, std::vector<pixel>* pixels, tga_header* pheader = nullptr)
			{
				auto decoder = decoder::create(data, size);
				const auto& header = decoder.read<tga_header>();

				if(pheader)
					*pheader = header;

				if(header.idlen != 0)
					decoder.proceed_reading(header.idlen);

				// extracting color map info
				const auto cmap_entry_size = header.colormap_entrysize / 8;
				const auto cmap_size = header.colormap_type == 1 ? header.colormap_len * cmap_entry_size : 0;
				const auto cmap = decoder.peek<uint8_t>();

				if (decoder.get_readable() < cmap_size)
					return false;

				const auto pcount = static_cast<imagesize>(header.width) * static_cast<imagesize>(header.height);
				const auto psize = header.colormap_type == 0 ? (header.bits / 8) : cmap_entry_size;
				const auto dsize = decoder.get_readable() - cmap_size;
				const auto isize = pcount * psize;

				// handling mapped images
				if (header.colormap_type == 1){
					// supported mapped images can only use 16bit or 8bit palette
					if (header.bits != 8 && header.bits != 16)
						return false;
					if (header.bits * pcount < dsize)
						return false;

					decoder.proceed_reading(cmap_size);
				}

				// supported images can only use 24bit or 32bit pixels
				if(psize != 3 && psize != 4)
					return false;

				// allocating temp pixels
				std::vector<pixel> temp_pixel;
				temp_pixel.resize(pcount);

				// getting pixel pointer
				auto bytes = temp_pixel.data();

				switch (header.image_type)
				{

				case TGA_UNCOMPRESSED_MAPPED: // Uncompressed paletted
				{
					switch (psize) 
					{
					case 3:
						switch (header.bits)
						{
						case 8:  tga_load_paletted<pixel24bgr, pixel, uint8_t>(decoder, cmap, bytes, pcount); break;
						case 16: tga_load_paletted<pixel24bgr, pixel, uint16_t>(decoder, cmap, bytes, pcount); break;
						}
						break;

					case 4:
						switch (header.bits)
						{
						case 8:  tga_load_paletted<pixel32bgra, pixel, uint8_t>(decoder, cmap, bytes, pcount); break;
						case 16: tga_load_paletted<pixel32bgra, pixel, uint16_t>(decoder, cmap, bytes, pcount); break;
						}
						break;
					}
					break;
				}


				case TGA_UNCOMPRESSED_RGB: // Uncompressed TrueColor
				{
					// mismatching between remaining bytes and pixel space
					if (dsize < isize)
						return false;

					switch(psize)
					{
					case 3: tga_load_uncompressed_true_color<pixel24bgr, pixel>(decoder, bytes, pcount); break;
					case 4: tga_load_uncompressed_true_color<pixel32bgra, pixel>(decoder, bytes, pcount); break;
					}

					break;
				}


				case TGA_RLE_RBG: // Compressed TrueColor
				{
					switch (header.bits)
					{
					case 24: tga_load_compressed_true_color<pixel24bgr, pixel>(decoder, bytes, pcount); break;
					case 32: tga_load_compressed_true_color<pixel32bgra, pixel>(decoder, bytes, pcount); break;
					}

					break;
				}

				default:
					return false;
				}

				*width = header.width;
				*height = header.height;
				*bpp = static_cast<int>(psize);
				*pixels = std::move(temp_pixel);
				return true;
			}

			template<pixel_type pixel>
			inline bool tga_load(const char* filename, typename image<pixel>::size* width, typename image<pixel>::size* height, typename image<pixel>::size* bpp, std::vector<pixel>* bytes, tga_header* header = nullptr)
			{
				std::ifstream f(filename, std::ios::binary | std::ios::ate);
				if (!f.is_open())
					return false;

				size_t size = f.tellg();
				f.seekg(0);

				auto buffer = std::make_unique<uint8_t[]>(size);
				f.read((char*)buffer.get(), size);
				f.close();

				return tga_load_memory(buffer.get(), size, width, height, bpp, bytes, header);
			}

			template<class palette_type = uint16_t, class imagetype, pixel_type pixelfrom = typename imagetype::pixel, pixel_type pixelto = pixel_bgr_cast<pixelfrom>>
			std::tuple<std::vector<pixelto>, std::vector<palette_type>> make_mapped_data(const imagetype& source){
				// making palette
				std::unordered_map<pixelfrom, palette_type> colormap;
				std::vector<pixelto> colortable;

				// making data vector
				std::vector<palette_type> data;
				data.reserve(source.pixels.size());

				for(auto& pixel : source.pixels)
				{
					// searching for known colors
					auto iter = colormap.find(pixel);
					if(iter != colormap.end()){
						data.emplace_back(iter->second);
						continue;
					}
					
					// registering a new color
					const auto pfx = pixel_cast<pixelto>(pixel);
					data.emplace_back(static_cast<palette_type>(colortable.size()));
					colormap[pixel] = colortable.size();
					colortable.emplace_back(pfx);
				}

				return std::make_tuple(colortable, data);
			}

			template<class imagetype, pixel_type pixel = typename imagetype::pixel, pixel_type pixelto = pixel_bgr_cast<pixel>>
			std::vector<uint8_t> rle_compress_pixels(const imagetype& source){
				static auto single_rle_chunk = [](auto iter, auto end, auto& ret){
					// at least 1 element is required
					if(iter == end)
						return end;

					// searching for equality
					auto next = std::next(iter);
					for(;next != end && std::distance(iter, next) < 128; next++){
						if(*next != *iter)
							break;
					}

					const auto eqcount = std::distance(iter, next);
					if(eqcount > 1)
					{
						// encoding header
						const uint8_t eqheader = static_cast<uint8_t>(eqcount-1) | 0x80;
						ret.emplace_back(eqheader);

						// encoding color
						auto color = pixel_cast<pixelto>(*iter);
						const auto& view = pixel_bytes_view(color);

						for(const auto& b : view)
							ret.emplace_back(b);
						
						return next;
					}

					// no equality means raw packet is being appended
					next = std::next(iter);
					auto prev = iter;

					for (; next != end && std::distance(iter, next) < 128; next++, prev = next) {
						if (*next == *prev)
							break;
					}

					// encoding header
					const auto count = std::distance(iter, next);
					const uint8_t header = static_cast<uint8_t>(count-1);
					ret.emplace_back(header);

					// encoding colors
					for(; iter != next && iter != end; iter++)
					{
						auto color = pixel_cast<pixelto>(*iter);
						const auto& view = pixel_bytes_view(color);

						for (auto& b : view)
							ret.emplace_back(b);
					}

					return next;
				};

				auto iter = source.pixels.begin();
				auto end = source.pixels.end();
				
				// encoding into buffer
				std::vector<uint8_t> ret{};
				while(iter != end)
					iter = single_rle_chunk(iter, end, ret);

				return ret;
			}
		}

		template<pixel_type pixel>
		inline image<pixel> load(const std::string& filename) {
			typename image<pixel>::pixelvec pixels{};
			typename image<pixel>::size width = 0, height = 0, bpp = 0;
			try
			{
				if (!detail::tga_load(filename.c_str(), &width, &height, &bpp, &pixels) || (bpp != 3 && bpp != 4))
					return image<pixel>::null();
				return image<pixel>::create(width, height, std::move(pixels));
			}

			catch (const std::runtime_error& error)
			{
				error::detail::on_error(error);
				return image<pixel>::null();
			}
		}

		template<pixel_type pixel>
		inline image<pixel> load_memory(const void* memory, size_t size) {
			auto* data = reinterpret_cast<const uint8_t*>(memory);
			typename image<pixel>::size width = 0, height = 0, bpp = 0;
			typename image<pixel>::pixelvec pixels{};

			try
			{
				if (!detail::tga_load_memory(data, size, &width, &height, &bpp, &pixels) || (bpp != 3 && bpp != 4))
					return image<pixel>::null();
				return image<pixel>::create(width, height, std::move(pixels));
			}

			catch (const std::runtime_error& error)
			{
				error::detail::on_error(error);
				return image<pixel>::null();
			}
		}

		template<tga_type type, pixel_type pixel>
		constexpr uint8_t detect_bits(){
			if(type == tga_type::TGA_UNCOMPRESSED_MAPPED)
				return 16;
			return pixel_is24bit<pixel> ? 24 : 32;
		}

		template<tga_type type, class imagetype>
		tga_header detect_header(const imagetype& source){
			using pixel = imagetype::pixel;

			tga_header header{};
			header.bits = detect_bits<type, pixel>();
			header.colormap_type = type == tga_type::TGA_UNCOMPRESSED_MAPPED ? 1 : 0;
			header.colormap_entrysize = pixel_is24bit<pixel> ? 24 : 32;
			header.colormap_origin = tga_type::TGA_UNCOMPRESSED_MAPPED ? sizeof(header) : 0;
			header.height = static_cast<uint16_t>(source.height);
			header.width = static_cast<uint16_t>(source.width);
			header.idlen = 0;
			header.imagedesc = pixel_is24bit<pixel> ? 0 : 8;
			header.image_type = type;
			return header;
		}		

		template<tga_type type = tga_type::TGA_UNCOMPRESSED_RGB, pixel_type pixel, encoder_type encoder>
		inline bool save_to_encoder(const image<pixel>& source, encoder& enc)
		{
			using pixel_dest = pixel_bgr_cast<pixel>;

			enc.reset();
			if constexpr (type == tga_type::TGA_NONE)
				return false;

			// writing header detecting it
			auto header = detect_header<type>(source);

			// handling mapped images
			if constexpr (type == tga_type::TGA_UNCOMPRESSED_MAPPED)
			{
				auto [colortable, pixels] = detail::make_mapped_data(source);
				header.colormap_len = colortable.size();

				enc.write(header);
				enc.write_pixels(colortable);
				enc.write(pixels.data(), pixels.size() * sizeof(pixels[0]));
				return true;
			}

			// handling RLE images
			else if constexpr (type == tga_type::TGA_RLE_RBG)
			{
				auto compressed_pixels = detail::rle_compress_pixels(source);
				enc.write(header);
				enc.write(compressed_pixels.data(), compressed_pixels.size());
				return true;
			}

			// handling uncompressed rgb
			else if constexpr (type == tga_type::TGA_UNCOMPRESSED_RGB)
			{
				enc.write(header);
				if constexpr (std::is_same_v<pixel, pixel_dest>)
					enc.write_pixels(source.pixels);
				else
					enc.write_pixels(
						pixel_convert<pixel_dest>(source.pixels));
				return true;
			}

			return false;
		}

		template<tga_type type = tga_type::TGA_UNCOMPRESSED_RGB, pixel_type pixel>
		inline bool save_to_file(const image<pixel>& source, const std::string& filename)
		{
			try
			{
				// using encoder to write output file
				auto enc = file_encoder::create(filename);
				if(!enc.is_open())
					return false;

				return save_to_encoder<type>(source, enc);
			}

			catch(const std::runtime_error& error)
			{
				error::detail::on_error(error);
				return false;
			}
		}

		template<tga_type type = tga_type::TGA_UNCOMPRESSED_RGB, pixel_type pixel>
		inline bool save_to_memory(const image<pixel>& source, memory_encoder& encoder)
		{
			try
			{
				return save_to_encoder<type>(source, encoder);
			}

			catch(const std::runtime_error& error)
			{
				error::detail::on_error(error);
				return false;
			}
		}
	}
}

#endif //INCLUDE_IMPLUSPLUS_TGA_HPP

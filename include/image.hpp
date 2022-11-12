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
#ifndef INCLUDE_IMAGE_IMPLEMENTATION_HPP
#define INCLUDE_IMAGE_IMPLEMENTATION_HPP
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "pixel.h"

namespace impp
{

	class image
	{
	public:
		using position = size_t;
		using pixelvec = std::vector<pixel32rgba>;
		enum orientation_value { LEFT_BOTTOM = 0, LEFT_TOP = 1 };
		static image from_file(const std::string& filename);
		static image from_buffer(void* memory, size_t size);
		static image create(position width, position height);
		static image create(position width, position height, pixelvec&& bytes);
		static image null(){ return create(0,0); }

	protected:
		image() = default;

	public:
		image(const image&) = default;
		image(image&& r);
		image& operator=(const image&) = default;
		image& operator=(image&&) = default;
		
		bool empty() const { return pixels.empty(); }
		const uint8_t* get_bytes() const { return reinterpret_cast<const uint8_t*>(pixels.data()); }
		
		void set_orientation(orientation_value ort);
		void set_pixel(position x, position y, const pixel32rgba& color);
		const pixel32rgba* get_pixel(position x, position y) const;
		void fill_rect(position x, position y, position width, position height, const pixel32rgba& color);
		void blank_rect(position x, position y, position width, position height);
		void overwrite(position x, position y, const image& src);
		void vertical_mirror();
		void horizontal_mirror();

	public:
		position width = 0;
		position height = 0;
		pixelvec pixels;
		orientation_value orientation = LEFT_TOP;
	};

	inline image image::create(position width, position height)
	{
		image ret;
		ret.pixels.resize(width * height);
		ret.width = width;
		ret.height = height;
		return ret;
	}

	inline image image::create(position width, position height, pixelvec&& bytes)
	{
		image ret;
		ret.pixels = std::forward<pixelvec>(bytes);
		ret.width = width;
		ret.height = height;
		return ret;
	}

	inline image::image(image&& r) :
		width(r.width),
		height(r.height)
	{
		pixels = std::move(r.pixels);
	}

	inline void image::set_orientation(orientation_value ort)
	{
		orientation = ort;
	}

	inline void image::set_pixel(position x, position y, const pixel32rgba& color)
	{
		// avoiding violation accessing on memory
		if (x < 0 || x >= width)
			return;
		if (y < 0 || y >= height)
			return;

		// reversing y axis
		if (orientation == LEFT_TOP)
			y = height - y -1;

		auto idx = y * width + x;
		pixels[idx] = color;
	}


	inline const pixel32rgba* image::get_pixel(position x, position y) const
	{
		// avoiding violation accessing on memory
		if (x >= width)
			return nullptr;
		if (y >= height)
			return nullptr;

		// reversing y axis
		if (orientation == LEFT_TOP)
			y = height - y -1;

		auto idx = y * width + x;
		return pixels.data() + idx;
	}

	inline void image::fill_rect(position x, position y, position w, position h, const pixel32rgba& color) {
		// avoiding violation accessing on memory
		const auto fx = std::min<position>(x + w, width);
		const auto fy = std::min<position>(y + h, height);

		// changing pixels
		for (position py = y; py < fy; py++)
			for (position px = x; px < fx; px++)
				set_pixel(px,py,color);
	}

	inline void image::blank_rect(position x, position y, position width, position height) {
		fill_rect(x, y, width, height, pixel32rgba{});
	}

	inline void image::overwrite(position x, position y, const image& source) {
		auto oror = orientation;
		const auto fx = std::min<size_t>(x + source.width, width);
		const auto fy = std::min<size_t>(y + source.height, height);
		auto sy = 0;
		set_orientation(source.orientation);
		for(size_t py = y; py < fy; py++, sy++)
			for(size_t px = x, sx = 0; px < fx; px++, sx++)
				if(auto* color = source.get_pixel(sx,sy))
					set_pixel(px,py,*color);
		set_orientation(oror);
	}

	inline void image::vertical_mirror()
	{
		auto copy = pixels;
		const auto* from = copy.data() + copy.size() - width;
		auto* to = pixels.data();
		for(size_t py = 0; py < height; py++, from -= width, to += width)
			memcpy(to, from, width * sizeof(*from));
	}

	inline void image::horizontal_mirror()
	{
		auto* from = pixels.data();
		auto* to = pixels.data() + width;
		for(size_t py = 0; py < height; py++, from += width, to += width)
			std::reverse(from, to);
	}
}

#endif //INCLUDE_IMAGE_IMPLEMENTATION_HPP
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
#ifndef INCLUDE_IMPLUSPLUS_IMAGE_HPP
#define INCLUDE_IMPLUSPLUS_IMAGE_HPP
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "pixel.hpp"

namespace impp
{
	template<class _pixel = pixel32rgba>
	class image
	{
	public:
		using size = uint32_t;
		using pixel = _pixel;
		using pixelvec = std::vector<pixel>;

		enum orientation_value { LEFT_BOTTOM = 0, LEFT_TOP = 1 };
		static image from_file(const std::string& filename);
		static image from_buffer(void* memory, size_t size);
		static image create(size width, size height);
		static image create(size width, size height, pixelvec&& bytes);
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
		void set_pixel(size x, size y, const pixel& color);
		const pixel* get_pixel(size x, size y) const;
		void fill_rect(size x, size y, size width, size height, const pixel& color);
		void blank_rect(size x, size y, size width, size height);
		void overwrite(size x, size y, const image& src);
		void vertical_mirror();
		void horizontal_mirror();

	public:
		size width = 0;
		size height = 0;
		pixelvec pixels;
		orientation_value orientation = LEFT_TOP;
	};

	using image32rgba = image<pixel32rgba>;
	using image32bgra = image<pixel32bgra>;
	using image24rgb = image<pixel24rgb>;
	using image24bgr = image<pixel24bgr>;

	template<class pixel>
	inline image<pixel> image<pixel>::create(size width, size height)
	{
		image ret;
		ret.pixels.resize(width * height);
		ret.width = width;
		ret.height = height;
		return ret;
	}

	template<class pixel>
	inline image<pixel> image<pixel>::create(size width, size height, pixelvec&& bytes)
	{
		image ret;
		ret.pixels = std::forward<pixelvec>(bytes);
		ret.width = width;
		ret.height = height;
		return ret;
	}

	template<class pixel>
	inline image<pixel>::image(image&& r) :
		width(r.width),
		height(r.height)
	{
		pixels = std::move(r.pixels);
	}

	template<class pixel>
	inline void image<pixel>::set_orientation(orientation_value ort)
	{
		orientation = ort;
	}

	template<class pixel>
	inline void image<pixel>::set_pixel(size x, size y, const pixel& color)
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

	template<class pixel>
	inline const pixel* image<pixel>::get_pixel(size x, size y) const
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

	template<class pixel>
	inline void image<pixel>::fill_rect(size x, size y, size w, size h, const pixel& color) {
		// avoiding violation accessing on memory
		const auto fx = std::min<size>(x + w, width);
		const auto fy = std::min<size>(y + h, height);

		// changing pixels
		for (size py = y; py < fy; py++)
			for (size px = x; px < fx; px++)
				set_pixel(px,py,color);
	}

	template<class pixel>
	inline void image<pixel>::blank_rect(size x, size y, size width, size height) {
		fill_rect(x, y, width, height, pixel{});
	}

	template<class pixel>
	inline void image<pixel>::overwrite(size x, size y, const image& source) {
		auto oror = orientation;
		const auto fx = std::min<size>(x + source.width, width);
		const auto fy = std::min<size>(y + source.height, height);
		auto sy = 0;
		set_orientation(source.orientation);
		for (size_t py = y; py < fy; py++, sy++)
			for (size_t px = x, sx = 0; px < fx; px++, sx++)
				if (auto* color = source.get_pixel(sx, sy))
					set_pixel(px, py, *color);
		set_orientation(oror);
	}

	template<class pixel>
	inline void image<pixel>::vertical_mirror()
	{
		auto copy = pixels;
		const auto* from = copy.data() + copy.size() - width;
		auto* to = pixels.data();
		for (size_t py = 0; py < height; py++, from -= width, to += width)
			memcpy(to, from, width * sizeof(*from));
	}

	template<class pixel>
	inline void image<pixel>::horizontal_mirror()
	{
		auto* from = pixels.data();
		auto* to = pixels.data() + width;
		for(size_t py = 0; py < height; py++, from += width, to += width)
			std::reverse(from, to);
	}

	template<class pixelto, class pixelfrom>
	image<pixelto> image_convert(const image<pixelfrom>& source)
	{
		auto pixels = pixel_convert<pixelto>(source.pixels);
		return image<pixelto>::create(source.width, source.height, std::move(pixels));
	}
}

#endif //INCLUDE_IMPLUSPLUS_IMAGE_HPP

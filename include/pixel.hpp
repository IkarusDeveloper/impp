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
#ifndef INCLUDE_IMPLUSPLUS_PIXEL_HPP
#define INCLUDE_IMPLUSPLUS_PIXEL_HPP
#include <type_traits>
#include <vector>
#include <array>

namespace impp
{
	struct pixel24rgb;
	struct pixel24bgr;
	struct pixel32rgba;
	struct pixel32bgra;

	template<class pixel>
	constexpr bool pixel_is24bit = std::is_same_v<pixel, pixel24bgr> || std::is_same_v<pixel, pixel24rgb>;
	template<class pixel>
	constexpr bool pixel_is32bit = std::is_same_v<pixel, pixel32rgba> || std::is_same_v<pixel, pixel32bgra>;

	namespace detail{
		template<class pixel>
		struct pixel_brg_cast;
		template<>
		struct pixel_brg_cast<pixel24rgb> {
			using type = pixel24bgr;
		};
		template<>
		struct pixel_brg_cast<pixel24bgr> {
			using type = pixel24bgr;
		};
		template<>
		struct pixel_brg_cast<pixel32rgba> {
			using type = pixel32bgra;
		};
		template<>
		struct pixel_brg_cast<pixel32bgra> {
			using type = pixel32bgra;
		};
	}
	template<class pixel>
	using pixel_bgr_cast = detail::pixel_brg_cast<pixel>::type;

	struct pixel24rgb;
	struct pixel24bgr;
	struct pixel32rgba;
	struct pixel32bgra;

	template<class type>
	concept pixel_type = std::is_same_v<type, pixel24bgr> || std::is_same_v<type, pixel24rgb> || std::is_same_v<type, pixel32bgra> || std::is_same_v<type, pixel32rgba>;

	template<impp::pixel_type pixel>
	bool pixel_less(const pixel& p1, const pixel& p2) {
		if (p1.r < p2.r)
			return true;
		if (p1.g < p2.g)
			return true;
		if (p1.b < p2.b)
			return true;
		if constexpr (impp::pixel_is32bit<pixel>)
			if (p1.a < p2.a)
				return true;
		return false;
	}

	template<impp::pixel_type pixel>
	bool pixel_equal(const pixel& p1, const pixel& p2) {
		if (p1.r != p2.r)
			return false;
		if (p1.g != p2.g)
			return false;
		if (p1.b != p2.b)
			return false;
		if constexpr (impp::pixel_is32bit<pixel>)
			if (p1.a != p2.a)
				return false;
		return true;
	}

	template<impp::pixel_type pixel>
	bool pixel_greater(const pixel& p1, const pixel& p2) {
		return !(p1 < p2) && !(p1 == p2);
	}

#pragma pack(push, 1)

	struct pixel24rgb
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;

		template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel24rgb, pixel>, int> = 0>
		void from(const pixel& from);

		bool operator<(const pixel24rgb& r) const { return pixel_less(*this, r); }
		bool operator>(const pixel24rgb& r) const { return pixel_greater(*this, r); }
		bool operator==(const pixel24rgb& r) const { return pixel_equal(*this, r); }
	};

	struct pixel24bgr
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;

		template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel24bgr, pixel>, int> = 0>
		void from(const pixel& from);

		bool operator<(const pixel24bgr& r) const { return pixel_less(*this, r); }
		bool operator>(const pixel24bgr& r) const { return pixel_greater(*this, r); }
		bool operator==(const pixel24bgr& r) const { return pixel_equal(*this, r); }
	};

	struct pixel32rgba
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;

		template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel32rgba, pixel>, int> = 0>
		void from(const pixel& from);

		bool operator<(const pixel32rgba& r) const { return pixel_less(*this, r); }
		bool operator>(const pixel32rgba& r) const { return pixel_greater(*this, r); }
		bool operator==(const pixel32rgba& r) const { return pixel_equal(*this, r); }
	};

	struct pixel32bgra
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;

		template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel32bgra, pixel>, int> = 0>
		void from(const pixel& from);

		bool operator<(const pixel32bgra& r) const { return pixel_less(*this, r); }
		bool operator>(const pixel32bgra& r) const { return pixel_greater(*this, r); }
		bool operator==(const pixel32bgra& r) const { return pixel_equal(*this, r); }
	};
#pragma pack(pop)

	// various pixel casting
	namespace detail
	{
		template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<
			pixel_is32bit<pixelfrom> && pixel_is32bit<pixelto> && !std::is_same_v<pixelfrom, pixelto>, int> = 0>
		void pixel32to32(const pixelfrom& from, pixelto& to)
		{
			to.a = from.a;
			to.b = from.b;
			to.r = from.r;
			to.g = from.g;
		}

		template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<
			pixel_is24bit<pixelfrom>&& pixel_is24bit<pixelto> && !std::is_same_v<pixelfrom, pixelto>, int> = 0>
		void pixel24to24(const pixelfrom& from, pixelto& to)
		{
			to.b = from.b;
			to.r = from.r;
			to.g = from.g;
		}

		template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<
			pixel_is24bit<pixelfrom> && pixel_is32bit<pixelto>, int> = 0>
		void pixel24to32(const pixelfrom& from, pixelto& to)
		{
			to.a = UINT8_MAX;
			to.b = from.b;
			to.r = from.r;
			to.g = from.g;
		}

		template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<
			pixel_is32bit<pixelfrom> && pixel_is24bit<pixelto>, int> = 0>
		void pixel32to24(const pixelfrom& from, pixelto& to)
		{
			to.b = from.b;
			to.r = from.r;
			to.g = from.g;
		}
	}

	//pixel casts
	template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<!std::is_same_v<pixelfrom, pixelto>, int> = 0>
	void pixel_cast(const pixelfrom& from, pixelto& to){
		if constexpr(pixel_is24bit<pixelfrom> && pixel_is24bit<pixelto>)
			detail::pixel24to24(from, to);
		else if constexpr (pixel_is32bit<pixelfrom> && pixel_is32bit<pixelto>)
			detail::pixel32to32(from, to);
		else if constexpr (pixel_is24bit<pixelfrom> && pixel_is32bit<pixelto>)
			detail::pixel24to32(from, to);
		else
			detail::pixel32to24(from, to);
	}

	template<pixel_type pixelto, pixel_type pixelfrom>
	pixelto pixel_cast(const pixelfrom& from) {
		if constexpr ( std::is_same_v<pixelto, pixelfrom> )
			return from;
		else{
			pixelto px;
			pixel_cast(from, px);
			return px;
		}
	}

	//post process alpha channel making it simulated using a background color
	template<pixel_type pixelfrom, pixel_type pixelto, std::enable_if_t<
		pixel_is32bit<pixelfrom>&& pixel_is24bit<pixelto>, int> = 0>
	void postprocess_pixel32to24(const pixelfrom& from, pixelto& to, const pixelto& bg)
	{
		to.b = bg.b * (1.0 - from.a) + from.b * from.a;
		to.r = bg.r * (1.0 - from.a) + from.r * from.a;
		to.g = bg.g * (1.0 - from.a) + from.g * from.a;
	}

	//impl of pixel24rgb
	template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel24rgb, pixel>, int>>
	void pixel24rgb::from(const pixel& from){ pixel_cast(from, *this); }

	//impl of pixel24bgr
	template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel24bgr, pixel>, int>>
	void pixel24bgr::from(const pixel& from) { pixel_cast(from, *this); }

	//impl of pixel32rgba
	template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel32rgba, pixel>, int>>
	void pixel32rgba::from(const pixel& from) { pixel_cast(from, *this); }

	//impl of pixel32bgra
	template<pixel_type pixel, std::enable_if_t<!std::is_same_v<pixel32bgra, pixel>, int>>
	void pixel32bgra::from(const pixel& from) { pixel_cast(from, *this); }

	// various methods to convert pixels
	template<pixel_type pixelfrom, pixel_type pixelto,
		std::enable_if_t<!std::is_same_v<pixelfrom, pixelto>, int> = 0>
	void pixel_convert(std::vector<uint8_t>* bytes, int pcount) {
		std::vector<uint8_t> dest(static_cast<size_t>(pcount) * sizeof(pixelto));
		auto* pfrom = reinterpret_cast<pixelfrom*>(dest.data());
		auto* pto = reinterpret_cast<pixelto*>(bytes->data());
		for (int i = 0; i < pcount; i++, pfrom++, pto++)
			pfrom->from(*pto);
		*bytes = std::move(dest);
	}

	template<pixel_type pixelfrom, pixel_type pixelto,
		std::enable_if_t<!std::is_same_v<pixelfrom, pixelto>, int> = 0>
	void pixel_convert(const std::vector<pixelfrom>& from, std::vector<pixelto>& to) {
		if(to.size() != from.size())
			to.resize(from.size());
		auto* pfrom = from.data();
		auto* pto = to.data();
		for (size_t i = 0; i < from.size(); i++, pfrom++, pto++)
			pto->from(*pfrom);
	}

	template<pixel_type pixelto, pixel_type pixelfrom,
		std::enable_if_t<!std::is_same_v<pixelfrom, pixelto>, int> = 0>
	std::vector<pixelto> pixel_convert(const std::vector<pixelfrom>& from) {
		std::vector<pixelto> to(from.size());
		auto* pfrom = from.data();
		auto* pto = to.data();
		for (size_t i = 0; i < from.size(); i++, pfrom++, pto++)
			pto->from(*pfrom);
		return to;
	}

	template<pixel_type pixelfrom, pixel_type pixelto,
		std::enable_if_t<!std::is_same_v<pixelfrom, pixelto>, int> = 0>
	void pixel_convert(const pixelfrom* from, pixelto* to, size_t pcount) {
		for (size_t i = 0; i < pcount; i++, from++, to++)
			to->from(*from);
	}

	template<pixel_type pixel, std::enable_if_t<pixel_is24bit<pixel>, int> = 0>
	const std::array<uint8_t, 3>& pixel_bytes_view(const pixel& px){
		return reinterpret_cast<const std::array<uint8_t, 3>&>(px);
	}

	template<pixel_type pixel, std::enable_if_t<pixel_is32bit<pixel>, int> = 0>
	const std::array<uint8_t, 4>& pixel_bytes_view(const pixel& px){
		return reinterpret_cast<const std::array<uint8_t, 4>&>(px);
	}
}

template<impp::pixel_type pixel>
struct std::hash<pixel> {
	size_t operator()(const pixel& value) const {
		auto fpx = impp::pixel_cast<impp::pixel32rgba>(value);
		return static_cast<size_t>(reinterpret_cast<uint32_t&>(fpx));
	}
};

#endif //INCLUDE_IMPLUSPLUS_PIXEL_HPP

#ifndef PDN_Header_pdn_bom_reader
#define PDN_Header_pdn_bom_reader

#include <cstddef>
#include <cstdint>
#include <istream>
#include <algorithm>
#include <utility>
#include <array>
#include <span>
#include <type_traits>

#include "pdn_unicode_base.h"

//    byte input stream (provide: get byte) // such as ifstream
//     |
// >>  +---> BOM reader (provide: get BOM)
//     |         extra: requires byte input stream has operation seekg
//     v
//    swap chain (provide: get byte)
//     |
//     v
//    code unit iterator (provide: get code unit, move backward(++it), check EOF)
//     |
//     +---- utf-8 decoder ---+
//     |                      |
//     +---- utf-16 decoder --+---- (provide: decode to get unicode code-point)
//     |                      |
//     +---- utf-32 decoder --+
//     v
//    code point iterator (provide: get code point, move backward(++it), check EOF)
//     |
//     v
//    lexer (provide: get token{ token code, value, token position })
//     |
//     v
//    parser (provide: parse) ----> pdn document object model

namespace pdn::unicode::concepts
{
	template <typename istream>
	concept ibyte_stream = requires(istream input)
	{
		typename ::std::remove_reference_t<istream>::char_type;
		requires sizeof(typename ::std::remove_reference_t<istream>::char_type) == 1;
		input.seekg(::std::declval<::std::streamoff>(), ::std::ios_base::cur);
	};
}

namespace pdn::bom
{
	using byte_t = ::std::uint8_t;
	inline constexpr auto utf_16_le = ::std::array<byte_t, 2>{ 0xFF, 0xFE };
	inline constexpr auto utf_16_be = ::std::array<byte_t, 2>{ 0xFE, 0xFF };
	inline constexpr auto utf_32_le = ::std::array<byte_t, 4>{ 0xFF, 0xFE, 0x00, 0x00 };
	inline constexpr auto utf_32_be = ::std::array<byte_t, 4>{ 0x00, 0x00, 0xFE, 0xFF };
	inline constexpr auto utf_8     = ::std::array<byte_t, 3>{ 0xEF, 0xBB, 0xBF };
}

namespace pdn::unicode
{
	// read bom from input stream
	// using input.seekg(...) to sets the input position indicator to the original position when failed in read BOM.
	template <concepts::ibyte_stream istream_t>
	bom_type read_bom(istream_t&& input)
	{
		using istream_type = ::std::remove_reference_t<istream_t>;
		using char_type    = typename istream_type::char_type;
		using byte_type    = bom::byte_t;
		using namespace bom;

		if (!input)
		{
			return bom_type::no_bom;
		}
		::std::array<byte_type, 4> my_bom{}; // BOM byte sequence

		input.read(reinterpret_cast<char_type*>(my_bom.data()), 4);

		if (input.fail())
		{
			input.clear(input.rdstate() & (::std::ios::badbit | ::std::ios::eofbit));
		}
		auto read_count = input.gcount();
		auto bom_size = read_count;

		bom_type result{};

		switch (bom_size)
		{
		case 4:
			if (my_bom == utf_32_le)
			{
				result = bom_type::utf_32_le;
				break;
			}
			if (my_bom == utf_32_be)
			{
				result = bom_type::utf_32_be;
				break;
			}
			bom_size = 3;
			[[fallthrough]];
		case 3:
			if (::std::equal(utf_8.cbegin(), utf_8.cend(), my_bom.begin()))
			{
				result = bom_type::utf_8;
				break;
			}
			bom_size = 2;
			[[fallthrough]];
		case 2:
			if (::std::equal(utf_16_le.cbegin(), utf_16_le.cend(), my_bom.begin()))
			{
				result = bom_type::utf_16_le;
				break;
			}
			if (::std::equal(utf_16_be.cbegin(), utf_16_be.cend(), my_bom.begin()))
			{
				result = bom_type::utf_16_be;
				break;
			}
			// assign bom_size zero in next step
			[[fallthrough]];
		default:
			bom_size = 0;
			result = bom_type::no_bom;
			break;
		}

		if (read_count != bom_size)
		{
			input.seekg(static_cast<::std::streamoff>(bom_size - read_count), ::std::ios_base::cur);
		}

		return result;
	}
}

#endif

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

namespace pdn::unicode
{
	// read bom from input stream
	// using input.seekg(...) to sets the input position indicator to the original position when failed in read BOM.
	template <concepts::ibyte_stream istream_t>
	auto read_bom(istream_t& input) -> bom_type
	{
		using istream_type = ::std::remove_reference_t<istream_t>;
		using char_type    = typename istream_type::char_type;
		using byte_type    = bom::byte_t;
		using enum bom_type;

		if (!input)
		{
			return no_bom;
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
			if (my_bom == bom::utf32_le)
			{
				result = utf32_le;
				break;
			}
			if (my_bom == bom::utf32_be)
			{
				result = utf32_be;
				break;
			}
			bom_size = 3;
			[[fallthrough]];
		case 3:
			if (::std::equal(bom::utf8.cbegin(), bom::utf8.cend(), my_bom.begin()))
			{
				result = utf8;
				break;
			}
			bom_size = 2;
			[[fallthrough]];
		case 2:
			if (::std::equal(bom::utf16_le.cbegin(), bom::utf16_le.cend(), my_bom.begin()))
			{
				result = utf16_le;
				break;
			}
			if (::std::equal(bom::utf16_be.cbegin(), bom::utf16_be.cend(), my_bom.begin()))
			{
				result = utf16_be;
				break;
			}
			// assign bom_size zero in next step
			[[fallthrough]];
		default:
			bom_size = 0;
			result = no_bom;
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

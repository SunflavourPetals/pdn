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
	bom_type read_bom(istream_t&& input)
	{
		using istream_type = ::std::remove_reference_t<istream_t>;
		using char_type = typename istream_type::char_type;
		using byte_type = ::std::uint8_t;

		if (!input)
		{
			return bom_type::no_bom;
		}
		::std::array<byte_type, 4> bom{}; // BOM byte sequence

		input.read(reinterpret_cast<char_type*>(bom.data()), 4);

		if (input.fail())
		{
			input.clear(input.rdstate() & (::std::ios::badbit | ::std::ios::eofbit));
		}
		auto read_count = input.gcount();
		auto bom_size = read_count;

		static ::std::array<byte_type, 2> utf_16_le{ 0xFF, 0xFE };
		static ::std::array<byte_type, 2> utf_16_be{ 0xFE, 0xFF };
		static ::std::array<byte_type, 4> utf_32_le{ 0xFF, 0xFE, 0x00, 0x00 };
		static ::std::array<byte_type, 4> utf_32_be{ 0x00, 0x00, 0xFE, 0xFF };
		static ::std::array<byte_type, 3> utf_8{ 0xEF, 0xBB, 0xBF };

		bom_type result{};

		if (bom_size == 4)
		{
			if (bom == utf_32_le)
			{
				result = bom_type::utf_32_le;
				goto out_of_judge;
			}
			if (bom == utf_32_be)
			{
				result = bom_type::utf_32_be;
				goto out_of_judge;
			}
			bom_size = 3;
			goto bom_size_3;
		}
		if (bom_size == 3)
		{
		bom_size_3:
			if (::std::equal(utf_8.cbegin(), utf_8.cend(), bom.begin()))
			{
				result = bom_type::utf_8;
				goto out_of_judge;
			}
			bom_size = 2;
			goto bom_size_2;
		}
		if (bom_size == 2)
		{
		bom_size_2:
			if (::std::equal(utf_16_le.cbegin(), utf_16_le.cend(), bom.begin()))
			{
				result = bom_type::utf_16_le;
				goto out_of_judge;
			}
			if (::std::equal(utf_16_be.cbegin(), utf_16_be.cend(), bom.begin()))
			{
				result = bom_type::utf_16_be;
				goto out_of_judge;
			}
			// assign bom_size zero in next step
			goto bom_no_bom;
		}
		if (bom_size < 2)
		{
		bom_no_bom:
			bom_size = 0;
			result = bom_type::no_bom;
		}

	out_of_judge:

		if (read_count != bom_size)
		{
			input.seekg(static_cast<::std::streamoff>(bom_size - read_count), ::std::ios_base::cur);
		}

		return result;
	}
}

#endif

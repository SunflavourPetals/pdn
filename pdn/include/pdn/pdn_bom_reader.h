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

//    byte input stream (provide: read byte...) such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |
//     v
//    byte iterator (provide: get byte, advance(++it), check EOF)
//     |
//     v
//    code unit iterator (provide: get code unit, advance(++it), check EOF)
//     |
//     +---- utf-8 decoder ---+
//     |                      |
//     +---- utf-16 decoder --+---- (provide: decode to get unicode code-point)
//     |                      |
//     +---- utf-32 decoder --+
//     v
// >> code point iterator (provide: get code point, advance(++it), check EOF)
//     |
//     v
//    lexer (provide: get token{ token code, value, token position })
//     |
//     v
//    parser (provide: parse) --> pdn document object model

namespace pdn::unicode::concepts
{
	// A byte input stream as required by pdn::unicode::read_bom: one byte wide
	// char_type plus the operations the BOM probe performs on it.
	template <typename istream>
	concept ibyte_stream = requires(istream input)
	{
		typename ::std::remove_reference_t<istream>::char_type;
		requires sizeof(typename ::std::remove_reference_t<istream>::char_type) == 1;

		input.seekg(::std::declval<::std::streamoff>(), ::std::ios_base::cur);
		input.read(::std::declval<typename ::std::remove_reference_t<istream>::char_type*>(), ::std::declval<::std::streamsize>());
		input.gcount();
		input.fail();
		input.clear(::std::declval<::std::ios_base::iostate>());
		input.rdstate();
		static_cast<bool>(input);
	};
}

namespace pdn::unicode
{
	// check BOM type
	constexpr auto check_bom(::std::size_t bom_size, ::std::array<bom::byte_t, 4> bom_bytes) -> bom_type
	{
		using enum bom_type;
		switch (bom_size)
		{
		case 4:
			if (bom_bytes == bom::utf32_le) return utf32_le;
			if (bom_bytes == bom::utf32_be) return utf32_be;
			[[fallthrough]];
		case 3:
			if (::std::equal(bom::utf8.cbegin(), bom::utf8.cend(), bom_bytes.begin())) return utf8;
			[[fallthrough]];
		case 2:
			if (::std::equal(bom::utf16_le.cbegin(), bom::utf16_le.cend(), bom_bytes.begin())) return utf16_le;
			if (::std::equal(bom::utf16_be.cbegin(), bom::utf16_be.cend(), bom_bytes.begin())) return utf16_be;
			[[fallthrough]];
		default:
			return no_bom;
		}
	}

	// Reads at most 4 bytes, detects the byte order mark and seeks back over the
	// bytes that do not belong to it, so the rest of the input stays unread.
	// The stream must be seekable and seekg must succeed: this layer only probes
	// the mark, it never buffers the input for a second pass. A stream that is
	// not good on entry yields no_bom without being touched. An input shorter
	// than 4 bytes makes the probe read set failbit (and eofbit); the failbit is
	// cleared here, the eofbit is kept. If the stream carries failbit or badbit
	// in its exception mask, that short probe read throws std::ios_base::failure
	// instead of reporting the failure through the stream state.
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
		auto my_bom = ::std::array<byte_type, 4>{}; // BOM byte sequence

		input.read(reinterpret_cast<char_type*>(my_bom.data()), ::std::streamsize{ 4 });

		if (input.fail())
		{
			input.clear(input.rdstate() & (::std::ios::badbit | ::std::ios::eofbit));
		}
		auto read_count = input.gcount();

		auto result = check_bom(read_count, my_bom);
		auto bom_size = to_byte_size(result);

		if (static_cast<::std::size_t>(read_count) != bom_size)
		{
			input.seekg(static_cast<::std::streamoff>(bom_size) - read_count, ::std::ios_base::cur);
		}

		return result;
	}
}

#endif
